#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <iostream>


namespace bsa
{
	namespace detail
	{
		enum class archive_flags : std::uint32_t	// BSArchive::ARCHIVE_FLAGS
		{
			dir_strings = 1 << 0,
			file_strings = 1 << 1,
			compressed = 1 << 2,
			retain_directory_names = 1 << 3,
			retain_file_names = 1 << 4,
			retain_file_name_offsets = 1 << 5,
			xbox_archive = 1 << 6,
			retain_strings_during_startup = 1 << 7,
			embedded_file_names = 1 << 8,
			xbox_compressed = 1 << 9
		};


		enum class archive_type : std::uint16_t	// ARCHIVE_TYPE_INDEX
		{
			meshes = 1 << 0,
			textures = 1 << 1,
			menus = 1 << 2,
			sounds = 1 << 3,
			voices = 1 << 4,
			shaders = 1 << 5,
			trees = 1 << 6,
			fonts = 1 << 7,
			misc = 1 << 8
		};


		template <class Enum>
		constexpr decltype(auto) to_underlying(const Enum& a_enum) noexcept
		{
			return static_cast<std::underlying_type_t<Enum>>(a_enum);
		}


		template <class Enum>
		constexpr decltype(auto) to_underlying(Enum&& a_enum) noexcept
		{
			return static_cast<std::underlying_type_t<Enum>>(a_enum);
		}


		constexpr char mapchar(char a_ch) noexcept
		{
			switch (a_ch) {
			case '/':
				return '\\';
			case 'A':
				return 'a';
			case 'B':
				return 'b';
			case 'C':
				return 'c';
			case 'D':
				return 'd';
			case 'E':
				return 'e';
			case 'F':
				return 'f';
			case 'G':
				return 'g';
			case 'H':
				return 'h';
			case 'I':
				return 'i';
			case 'J':
				return 'j';
			case 'K':
				return 'k';
			case 'L':
				return 'l';
			case 'M':
				return 'm';
			case 'N':
				return 'n';
			case 'O':
				return 'o';
			case 'P':
				return 'p';
			case 'Q':
				return 'q';
			case 'R':
				return 'r';
			case 'S':
				return 's';
			case 'T':
				return 't';
			case 'U':
				return 'u';
			case 'V':
				return 'v';
			case 'W':
				return 'w';
			case 'X':
				return 'x';
			case 'Y':
				return 'y';
			case 'Z':
				return 'z';
			default:
				return a_ch;
			}
		}


		class header_t
		{
		public:
			constexpr header_t() noexcept : _block() {}

			constexpr bool dir_strings() const noexcept { return (_block.flags & to_underlying(archive_flags::dir_strings)) != 0; }
			constexpr decltype(auto) dir_names_length() const noexcept { return _block.directoryNamesLength; }
			constexpr decltype(auto) file_names_length() const noexcept { return _block.fileNamesLength; }
			constexpr bool file_strings() const noexcept { return (_block.flags & to_underlying(archive_flags::file_strings)) != 0; }
			constexpr decltype(auto) header_size() const noexcept { return _block.headerSize; }
			constexpr decltype(auto) num_directories() const noexcept { return _block.numDirectories; }
			constexpr decltype(auto) num_files() const noexcept { return _block.numFiles; }

			inline bool read(std::fstream& a_file)
			{
				if (!a_file.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
					return false;
				}

				return true;
			}

		private:
			struct block_t	// BSArchiveHeader
			{
				constexpr block_t() noexcept :
					tag{ '\0' },
					version(0),
					headerSize(0),
					flags(0),
					numDirectories(0),
					numFiles(0),
					directoryNamesLength(0),
					fileNamesLength(0),
					archiveType(0)
				{}

				char tag[4];
				std::uint32_t version;
				std::uint32_t headerSize;
				std::uint32_t flags;
				std::uint32_t numDirectories;
				std::uint32_t numFiles;
				std::uint32_t directoryNamesLength;
				std::uint32_t fileNamesLength;
				std::uint16_t archiveType;
			};

			block_t _block;
		};


