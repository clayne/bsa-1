#include <cstdlib>
#include <iostream>
#include <regex>
#include <type_traits>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
#if 0
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim Special Edition\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Oblivion\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 3 goty\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Fallout New Vegas\\Data",
	};

	std::filesystem::path path;
	std::regex regex(".*.bsa", std::regex_constants::grep | std::regex_constants::icase);
	bsa::archive archive;
	std::ios_base::sync_with_stdio(false);

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
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data"
	};

	std::filesystem::path path;
	std::regex regex(".*.ba2", std::regex_constants::grep | std::regex_constants::icase);
	ba2::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : std::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive.read(sysEntry.path());
			for (auto& file : ba2::file_iterator(archive)) {
				std::cout << file.string() << '\n';
			}
			[[maybe_unused]] bool dummy = true;
		}
	}
#endif

	return EXIT_SUCCESS;
}
