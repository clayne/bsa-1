#include <chrono>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <type_traits>

#include "bsa/bsa.hpp"


namespace stl
{
	using namespace bsa::stl;

	namespace filesystem
	{
		using namespace bsa::stl::filesystem;

#if __cplusplus >= 201703L
		using std::filesystem::directory_iterator;
		using std::filesystem::file_size;
		using std::filesystem::recursive_directory_iterator;
		using std::filesystem::relative;

		inline bool is_regular_file(const std::filesystem::directory_entry& a_entry)
		{
			return a_entry.is_regular_file();
		}
#else
		using boost::filesystem::directory_iterator;
		using boost::filesystem::file_size;
		using boost::filesystem::recursive_directory_iterator;
		using boost::filesystem::relative;

		inline bool is_regular_file(const boost::filesystem::directory_entry& a_entry)
		{
			return a_entry.status().type() == boost::filesystem::file_type::regular_file;
		}
#endif
	}
}


class stopwatch
{
private:
	template <class>
	struct is_duration :
		std::false_type
	{};

	template <class Rep, class Period>
	struct is_duration<std::chrono::duration<Rep, Period>> :
		std::true_type
	{};

public:
	void start()
	{
		_start = clock_t::now();
	}

	template <
		class Duration,
		std::enable_if_t<
			is_duration<Duration>::value,
			int> = 0>
	void stamp()
	{
		auto end = clock_t::now();
		auto diff = std::chrono::duration_cast<Duration>(end - _start);
		std::cout << "Stamp: " << diff.count() << '\n';
	}

private:
	using clock_t = std::chrono::high_resolution_clock;

	std::chrono::time_point<clock_t> _start;
};


void extract_tes3()
{
	stl::filesystem::path path("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
	bsa::tes3::archive archive(path);
	archive.extract("E:\\Repos\\bsa\\mytest");
}


void compare_tes3(stl::filesystem::path a_lhsP, stl::filesystem::path a_rhsP)
{
	if (stl::filesystem::file_size(a_lhsP) != stl::filesystem::file_size(a_rhsP)) {
		assert(false);
	}

	constexpr auto FLAGS = std::ios_base::in | std::ios_base::binary;
	std::ifstream lhsF(a_lhsP.native(), FLAGS);
	std::ifstream rhsF(a_rhsP.native(), FLAGS);

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
	bsa::tes3::archive archive("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
	archive << "E:\\Repos\\bsa\\mytest.bsa";
	compare_tes3("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa", "E:\\Repos\\bsa\\mytest.bsa");
}


void repack_tes3()
{
	bsa::tes3::archive archive;
	std::vector<bsa::tes3::file> files;
	stl::filesystem::path root = "E:\\Repos\\bsa\\mytest";
	for (auto& dirEntry : stl::filesystem::recursive_directory_iterator(root)) {
		if (stl::filesystem::is_regular_file(dirEntry)) {
			files.emplace_back(stl::filesystem::relative(dirEntry.path(), root).string(), dirEntry.path());
		}
	}

	archive.insert(files.begin(), files.end());
	archive << "E:\\Repos\\bsa\\mytest.bsa";
	compare_tes3("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa", "E:\\Repos\\bsa\\mytest.bsa");
}


void parse_tes3()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files"
	};

	stl::filesystem::path path;
	std::regex regex(".*.bsa$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::tes3::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
			if (!std::regex_match(sysEntry.path().string(), regex)) {
				continue;
			}

			archive >> sysEntry.path();
			for (auto& file : archive) {
				if (!archive.contains(file)) {
					assert(false);
				} else if (!archive.find(file.string())) {
					assert(false);
				}
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

	stl::filesystem::path path;
	std::regex regex(".*.bsa$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::tes4::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
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

	stl::filesystem::path path;
	std::regex regex(".*.ba2$", std::regex_constants::grep | std::regex_constants::icase);
	bsa::fo4::archive archive;
	std::ios_base::sync_with_stdio(false);

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
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


int main(int, const char*[])
{
	stopwatch watch;
	watch.start();

	//extract_tes3();
	//repack_tes3();
	//write_tes3();
	//parse_tes3();

	//parse_tes4();

	//parse_fo4();

	bsa::sse::archive archive;
	archive >> R"(E:\Games\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Skyrim - Textures8.bsa)";

#if 0
	{
		bsa::tes3::archive src("E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa");
		bsa::tes3::archive dst;
		std::vector<bsa::tes3::file> files;
		if (!src.empty()) {
			for (auto& file : src) {
				files.push_back(file);
			}

			//files.assign(20, src.front());
			dst.insert(files.begin(), files.end());
			for (auto& file : files) {
				dst >> file;
			}
			dst.erase(files.front());
			//files.front().extract_to("E:\\Repos\\bsa\\mytest");
		}
		[[maybe_unused]] bool dummy = true;
	}
#endif

	watch.stamp<std::chrono::milliseconds>();

	return EXIT_SUCCESS;
}
