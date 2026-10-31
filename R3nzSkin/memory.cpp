#pragma warning(disable: 28182 6011)

#include <Windows.h>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "CheatManager.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"

// Small file log so a failed signature search can be diagnosed even when the
// in-game menu never shows up.
static void log_search(const char* text, const char* detail = nullptr) noexcept
{
	char temp[MAX_PATH]{};
	if (::GetTempPathA(MAX_PATH, temp) == 0)
		return;

	std::ofstream out{ std::string{ temp } + "R3nzSkin.log", std::ios::app };
	if (out) {
		out << text;
		if (detail)
			out << detail;
		out << '\n';
	}
}

[[nodiscard]] static std::uint8_t* find_signature(const wchar_t* szModule, const char* szSignature) noexcept
{
	const auto module{ ::GetModuleHandleW(szModule) };
	if (!module)
		return nullptr;

	using bytes_t = std::vector<std::int32_t>;

	static const auto pattern_to_byte = [](const char* pattern) noexcept -> bytes_t
	{
		bytes_t bytes{};
		const auto start{ const_cast<char*>(pattern) };
		const auto end{ const_cast<char*>(pattern) + strlen(pattern) };

		for (auto current{ start }; current < end; ++current) {
			if (*current == '?') {
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			} else {
				bytes.push_back(strtoul(current, &current, 16));
			}
		}

		return bytes;
	};

	const auto dosHeader{ reinterpret_cast<PIMAGE_DOS_HEADER>(module) };
	const auto ntHeaders{ reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew) };
	const auto textSection{ IMAGE_FIRST_SECTION(ntHeaders) };

	// On the Chinese client the executable is packed, so the on-disk raw size of the
	// first section is much smaller than the virtual size it occupies once unpacked.
	// Scanning only SizeOfRawData silently misses every pattern past that point.
	const auto sectionSize{ textSection->Misc.VirtualSize > textSection->SizeOfRawData ? textSection->Misc.VirtualSize : textSection->SizeOfRawData };
	const auto patternBytes{ pattern_to_byte(szSignature) };
	const auto scanBytes{ reinterpret_cast<std::uint8_t*>(module) + textSection->VirtualAddress };

	const auto s{ patternBytes.size() };
	const auto d{ patternBytes.data() };

	if (s == 0 || sectionSize <= s)
		return nullptr;

	MEMORY_BASIC_INFORMATION mbi{ nullptr };
	const std::uint8_t* next_check_address{ nullptr };
	constexpr auto readable{ PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY };

	for (auto i{ 0ul }; i < sectionSize - s; ++i) {
		bool found{ true };
		for (auto j{ 0ul }; j < s; ++j) {
			const auto current_address{ scanBytes + i + j };
			if (current_address >= next_check_address) {
				if (!::VirtualQuery(current_address, &mbi, sizeof(mbi))) {
					found = false;
					break;
				}

				// Uncommitted, guard and non-readable pages must be skipped whole.
				// Reading them raises an access violation on the injected thread,
				// which would take the whole game process down while it is loading.
				if (mbi.State != MEM_COMMIT || !(mbi.Protect & readable) || (mbi.Protect & PAGE_GUARD)) {
					i += reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize - (reinterpret_cast<std::uintptr_t>(scanBytes) + i);
					i--;
					found = false;
					break;
				} else {
					next_check_address = static_cast<std::uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
				}
			}

			if (scanBytes[i + j] != d[j] && d[j] != -1) {
				found = false;
				break;
			}
		}

		if (found)
			return &scanBytes[i];
	}

	return nullptr;

}

void Memory::update(bool gameClient) noexcept
{
	if (gameClient) {
		this->client = *reinterpret_cast<GameClient**>(this->base + offsets::global::GameClient);
	} else {
		this->localPlayer = *reinterpret_cast<AIBaseCommon**>(this->base + offsets::global::Player);
		this->heroList = *reinterpret_cast<ManagerTemplate<AIHero>**>(this->base + offsets::global::ManagerTemplate_AIHero_);
		this->minionList = *reinterpret_cast<ManagerTemplate<AIMinionClient>**>(this->base + offsets::global::ManagerTemplate_AIMinionClient_);
		this->turretList = *reinterpret_cast<ManagerTemplate<AITurret>**>(this->base + offsets::global::ManagerTemplate_AITurret_);
		this->championManager = *reinterpret_cast<ChampionManager**>(this->base + offsets::global::ChampionManager);
		this->materialRegistry = reinterpret_cast<std::uintptr_t(__fastcall*)()>(this->base + offsets::functions::Riot__Renderer__MaterialRegistry__GetSingletonPtr)();
		this->swapChain = *reinterpret_cast<IDXGISwapChain**>(this->materialRegistry + offsets::MaterialRegistry::SwapChain);
		this->window = *reinterpret_cast<HWND*>(this->base + offsets::global::Riot__g_window);
		this->translateString = reinterpret_cast<translateString_t>(this->base + offsets::functions::translateString_UNSAFE_DONOTUSE);
	}
}

