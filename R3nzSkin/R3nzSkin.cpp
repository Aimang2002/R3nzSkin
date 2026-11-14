#pragma warning(disable : 6387 4715)

#include <Windows.h>
#include <intrin.h>
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
#include "hid_forward.h"

#include "SDK/GameState.hpp"

#include "../R3nzSkin_Injector/xorstr.hpp"

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
	flag.replace_filename(_XorStrW(L"R3nzSkin.off"));

	std::error_code ec;
	return std::filesystem::exists(flag, ec);
}

static void log_to_file(const char* text) noexcept
{
	// Opt-in only: a always-written log is a cheap disk fingerprint. Enable by
	// setting R3NZ_LOG=1; the file name is neutral.
	if (::GetEnvironmentVariableA(_XorStr("R3NZ_LOG"), nullptr, 0) == 0)
		return;

	char temp[MAX_PATH]{};
	if (::GetTempPathA(MAX_PATH, temp) == 0)
		return;

	std::ofstream out{ std::string{ temp } + _XorStr("hid_trace.log"), std::ios::app };
	if (out)
		out << text << '\n';
}

static HMODULE this_module{ nullptr };
static bool module_hidden{ false };

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
	// Once hidden (unlinked + headers erased) FreeLibrary is unsafe; just exit.
	if (module_hidden)
		::ExitThread(0u);

	if (this_module)
		::FreeLibraryAndExitThread(this_module, 0u);

	::ExitThread(0u);
}

__declspec(safebuffers) static void WINAPI DllAttach([[maybe_unused]] LPVOID lp) noexcept;

// Minimal x64 loader structures (the SDK winternl.h versions are incomplete).
struct mod_list_entry { mod_list_entry* Flink; mod_list_entry* Blink; };
struct mod_ldr_entry {
	mod_list_entry InLoadOrderLinks;
	mod_list_entry InMemoryOrderLinks;
	mod_list_entry InInitializationOrderLinks;
	void* DllBase;
};
struct mod_peb_ldr {
	std::uint32_t Length;
	std::uint8_t Initialized[4];
	void* SsHandle;
	mod_list_entry InLoadOrderModuleList;
	mod_list_entry InMemoryOrderModuleList;
	mod_list_entry InInitializationOrderModuleList;
};
struct mod_peb { std::uint8_t pad[0x18]; mod_peb_ldr* Ldr; };

// Remove our module from the PEB loader lists and erase the PE headers so that
// module enumeration and header scans find no identifiable image. The game has
// already resolved imports by the time this runs on the worker thread.
static void hide_module() noexcept
{
	__try {
		const auto peb{ reinterpret_cast<mod_peb*>(__readgsqword(0x60)) };
		const auto ldr{ peb->Ldr };
		for (auto e{ ldr->InLoadOrderModuleList.Flink }; e != &ldr->InLoadOrderModuleList; e = e->Flink) {
			const auto entry{ reinterpret_cast<mod_ldr_entry*>(e) };
			if (entry->DllBase != this_module)
				continue;

			for (mod_list_entry* links : { &entry->InLoadOrderLinks, &entry->InMemoryOrderLinks, &entry->InInitializationOrderLinks }) {
				links->Blink->Flink = links->Flink;
				links->Flink->Blink = links->Blink;
			}
			break;
		}

		DWORD old{};
		if (::VirtualProtect(this_module, 0x1000, PAGE_READWRITE, &old)) {
			::SecureZeroMemory(this_module, 0x1000);
			::VirtualProtect(this_module, 0x1000, old, &old);
		}
		module_hidden = true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// Hiding is best-effort; never crash the game over it.
	}
}

