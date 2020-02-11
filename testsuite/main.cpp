#include <cstdlib>
#include <iostream>
#include <regex>
#include <type_traits>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
#if 1
	constexpr const char* PATHS[] = {
		R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data)",
		R"(D:\Games\SteamLibrary\steamapps\common\Skyrim\Data)",
		R"(D:\Games\SteamLibrary\steamapps\common\Oblivion\Data)",
		R"(D:\Games\SteamLibrary\steamapps\common\Fallout 3 goty\Data)",
		R"(D:\Games\SteamLibrary\steamapps\common\Fallout New Vegas\Data)",
	};

	std::filesystem::path path;
	std::regex regex(".*.bsa", std::regex_constants::grep | std::regex_constants::icase);
	bsa::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : std::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive.read(sysEntry.path());
			for (auto& dir : bsa::directory_iterator(archive)) {
				std::cout << dir.string() << '\n';
				for (auto& file : bsa::file_iterator(dir)) {
					std::cout << '\t' << file.string() << '\n';
				}
			}
		}
	}
#else
	[[maybe_unused]] bsa::archive swap(R"(swappedtest.bsa)");
	[[maybe_unused]] bsa::archive noswap(R"(test.bsa)");
#endif

	return EXIT_SUCCESS;
}
