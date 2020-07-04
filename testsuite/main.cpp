#define BSA_PRESERVE_PADDING

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
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

void parse_archives(nonstd::span<const filesystem::path> a_directories, std::function<void(const filesystem::path&)> a_functor)
{
	boost::regex regex{ ".*\\.bsa$", boost::regex_constants::grep | boost::regex_constants::icase };

	for (auto& dir : a_directories) {
		try {
			for (auto& sysEntry : filesystem::directory_iterator(dir)) {
				if (const auto& path = sysEntry.path(); boost::regex_match(path.string(), regex)) {
					a_functor(path);
				}
			}
		} catch (const filesystem::filesystem_error&) {}
	}
}

class tes3
{
public:
	static void parse()
	{
		parse_archives(PATHS, [](const filesystem::path& a_path) {
			archive_type archive;
			archive << a_path;
			for (auto& file : archive) {
				if (!archive.contains(file)) {
					assert(false);
				} else if (!archive.find(file.string())) {
					assert(false);
				}
				std::cout << file.string() << '\n';
			}
		});
	}

	static void write()
	{
		write_archives<archive_type>({ PATHS });
	}

	static void extract()
	{
		filesystem::path path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files\\Tribunal.bsa" };
		archive_type archive{ path };
		archive.extract("E:\\Repos\\bsa\\mytest");
	}

	static void repack()
	{
		archive_type archive;
		std::vector<file_type> files;
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

private:
	using archive_type = bsa::tes3::archive;
	using file_type = bsa::tes3::file;

	static inline const std::array PATHS{
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Morrowind\\Data Files" }
	};

	tes3() = delete;
};

class tes4
{
public:
	static void parse()
	{
		parse_archives(PATHS, [](const filesystem::path& a_path) {
			archive_type archive;
			archive << a_path;
			for (const auto& dir : archive) {
				std::cout << dir.string() << '\n';
				for (const auto& file : dir) {
					std::cout << '\t' << file.string() << '\n';
				}
			}
		});
	}

	static void write()
	{
		// Oblivion
		// * "Oblivion - Meshes.bsa", data block in non-standard order
		// Skyrim LE
		// * "HighResTexturePack02.bsa", lots of padding in file name block
		// * "Skyrim - Misc.bsa", data block in non-standard order

		write_archives<archive_type>({ PATHS });
	}

private:
	using archive_type = bsa::tes4::archive;

	static inline const std::array PATHS{
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Oblivion\\Data" },
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim\\Data" },
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Skyrim Special Edition\\Data" }
	};

	tes4() = delete;
};

class fo4
{
public:
	static void parse()
	{
		parse_archives(PATHS, [](const filesystem::path& a_path) {
			archive_type archive;
			archive.read(a_path);
			for (const auto& file : archive) {
				std::cout << file.string() << '\n';
			}
		});
	}

private:
	using archive_type = bsa::fo4::archive;

	static inline const std::array PATHS{
		filesystem::path{ "E:\\Games\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data" }
	};

	fo4() = delete;
};

int main(int, const char*[])
{
	std::ios_base::sync_with_stdio(false);
	stopwatch watch;
	watch.start();

	//tes3::extract();
	//tes3::repack();
	//tes3::write();
	//tes3::parse();

	tes4::parse();
	//tes4::write();

	//fo4::parse();

	watch.stamp<std::chrono::milliseconds>();

	return EXIT_SUCCESS;
}