bool Memory::Search(bool gameClient)
{
	using namespace std::chrono_literals;

	// Fallback: never retry forever. If the signatures cannot be resolved in time
	// (game update moved them, protected pages, ...), give up and leave the game
	// completely alone instead of keeping a scanner running next to the match.
	constexpr auto maxAttempts{ 30u };

	try {
		this->base = reinterpret_cast<std::uintptr_t>(::GetModuleHandle(nullptr));
		const auto& signatureToSearch{ (gameClient ? this->gameClientSig : this->sigs) };

		for (const auto& sig : signatureToSearch)
			*sig.offset = 0;

		for (auto attempt{ 0u };; ++attempt) {
			bool missing_offset{ false };
			for (auto& sig : signatureToSearch) {

				if (*sig.offset != 0)
					continue;

				for (auto& pattern : sig.pattern) {
					auto address{ find_signature(nullptr, pattern.c_str()) };

					if (!address) {
						// Never show a modal dialog here: this runs on the thread that is
						// injected while the game is still starting, and a hidden message
						// box behind the fullscreen game looks exactly like a frozen game
						// that never gets into the match. Retry quietly instead.
						if (attempt < 3) {
							cheatManager.logger->addLog("[..] Pattern not found yet: %s\n", pattern.c_str());
							log_search("[..] pattern not found yet: ", pattern.c_str());
						}
						continue;
					}
					if (sig.offset == &offsets::global::GameClient) {
						if (address[21] >= 32)
							continue;
						std::memcpy(&offsets::encoding::GameClientAdd, address + 15, sizeof(std::uint32_t));
						offsets::encoding::GameClientRotate = address[21];
						std::memcpy(&offsets::encoding::GameClientXor, address + 24, sizeof(std::uint32_t));
					} else if (sig.offset == &offsets::AIBaseCommon::SkinId) {
						std::uint32_t field_offset{};
						std::memcpy(&field_offset, address + 33, sizeof(field_offset));
						if (!field_offset || address[2] >= 32 || address[23] >= 32)
							continue;
						offsets::encoding::SkinIdRotate1 = address[2];
						std::memcpy(&offsets::encoding::SkinIdXor, address + 9, sizeof(std::uint32_t));
						offsets::encoding::SkinIdRotate2 = address[23];
						*sig.offset = field_offset;
						break;
					}

					if (sig.read)
						address = *reinterpret_cast<std::uint8_t**>(address + (pattern.find_first_of('?') / 3));
					else if (sig.relative)
						address = address + *reinterpret_cast<std::uint32_t*>(address + 3) + 7;
					else if (address[0] == 0xE8)
						address = address + *reinterpret_cast<std::uint32_t*>(address + 1) + 5;

					if (sig.sub_base)
						address -= this->base;

					address += sig.additional;

					*sig.offset = reinterpret_cast<std::uint32_t>(address);
					// cheatManager.logger->addLog("Found: %s\n\tAddress: 0x%X\n", pattern.c_str(), *sig.offset);
					break;
				}

				if (!*sig.offset) {
					missing_offset = true;
					break;
				}
			}

			if (!missing_offset) {
				log_search(gameClient ? "[OK] game client offsets found" : "[OK] all offsets found");
				this->update(gameClient);
				return true;
			}

			if (attempt + 1 >= maxAttempts) {
				// Inert fallback: no hooks, no memory writes, the game keeps running.
				cheatManager.logger->addLog("[!!] Signatures not found, mod disabled for this game.\n");
				log_search("[!!] signatures not found, mod disabled for this game");
				return false;
			}

			std::this_thread::sleep_for(2s);
		}
	} catch (const std::exception& e) {
		cheatManager.logger->addLog("[!!] Memory search failed: %s\n", e.what());
		log_search("[!!] memory search failed: ", e.what());
		return false;
	}

	return false;
}
