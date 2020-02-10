#include <cstdlib>
#include <iostream>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
	bsa::archive archive(R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Skyrim - Patch.bsa)");
	//archive.dump();

	for (auto& dir : bsa::directory_iterator(archive)) {
		std::cout << dir.string() << '\n';
		for (auto& file : bsa::file_iterator(dir)) {
			std::cout << '\t' << file.string() << '\n';
		}
	}

	return EXIT_SUCCESS;
}
