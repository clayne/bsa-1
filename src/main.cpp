#include <cstdlib>
#include <iostream>
#include <regex>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
#if 0
	std::filesystem::path path(R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data)");
	std::regex regex(".*.bsa", std::regex_constants::grep | std::regex_constants::icase);
	bsa::archive archive;

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
#else
	[[maybe_unused]] bsa::archive swap(R"(swappedtest.bsa)");
	[[maybe_unused]] bsa::archive noswap(R"(test.bsa)");
#endif

	return EXIT_SUCCESS;
}
