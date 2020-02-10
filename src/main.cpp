#include <cstdlib>
#include <iostream>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
	bsa::archive archive(R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Skyrim - Patch.bsa)");
	//archive.dump();

	for ([[maybe_unused]] auto& dir : bsa::directory_iterator(archive)) {
		std::cout << dir.string() << '\n';
		[[maybe_unused]] bool dummy = true;
	}

	return EXIT_SUCCESS;
}
