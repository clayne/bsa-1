#define BSA_PRESERVE_PADDING

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <type_traits>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/regex.hpp>

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
		using std::filesystem::filesystem_error;
		using std::filesystem::recursive_directory_iterator;
		using std::filesystem::relative;

		inline bool is_regular_file(const std::filesystem::directory_entry& a_entry)
		{
			return a_entry.is_regular_file();
		}
#else
		using boost::filesystem::directory_iterator;
		using boost::filesystem::file_size;
		using boost::filesystem::filesystem_error;
		using boost::filesystem::recursive_directory_iterator;
		using boost::filesystem::relative;

		inline bool is_regular_file(const boost::filesystem::directory_entry& a_entry)
		{
			return a_entry.status().type() == boost::filesystem::file_type::regular_file;
		}
#endif
	}
}

enum class color
{
	red,
	green
};

namespace util
{
	template <class... Args>
	void print(color a_color, Args... a_args)
	{
		constexpr const char SGR[] = "\033[0m";
		constexpr const char RED[] = "\x1B[31m";
		constexpr const char GREEN[] = "\x1B[32m";

		std::stringstream ss;
		switch (a_color) {
		case color::red:
			ss << RED;
			break;
		case color::green:
			ss << GREEN;
			break;
		default:
			assert(false);
			return;
		}

		((ss << a_args), ...);
		ss << SGR;
		std::cout << ss.str();
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

void compare_files(stl::filesystem::path a_lhsP, stl::filesystem::path a_rhsP)
{
	boost::iostreams::mapped_file_source lhsF{ a_lhsP.string() };
	boost::iostreams::mapped_file_source rhsF{ a_rhsP.string() };

	if (lhsF.size() != rhsF.size()) {
		util::print(color::red, "FAIL (size: ", lhsF.size(), " != size: ", rhsF.size(), ")\n");
		return;
	}

	if (std::memcmp(lhsF.data(), rhsF.data(), lhsF.size()) != 0) {
		for (std::size_t i = 0; i < lhsF.size(); ++i) {
			if (lhsF.data()[i] != rhsF.data()[i]) {
				util::print(color::red, "FAIL (at pos ", i, ")\n");
				return;
			}
		}
	}

	util::print(color::green, "PASS\n");
}

void extract_tes3()
{
	stl::filesystem::path path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
	bsa::tes3::archive archive{ path };
	archive.extract("E:\\Repos\\bsa\\mytest");
}

void write_tes3()
{
	stl::filesystem::path lhsP{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
	stl::filesystem::path rhsP{ "E:\\Repos\\bsa\\mytest.bsa" };

	bsa::tes3::archive archive{ lhsP };
	archive >> rhsP;
	compare_files(lhsP, rhsP);
}

void repack_tes3()
{
	stl::filesystem::path lhsP{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
	stl::filesystem::path rhsP{ "E:\\Repos\\bsa\\mytest.bsa" };

	bsa::tes3::archive archive;
	std::vector<bsa::tes3::file> files;
	stl::filesystem::path root{ "E:\\Repos\\bsa\\mytest" };
	for (auto& dirEntry : stl::filesystem::recursive_directory_iterator(root)) {
		if (stl::filesystem::is_regular_file(dirEntry)) {
			files.emplace_back(stl::filesystem::relative(dirEntry.path(), root).string(), dirEntry.path());
		}
	}

	archive.insert(files.begin(), files.end());
	archive >> rhsP;
	compare_files(lhsP, rhsP);
}

void parse_tes3()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files"
	};

	stl::filesystem::path path;
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::tes3::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		try {
			for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
				if (!boost::regex_match(sysEntry.path().string(), regex)) {
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
		} catch (const stl::filesystem::filesystem_error&) {}
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
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::tes4::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		try {
			for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
				if (!boost::regex_match(sysEntry.path().string(), regex)) {
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
		} catch (const stl::filesystem::filesystem_error&) {}
	}
}

void write_tes4()
{
	// ignore "Oblivion - Meshes.bsa", valid but non-standard pack
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Oblivion\\Data"
	};

	stl::filesystem::path rhsP{ "E:\\Repos\\bsa\\mytest.bsa" };
	stl::filesystem::path path;
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::tes4::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		try {
			for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
				if (!boost::regex_match(sysEntry.path().string(), regex)) {
					continue;
				}

				const auto& lhsP = sysEntry.path();
				archive << lhsP;
				archive >> rhsP;
				std::cout << lhsP << ' ';
				compare_files(lhsP, rhsP);
			}
		} catch (const stl::filesystem::filesystem_error&) {}
	}
}

void parse_fo4()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data"
	};

	stl::filesystem::path path;
	boost::regex regex{ ".*\\.ba2$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::fo4::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : stl::filesystem::directory_iterator(path)) {
			if (!boost::regex_match(sysEntry.path().string(), regex)) {
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
	std::ios_base::sync_with_stdio(false);
	stopwatch watch;
	watch.start();

	//extract_tes3();
	//repack_tes3();
	//write_tes3();
	//parse_tes3();

	//parse_tes4();
	write_tes4();

	//parse_fo4();

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