		struct hash_t	// BSHash
		{
			constexpr hash_t() noexcept :
				last(0),
				last2(0),
				length(0),
				first(0),
				crc(0)
			{}

			constexpr hash_t(const hash_t& a_rhs) noexcept :
				last(a_rhs.last),
				last2(a_rhs.last2),
				length(a_rhs.length),
				first(a_rhs.first),
				crc(a_rhs.crc)
			{}

			constexpr hash_t(hash_t&& a_rhs) noexcept :
				last(std::move(a_rhs.last)),
				last2(std::move(a_rhs.last2)),
				length(std::move(a_rhs.length)),
				first(std::move(a_rhs.first)),
				crc(std::move(a_rhs.crc))
			{}

			constexpr hash_t& operator=(const hash_t& a_rhs) noexcept
			{
				if (this == &a_rhs) {
					return *this;
				}

				last = a_rhs.last;
				last2 = a_rhs.last2;
				length = a_rhs.length;
				first = a_rhs.first;
				crc = a_rhs.crc;

				return *this;
			}

			constexpr hash_t& operator=(hash_t&& a_rhs) noexcept
			{
				if (this == &a_rhs) {
					return *this;
				}

				last = std::move(a_rhs.last);
				last2 = std::move(a_rhs.last2);
				length = std::move(a_rhs.length);
				first = std::move(a_rhs.first);
				crc = std::move(a_rhs.crc);

				return *this;
			}

			constexpr bool operator==(const hash_t& a_rhs) const noexcept
			{
				return last == a_rhs.last &&
					last2 == a_rhs.last2 &&
					length == a_rhs.length &&
					first == a_rhs.first &&
					crc == a_rhs.crc;
			}

			constexpr bool operator!=(const hash_t& a_rhs) const noexcept { return !(*this == a_rhs); }


			std::int8_t last;
			std::int8_t last2;
			std::int8_t length;
			std::int8_t first;
			std::int32_t crc;
		};


		class file_t
		{
		public:
			inline file_t() noexcept :
				_block(),
				_name()
			{}

			inline file_t(const file_t& a_rhs) :
				_block(a_rhs._block),
				_name(a_rhs._name)
			{}

			inline file_t(file_t&& a_rhs) noexcept :
				_block(std::move(a_rhs._block)),
				_name(std::move(a_rhs._name))
			{}

			inline file_t& operator=(const file_t& a_rhs)
			{
				if (this == &a_rhs) {
					return *this;
				}

				_block = a_rhs._block;
				_name = a_rhs._name;

				return *this;
			}

			inline file_t& operator=(file_t&& a_rhs) noexcept
			{
				if (this == &a_rhs) {
					return *this;
				}

				_block = std::move(a_rhs._block);
				_name = std::move(a_rhs._name);

				return *this;
			}

			static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

			inline decltype(auto) c_str() const noexcept { return _name.c_str(); }

			constexpr hash_t hash() noexcept { return _block.hash; }
			constexpr const hash_t& hash() const noexcept { return _block.hash; }

			inline std::string str() { return _name; }
			inline const std::string& str() const noexcept { return _name; }

			inline bool read(std::fstream& a_file, [[maybe_unused]] const header_t& a_header)
			{
				if (!a_file.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
					return false;
				}

				return true;
			}

			inline bool read_name(std::fstream& a_file)
			{
				char ch;
				do {
					a_file.get(ch);
					_name.push_back(ch);
				} while (ch != '\0' && a_file);
				_name.pop_back();
				return !a_file.fail();
			}

		private:
			struct block_t	// BSFileEntry
			{
				constexpr block_t() noexcept :
					hash(),
					size(0),
					offset(0)
				{}

				constexpr block_t(const block_t& a_rhs) noexcept :
					hash(a_rhs.hash),
					size(a_rhs.size),
					offset(a_rhs.offset)
				{}

				constexpr block_t(block_t&& a_rhs) noexcept :
					hash(std::move(a_rhs.hash)),
					size(std::move(a_rhs.size)),
					offset(std::move(a_rhs.offset))
				{}