// Start the worker with a thread-start address inside ntdll (a legitimate
// module) instead of inside our own image, then redirect execution to DllAttach
// before the thread runs. Defeats naive thread-start-address range checks.
static void start_worker() noexcept
{
	const auto ntdll{ ::GetModuleHandleW(L"ntdll.dll") };
	const auto start{ reinterpret_cast<LPTHREAD_START_ROUTINE>(::GetProcAddress(ntdll, "RtlUserThreadStart")) };
	if (!start) {
		::_beginthreadex(nullptr, 0u, reinterpret_cast<_beginthreadex_proc_type>(DllAttach), nullptr, 0u, nullptr);
		return;
	}

	DWORD tid{};
	const HANDLE h{ ::CreateThread(nullptr, 0u, start, nullptr, CREATE_SUSPENDED, &tid) };
	if (!h) {
		::_beginthreadex(nullptr, 0u, reinterpret_cast<_beginthreadex_proc_type>(DllAttach), nullptr, 0u, nullptr);
		return;
	}

	CONTEXT ctx{};
	ctx.ContextFlags = CONTEXT_CONTROL;
	if (::GetThreadContext(h, &ctx)) {
		ctx.Rip = reinterpret_cast<DWORD64>(&DllAttach);
		ctx.Rcx = 0;
		::SetThreadContext(h, &ctx);
	}
	::ResumeThread(h);
	::CloseHandle(h);
}

__declspec(safebuffers) static void WINAPI DllAttach([[maybe_unused]] LPVOID lp) noexcept
{
	using namespace std::chrono_literals;

	// The dll is loaded at process start (hid.dll / DLL hijacking). Stay
	// completely idle while the game creates its renderer and the anti-cheat
	// unpacks its code: no allocations, no API calls, no memory scan.
	std::this_thread::sleep_for(5s);

	hide_module();

	if (is_disabled_by_user())
		exit_silently(_XorStr("[OFF] R3nzSkin.off found, mod disabled and unloaded"));

	std::this_thread::sleep_for(5s);

	std::setlocale(LC_ALL, ".utf8");
	cheatManager.start();
	if (HideThread(::GetCurrentThread()))
		cheatManager.logger->addLog(_XorStr("Thread Hidden!\n"));

	if (!cheatManager.memory->Search(true))
		exit_silently(_XorStr("[!!] game client not found, mod disabled and unloaded"));

	std::uint32_t empty_client_attempts{ 0 };
	while (true) {
		std::this_thread::sleep_for(1s);

		if (!cheatManager.memory->client) {
			if (!cheatManager.memory->Search(true))
				exit_silently(_XorStr("[!!] game client not found, mod disabled and unloaded"));

			if (!cheatManager.memory->client && ++empty_client_attempts > 60)
				exit_silently(_XorStr("[!!] game client never appeared, mod disabled and unloaded"));
		} else if (cheatManager.memory->client->get_game_state() == GGameState_s::Running)
			break;
	}
	cheatManager.logger->addLog(_XorStr("GameClient found!\n"));

	std::this_thread::sleep_for(500ms);
	if (!cheatManager.memory->Search(false))
		exit_silently(_XorStr("[!!] offsets not found, mod disabled and unloaded"));
	cheatManager.logger->addLog(_XorStr("All offsets found!\n"));
	std::this_thread::sleep_for(500ms);

	cheatManager.config->init();
	cheatManager.config->load();
	cheatManager.gui->is_open = cheatManager.config->isOpen;
	cheatManager.logger->addLog(_XorStr("CFG loaded!\n"));

	// Last safety check before taking over Present: without a swap chain/window
	// the overlay cannot work, and hooking would only risk the running game.
	if (!cheatManager.memory->swapChain || !cheatManager.memory->window)
		exit_silently(_XorStr("[!!] renderer not ready, mod disabled and unloaded"));

	cheatManager.hooks->install();

	while (cheatManager.cheatState) {
		// While the menu is closed the WndProc subclass is removed; poll hotkeys
		// here instead so quick-skin keys still work without a persistent hook.
		if (!cheatManager.gui->is_open)
			cheatManager.hooks->poll();
		std::this_thread::sleep_for(50ms);
	}

	::ExitProcess(0u);
}

__declspec(safebuffers) BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD reason, [[maybe_unused]] LPVOID reserved)
{
	DisableThreadLibraryCalls(hModule);

	if (reason != DLL_PROCESS_ATTACH)
		return TRUE;

	this_module = hModule;

	// Resolve real hid.dll exports so our proxy thunks forward correctly and the
	// export table matches the genuine system module.
	hid_forward_init();

	// Keep DllMain minimal: it runs under the loader lock while the game is
	// starting. All real work happens later on the worker thread.
	start_worker();
	::CloseHandle(hModule);
	return TRUE;
}
