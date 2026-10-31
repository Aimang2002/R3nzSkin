#pragma warning(disable : 6387 4715)

#include <Windows.h>
#include <clocale>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>

#include "CheatManager.hpp"

#include "Config.hpp"
#include "Hooks.hpp"
#include "Memory.hpp"

#include "SDK/GameState.hpp"

bool WINAPI HideThread(const HANDLE hThread) noexcept
{
	__try {
		using FnSetInformationThread = NTSTATUS(NTAPI*)(HANDLE ThreadHandle, UINT ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
		const auto NtSetInformationThread{ reinterpret_cast<FnSetInformationThread>(::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtSetInformationThread")) };

		if (!NtSetInformationThread)
			return false;

		if (const auto status{ NtSetInformationThread(hThread, 0x11u, nullptr, 0ul) }; status == 0x00000000)
			return true;
	} __except (TRUE) {
		return false;
	}
	return false;
}

// Inert switch: if this file exists next to the game executable the mod does
// nothing at all (no scan, no hooks), so the game can always be entered.
static bool is_disabled_by_user() noexcept
{
	wchar_t exe_path[MAX_PATH]{};
	if (!::GetModuleFileNameW(nullptr, exe_path, MAX_PATH))
		return false;

	std::filesystem::path flag{ exe_path };
	flag.replace_filename(L"R3nzSkin.off");

	std::error_code ec;
	return std::filesystem::exists(flag, ec);
}

static void log_to_file(const char* text) noexcept
{
	char temp[MAX_PATH]{};
	if (::GetTempPathA(MAX_PATH, temp) == 0)
		return;

	std::ofstream out{ std::string{ temp } + "R3nzSkin.log", std::ios::app };
	if (out)
		out << text << '\n';
}

static HMODULE this_module{ nullptr };

// Silent exit: drop our module reference and end the worker thread without
// touching the game. If the loader (hid.dll) still owns a reference Windows
// keeps the module mapped, but there is no thread left doing anything.
[[noreturn]] static void exit_silently(const char* reason) noexcept
{
	using namespace std::chrono_literals;

	log_to_file(reason);

	// The worker thread may start inside DllMain while the loader is still inside
	// LoadLibrary. Give the load time to finish before dropping our reference.
	std::this_thread::sleep_for(500ms);

	// Best-effort self-unload. If the game still holds a reference to hid.dll
	// (it normally does) Windows keeps the module mapped, but this thread ends
	// and nothing else of ours runs: no scanning, no hooks, no memory writes.
	if (this_module)
		::FreeLibraryAndExitThread(this_module, 0u);

	::ExitThread(0u);
}

__declspec(safebuffers) static void WINAPI DllAttach([[maybe_unused]] LPVOID lp) noexcept
{
	using namespace std::chrono_literals;

	// The dll is loaded at process start (hid.dll / DLL hijacking). Stay
	// completely idle while the game creates its renderer and the anti-cheat
	// unpacks its code: no allocations, no API calls, no memory scan.
	std::this_thread::sleep_for(5s);

	if (is_disabled_by_user())
		exit_silently("[OFF] R3nzSkin.off found, mod disabled and unloaded");

	std::this_thread::sleep_for(5s);

	std::setlocale(LC_ALL, ".utf8");
	cheatManager.start();
	if (HideThread(::GetCurrentThread()))
		cheatManager.logger->addLog("Thread Hidden!\n");

	if (!cheatManager.memory->Search(true))
		exit_silently("[!!] game client not found, mod disabled and unloaded");

	std::uint32_t empty_client_attempts{ 0 };
	while (true) {
		std::this_thread::sleep_for(1s);
		
		if (!cheatManager.memory->client) {
			if (!cheatManager.memory->Search(true))
				exit_silently("[!!] game client not found, mod disabled and unloaded");

			if (!cheatManager.memory->client && ++empty_client_attempts > 60)
				exit_silently("[!!] game client never appeared, mod disabled and unloaded");
		} else if (cheatManager.memory->client->get_game_state() == GGameState_s::Running)
			break;
	}
	cheatManager.logger->addLog("GameClient found!\n");
	
	std::this_thread::sleep_for(500ms);
	if (!cheatManager.memory->Search(false))
		exit_silently("[!!] offsets not found, mod disabled and unloaded");
	cheatManager.logger->addLog("All offsets found!\n");
	std::this_thread::sleep_for(500ms);

	cheatManager.config->init();
	cheatManager.config->load();
	cheatManager.gui->is_open = cheatManager.config->isOpen;
	cheatManager.logger->addLog("CFG loaded!\n");

	// Last safety check before taking over Present: without a swap chain/window
	// the overlay cannot work, and hooking would only risk the running game.
	if (!cheatManager.memory->swapChain || !cheatManager.memory->window)
		exit_silently("[!!] renderer not ready, mod disabled and unloaded");

	cheatManager.hooks->install();
		
	while (cheatManager.cheatState)
		std::this_thread::sleep_for(250ms);

	::ExitProcess(0u);
}

__declspec(safebuffers) BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD reason, [[maybe_unused]] LPVOID reserved)
{
	DisableThreadLibraryCalls(hModule);

	if (reason != DLL_PROCESS_ATTACH)
		return TRUE;

	this_module = hModule;

	// Keep DllMain minimal: it runs under the loader lock while the game is
	// starting. All real work happens later on the worker thread.
	::_beginthreadex(nullptr, 0u, reinterpret_cast<_beginthreadex_proc_type>(DllAttach), nullptr, 0u, nullptr);
	::CloseHandle(hModule);
	return TRUE;
}