				constexpr block_t& operator=(const block_t& a_rhs) noexcept
				{
					if (this == &a_rhs) {
						return *this;
					}

					hash = a_rhs.hash;
					size = a_rhs.size;
					offset = a_rhs.offset;

					return *this;
				}

				constexpr block_t& operator=(block_t&& a_rhs) noexcept
				{
					if (this == &a_rhs) {
						return *this;
					}

					hash = std::move(a_rhs.hash);
					size = std::move(a_rhs.size);
					offset = std::move(a_rhs.offset);

					return *this;
				}

				hash_t hash;
				std::uint32_t size;
				std::uint32_t offset;
			};

			block_t _block;
			std::string _name;
		};


		class directory_t
		{
		public:
			inline directory_t() noexcept :
				_block(),
				_name(),
				_files()
			{}

			inline directory_t(const directory_t& a_rhs) :
				_block(a_rhs._block),
				_name(a_rhs._name),
				_files(a_rhs._files)
			{}

			inline directory_t(directory_t&& a_rhs) noexcept :
				_block(std::move(a_rhs._block)),
				_name(std::move(a_rhs._name)),
				_files(std::move(a_rhs._files))
			{}

			inline directory_t& operator=(const directory_t& a_rhs)
			{
				if (this == &a_rhs) {
					return *this;
				}

				_block = a_rhs._block;
				_name = a_rhs._name;
				_files = a_rhs._files;

				return *this;
			}

			inline directory_t& operator=(directory_t&& a_rhs) noexcept
			{
				if (this == &a_rhs) {
					return *this;
				}

				_block = std::move(a_rhs._block);
				_name = std::move(a_rhs._name);
				_files = std::move(a_rhs._files);

				return *this;
			}

			inline decltype(auto) c_str() const noexcept { return _name.c_str(); }

			constexpr decltype(auto) files_offset() const noexcept { return _block.filesOffset; }

			constexpr hash_t hash() noexcept { return _block.hash; }
			constexpr const hash_t& hash() const noexcept { return _block.hash; }

			constexpr decltype(auto) num_files() const noexcept { return _block.numFiles; }

			inline std::string str() { return _name; }
			inline const std::string& str() const noexcept { return _name; }

			inline void for_each_file(std::function<void(file_t&)> a_fn)
			{
				for (auto& file : _files) {
					a_fn(file);
				}
			}

			inline void for_each_file(std::function<void(const file_t&)> a_fn) const
			{
				for (auto& file : _files) {
					a_fn(file);
				}
			}

			inline bool read(std::fstream& a_file, const header_t& a_header)
			{
				if (!a_file.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
					return false;
				}

				if (!a_header.dir_strings() && num_files() == 0) {
					return true;
				}

				auto pos = a_file.tellg();
				a_file.seekg(files_offset() - a_header.file_names_length(), std::ios_base::beg);

				if (a_header.dir_strings()) {
					std::int8_t length;
					if (!a_file.read(reinterpret_cast<char*>(&length), sizeof(length))) {
						return false;
					}

					_name.resize(static_cast<std::size_t>(length) - 1);
					if (!a_file.read(_name.data(), length)) {
						return false;
					}
				}

				for (std::uint32_t i = 0; i < num_files(); ++i) {
					file_t file;
					if (!file.read(a_file, a_header)) {
						return false;
					}
					_files.push_back(std::move(file));
				}

				a_file.seekg(pos, std::ios_base::beg);

				return true;
			}

		private:
			struct block_t	// BSDirectoryEntry
			{
				constexpr block_t() noexcept :
					hash(),
					numFiles(0),
					pad(0),
					filesOffset(0)
				{}

				constexpr block_t(const block_t& a_rhs) noexcept :
					hash(a_rhs.hash),
					numFiles(a_rhs.numFiles),
					pad(a_rhs.pad),
					filesOffset(a_rhs.filesOffset)
				{}

				constexpr block_t(block_t&& a_rhs) noexcept :
					hash(std::move(a_rhs.hash)),
					numFiles(std::move(a_rhs.numFiles)),
					pad(std::move(a_rhs.pad)),
					filesOffset(std::move(a_rhs.filesOffset))
				{}

