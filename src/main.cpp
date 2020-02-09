#include <cstdlib>

#include "bsa.hpp"


int main([[maybe_unused]] int a_argc, [[maybe_unused]] const char* a_argv[])
{
	bsa::archive archive(R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Skyrim - Patch.bsa)");
	archive.parse();
	//archive.dump();

	return EXIT_SUCCESS;
}
