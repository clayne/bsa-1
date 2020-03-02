#include <chrono>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <type_traits>

#include "bsa.hpp"


class StopWatch
{
public:
	void start()
	{
		_start = std::chrono::high_resolution_clock::now();
	}

	void time_stamp()
	{
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - _start;
		std::cout << "Milestone: " << diff.count() << '\n';
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};


void extract_tes3()
{
	std::filesystem::path path("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
	bsa::tes3::archive archive(path);
	archive.extract("E:\\Repos\\bsa\\mytest");
}


void compare_tes3(std::filesystem::path a_lhsP, std::filesystem::path a_rhsP)
{
	if (std::filesystem::file_size(a_lhsP) != std::filesystem::file_size(a_rhsP)) {
		assert(false);
	}

	constexpr auto FLAGS = std::ios_base::in | std::ios_base::binary;
	std::ifstream lhsF(a_lhsP, FLAGS);
	std::ifstream rhsF(a_rhsP, FLAGS);

	do {
		if (lhsF.get() != rhsF.get()) {
			char lhsC;
			lhsF.seekg(-1, std::ios_base::cur);
			auto lhsPos = lhsF.tellg();
			lhsF.get(lhsC);

			char rhsC;
			rhsF.seekg(-1, std::ios_base::cur);
			auto rhsPos = rhsF.tellg();
			rhsF.get(rhsC);

			std::cout << "Comparison failure!\n";
			assert(false);
			return;
		}
	} while (lhsF && rhsF);

	if (!lhsF.eof() || !rhsF.eof()) {
		assert(false);
	}
}


void write_tes3()
{
	{
		bsa::tes3::archive archive("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
		archive.write("E:\\Repos\\bsa\\mytest.bsa");
	}

	compare_tes3("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa", "E:\\Repos\\bsa\\mytest.bsa");
}


void repack_tes3()
{
	bsa::tes3::archive archive;
	std::vector<bsa::tes3::file> files;
	std::filesystem::path root = "E:\\Repos\\bsa\\mytest";
	for (auto& dirEntry : std::filesystem::recursive_directory_iterator(root)) {
		if (dirEntry.is_regular_file()) {
			files.emplace_back(std::filesystem::relative(dirEntry.path(), root).string(), dirEntry.path());
		}
	}

	archive.insert(files.begin(), files.end());
	archive.write("E:\\Repos\\bsa\\mytest.bsa");
	compare_tes3("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa", "E:\\Repos\\bsa\\mytest.bsa");
}


void parse_tes3()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files"
	};

	std::filesystem::path path;
	std::regex regex(".*.bsa$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::tes3::archive archive;
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
	StopWatch watch;
	watch.start();

	//extract_tes3();
	repack_tes3();
	//write_tes3();
	//parse_tes3();

	//parse_tes4();

	//parse_fo4();

	watch.time_stamp();

	return EXIT_SUCCESS;
}