				constexpr block_t& operator=(const block_t& a_rhs) noexcept
				{
					if (this == &a_rhs) {
						return *this;
					}

					hash = a_rhs.hash;
					numFiles = a_rhs.numFiles;
					pad = a_rhs.pad;
					filesOffset = a_rhs.filesOffset;

					return *this;
				}

				constexpr block_t& operator=(block_t&& a_rhs) noexcept
				{
					if (this == &a_rhs) {
						return *this;
					}

					hash = std::move(a_rhs.hash);
					numFiles = std::move(a_rhs.numFiles);
					pad = std::move(a_rhs.pad);
					filesOffset = std::move(a_rhs.filesOffset);

					return *this;
				}

				hash_t hash;
				std::uint32_t numFiles;
				std::uint32_t pad;
				std::uint64_t filesOffset;
			};

			block_t _block;
			std::string _name;
			std::vector<file_t> _files;
		};


		class dir_hasher
		{
		public:
			inline hash_t operator()(std::string a_path) const
			{
				auto fullPath = normalize(std::move(a_path));
				return hash(fullPath);
			}

		protected:
			inline hash_t hash(const std::string& a_fullPath) const noexcept
			{
				hash_t hash;
				switch (std::min<std::size_t>(a_fullPath.length(), 3)) {
				case 3:
					hash.last2 = a_fullPath[a_fullPath.length() - 2];
					[[fallthrough]];
				case 2:
					hash.last = a_fullPath.back();
					[[fallthrough]];
				case 1:
					hash.first = a_fullPath.front();
					[[fallthrough]];
				default:
					break;
				}

				hash.length = static_cast<std::int8_t>(std::min<std::size_t>(a_fullPath.length(), std::numeric_limits<std::int8_t>::max()));
				if (hash.length <= 3) {
					return hash;
				}

				// skip first and last two chars
				for (auto it = a_fullPath.begin() + 1; it != a_fullPath.end() - 2; ++it) {
					hash.crc = *it + hash.crc * HASH_CONSTANT;
				}

				return hash;
			}

			static constexpr std::uint32_t HASH_CONSTANT = 0x1003F;

		private:
			inline std::string normalize(std::string a_path) const
			{
				std::filesystem::path path(std::move(a_path));

				std::string fullPath = path.u8string();
				for (auto& ch : fullPath) {
					ch = mapchar(ch);
				}
				if (fullPath.empty()) {
					fullPath.push_back('.');
				}
				if (!fullPath.empty() && fullPath.back() == '\\') {
					fullPath.pop_back();
				}
				if (!fullPath.empty() && fullPath.front() == '\\') {
					fullPath = fullPath.substr(1);
				}

				return fullPath;
			}
		};


		class file_hasher : public dir_hasher
		{
		public:
			inline hash_t operator()(std::string a_path) const
			{
				auto [stem, extension] = normalize(std::move(a_path));
				return hash(stem, extension);
			}

		private:
			union extension_t
			{
				constexpr extension_t() noexcept :
					i(0)
				{}

				constexpr extension_t(std::uint32_t a_val) noexcept :
					i(a_val)
				{}

				constexpr extension_t(const char a_val[4]) noexcept :
					c{ '\0' }
				{
					for (std::size_t idx = 0; idx < 4; ++idx) {
						c[idx] = a_val[idx];
					}
				}

				constexpr extension_t(const std::string_view& a_val) noexcept :
					c{ '\0' }
				{
					std::size_t idx = 0;
					while (idx < std::min<std::size_t>(a_val.length(), 4)) {
						c[idx] = a_val[idx];
						++idx;
					}
					while (idx < 4) {
						c[idx] = a_val[idx];
						++idx;
					}
				}

				char c[4];
				std::uint32_t i;
			};

			inline std::pair<std::string, std::string> normalize(std::string a_path) const
			{
				std::filesystem::path path(std::move(a_path));

				std::string stem;
				if (path.has_stem()) {
					stem = path.stem().u8string();
					for (auto& ch : stem) {
						ch = mapchar(ch);
					}
				}

				std::string extension;
				if (path.has_extension()) {
					extension = path.extension().u8string();
					for (auto& ch : extension) {
						ch = mapchar(ch);
					}
				}

				return std::make_pair(std::move(stem), std::move(extension));
			}

