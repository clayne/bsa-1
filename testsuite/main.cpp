#include <cstdlib>
#include <iostream>
#include <regex>
#include <type_traits>

#include "bsa.hpp"


void extract_tes3()
{
	std::filesystem::path path("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
	bsa::tes3::archive archive(path);
	archive.extract("E:\\Repos\\bsa\\mytest");
}


void write_tes3()
{
	{
		std::filesystem::path path("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
		bsa::tes3::archive archive(path);
		archive.write("E:\\Repos\\bsa\\mytest.bsa");
	}

	std::filesystem::path originalPath = "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa";
	std::filesystem::path newPath = "E:\\Repos\\bsa\\mytest.bsa";
	if (std::filesystem::file_size(originalPath) != std::filesystem::file_size(newPath)) {
		assert(false);
	}

	constexpr auto FLAGS = std::ios_base::in | std::ios_base::binary;
	std::ifstream originalFile(originalPath, FLAGS);
	std::ifstream newFile(newPath, FLAGS);

	char originalChar;
	char newChar;
	do {
		originalFile.get(originalChar);
		newFile.get(newChar);
		if (originalChar != newChar) {
			assert(false);
		}
	} while (originalFile && newFile);

	if (!originalFile.eof() || !newFile.eof()) {
		assert(false);
	}
}


void parse_tes3()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files"
	};

	std::filesystem::path path;
	std::regex regex(".*.bsa$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::tes3::archive_view archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : std::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive.read(sysEntry.path());
			for (auto& file : archive) {
				std::cout << file.string() << '\n';
			}
		}
	}
}


void parse_tes4()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim Special Edition\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Oblivion\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 3 goty\\Data",
		"D:\\Games\\SteamLibrary\\steamapps\\common\\Fallout New Vegas\\Data",
	};

	std::filesystem::path path;
	std::regex regex(".*.bsa$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::tes4::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : std::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive.read(sysEntry.path());
			for (auto& dir : archive) {
				std::cout << dir.string() << '\n';
				for (auto& file : dir) {
					std::cout << '\t' << file.string() << '\n';
				}
			}
		}
	}
}


void parse_fo4()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data"
	};

	std::filesystem::path path;
	std::regex regex(".*.ba2$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::fo4::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : std::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive.read(sysEntry.path());
			for (auto& file : archive) {
				std::cout << file.string() << '\n';
			}
		}
	}
}


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
	//extract_tes3();
	write_tes3();
	//parse_tes3();

	//parse_tes4();

	//parse_fo4();

	return EXIT_SUCCESS;
}
