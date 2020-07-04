#define BSA_PRESERVE_PADDING

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <streambuf>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/regex.hpp>

#include "bsa/bsa.hpp"
#include "mstream.hpp"

namespace filesystem = boost::filesystem;

enum class color
{
	red,
	green
};

namespace util
{
	template <class... Args>
	void print(color a_color, Args&&... a_args)
	{
		std::stringstream ss;
		switch (a_color) {
		case color::red:
			ss << "\x1B[31m";
			break;
		case color::green:
			ss << "\x1B[32m";
			break;
		default:
			assert(false);
			return;
		}

		((ss << std::forward<Args>(a_args)), ...);
		ss << "\033[0m";
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

void compare_files(const boost::iostreams::mapped_file_source& a_lhs, nonstd::span<const char> a_rhs)
{
	if (a_lhs.size() != a_rhs.size()) {
		util::print(color::red, "FAIL (size: ", a_lhs.size(), " != size: ", a_rhs.size(), ')');
		return;
	}

	if (std::memcmp(a_lhs.data(), a_rhs.data(), a_rhs.size()) != 0) {
		for (std::size_t i = 0; i < a_rhs.size(); ++i) {
			if (a_lhs.data()[i] != a_rhs.data()[i]) {
				util::print(color::red, "FAIL (at pos ", i, ')');
				return;
			}
		}
	}

	util::print(color::green, "PASS");
}

template <class Archive>
void write_archives(nonstd::span<const filesystem::path> a_directories)
{
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	Archive archive;

	for (auto& dir : a_directories) {
		try {
			for (auto& sysEntry : filesystem::directory_iterator(dir)) {
				if (boost::regex_match(sysEntry.path().string(), regex)) {
					const auto& path = sysEntry.path();
					boost::iostreams::mapped_file_source src{ path };
					archive << path;

					omemorystream os(archive.size_bytes());
					archive >> os;

					std::cout << path << ' ';
					compare_files(src, os.span());
					std::cout << std::endl;
				}
			}
		} catch (const filesystem::filesystem_error&) {}
	}
}

void extract_tes3()
{
	filesystem::path path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
	bsa::tes3::archive archive{ path };
	archive.extract("E:\\Repos\\bsa\\mytest");
}

void write_tes3()
{
	const std::array PATHS{
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files" }
	};

	write_archives<bsa::tes3::archive>({ PATHS });
}

void repack_tes3()
{
	bsa::tes3::archive archive;
	std::vector<bsa::tes3::file> files;
	filesystem::path root{ "E:\\Repos\\bsa\\mytest" };
	for (auto& dirEntry : filesystem::recursive_directory_iterator(root)) {
		if (filesystem::is_regular_file(dirEntry)) {
			files.emplace_back(filesystem::relative(dirEntry.path(), root).string(), dirEntry.path());
		}
	}

	archive.insert(files.begin(), files.end());

	filesystem::path path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
	boost::iostreams::mapped_file_source src{ path };

	omemorystream os(archive.size_bytes());
	archive >> os;

	compare_files(src, os.span());
}

void parse_tes3()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files"
	};

	filesystem::path path;
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::tes3::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		try {
			for (auto& sysEntry : filesystem::directory_iterator(path)) {
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
		} catch (const filesystem::filesystem_error&) {}
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

	filesystem::path path;
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::tes4::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		try {
			for (auto& sysEntry : filesystem::directory_iterator(path)) {
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
		} catch (const filesystem::filesystem_error&) {}
	}
}

void write_tes4()
{
	// Oblivion
	// * "Oblivion - Meshes.bsa", data block in non-standard order
	// Skyrim LE
	// * "HighResTexturePack02.bsa", lots of padding in file name block
	// * "Skyrim - Misc.bsa", data block in non-standard order
	const std::array PATHS{
		//filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Oblivion\\Data" },
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim\\Data" },
		//filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim Special Edition\\Data" }
	};

	write_archives<bsa::tes4::archive>({ PATHS });
}

void parse_fo4()
{
	constexpr const char* PATHS[] = {
		"E:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data"
	};

	filesystem::path path;
	boost::regex regex{ ".*\\.ba2$", boost::regex_constants::grep | boost::regex_constants::icase };
	bsa::fo4::archive archive;

	for (std::size_t i = 0; i < std::extent_v<decltype(PATHS)>; ++i) {
		path = PATHS[i];

		for (auto& sysEntry : filesystem::directory_iterator(path)) {
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

	watch.stamp<std::chrono::milliseconds>();

	return EXIT_SUCCESS;
}