			inline hash_t hash(const std::string& a_stem, const std::string& a_extension) const noexcept
			{
				constexpr extension_t EXTENSIONS[] = {
					extension_t("\0\0\0\0"),
					extension_t(".nif"),
					extension_t(".kf\0"),
					extension_t(".dds"),
					extension_t(".wav"),
					extension_t(".adp")
				};

				constexpr std::size_t SIZE = std::extent_v<decltype(EXTENSIONS)>;

				auto hash = dir_hasher::hash(a_stem);

				std::uint32_t extCRC = 0;
				for (auto& ch : a_extension) {
					extCRC = ch + extCRC * HASH_CONSTANT;
				}
				hash.crc += extCRC;

				extension_t ext(a_extension);
				for (std::uint8_t i = 0; i < SIZE; ++i) {
					if (ext.i == EXTENSIONS[i].i) {
						hash.first += 32 * (i & 0xFC);
						hash.last += (i & 0xFE) << 6;
						hash.last2 += i << 7;
						break;
					}
				}

				return hash;
			}
		};
	}


	class archive
	{
	public:
		inline archive(std::filesystem::path a_path) :
			_file(a_path, std::ios_base::in | std::ios_base::out | std::ios_base::binary),
			_dirs()
		{}

		inline void parse()
		{
			assert(_file.is_open() && _file);

			detail::header_t head;
			head.read(_file);

			_file.seekg(head.header_size(), std::ios_base::beg);
			for (std::uint32_t i = 0; i < head.num_directories(); ++i) {
				detail::directory_t dir;
				dir.read(_file, head);
				_dirs.push_back(std::move(dir));
			}

			std::streampos offset = head.dir_names_length() + _dirs.size();	// include prefixed length byte
			offset += head.num_files() * detail::file_t::block_size();
			_file.seekg(offset, std::ios_base::cur);

			if (head.file_strings()) {
				for (auto& dir : _dirs) {
					dir.for_each_file([&](detail::file_t& a_file)
					{
						a_file.read_name(_file);
					});
				}
			}

			sanity_check();

			[[maybe_unused]] bool dummy = true;
		}

		inline void dump()
		{
			directory_t root("data");

			std::filesystem::path path;
			directory_t* cur;
			for (auto& dir : _dirs) {
				path = dir.str();
				cur = &root;
				for (auto& entry : path) {
					auto it = cur->subdirectories.insert(std::make_pair(entry, directory_t(entry)));
					cur = &it.first->second;
				}
				dir.for_each_file([&](detail::file_t& a_file)
				{
					cur->files.push_back(a_file.str());
				});
			}

			root.dump("");
		}

	private:
		struct directory_t
		{
			inline directory_t(std::filesystem::path a_name) :
				name(std::move(a_name)),
				subdirectories(),
				files()
			{}

			inline void dump(std::string a_prefix)
			{
				std::cout << a_prefix << name << '\n';

				for (auto& subdir : subdirectories) {
					subdir.second.dump(a_prefix + "|\t");
				}

				std::sort(files.begin(), files.end(), std::less_equal<std::string>());

				for (auto& file : files) {
					std::cout << a_prefix << "|\t" << file << '\n';
				}
			}

			std::filesystem::path name;
			std::map<std::filesystem::path, directory_t> subdirectories;
			std::vector<std::string> files;
		};

		inline void sanity_check()
		{
			for (const auto& dir : _dirs) {
				auto hash = detail::dir_hasher()(dir.str());
				if (hash != dir.hash()) {
					assert(false);
				}

				dir.for_each_file([&](const detail::file_t& a_file)
				{
					auto hash = detail::file_hasher()(a_file.c_str());
					if (hash != a_file.hash()) {
						assert(false);
					}
				});
			}
		}

		std::fstream _file;
		std::vector<detail::directory_t> _dirs;
	};
}
