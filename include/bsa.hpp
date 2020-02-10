#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <istream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <iostream>
#include <map>


namespace bsa
{
	using istream_t = std::istream;


	namespace detail
	{
		class dir_hasher;
		class directory_t;
		class file_hasher;
		class file_t;
		class hash_t;
		class header_t;


		using archive_flags = std::uint32_t;	// BSArchive::ARCHIVE_FLAGS
		static constexpr archive_flags directory_strings_bit = 1 << 0;
		static constexpr archive_flags file_strings_bit = 1 << 1;
		static constexpr archive_flags compressed_bit = 1 << 2;
		static constexpr archive_flags retain_directory_names_bit = 1 << 3;
		static constexpr archive_flags retain_file_names_bit = 1 << 4;
		static constexpr archive_flags retain_file_name_offsets_bit = 1 << 5;
		static constexpr archive_flags xbox_archive_bit = 1 << 6;
		static constexpr archive_flags retain_strings_during_startup_bit = 1 << 7;
		static constexpr archive_flags embedded_file_names_bit = 1 << 8;
		static constexpr archive_flags xbox_compressed_bit = 1 << 9;


		using archive_type = std::uint16_t;	// ARCHIVE_TYPE_INDEX
		static constexpr archive_type meshesbit = 1 << 0;
		static constexpr archive_type texturesbit = 1 << 1;
		static constexpr archive_type menusbit = 1 << 2;
		static constexpr archive_type soundsbit = 1 << 3;
		static constexpr archive_type voicesbit = 1 << 4;
		static constexpr archive_type shadersbit = 1 << 5;
		static constexpr archive_type treesbit = 1 << 6;
		static constexpr archive_type fontsbit = 1 << 7;
		static constexpr archive_type miscbit = 1 << 8;


		[[nodiscard]] constexpr char mapchar(char a_ch) noexcept
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
			constexpr header_t() noexcept :
				_block()
			{}

			constexpr header_t(const header_t& a_rhs) noexcept :
				_block(a_rhs._block)
			{}

			constexpr header_t(header_t&& a_rhs) noexcept :
				_block(std::move(a_rhs._block))
			{}

			constexpr header_t& operator=(const header_t& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_block = a_rhs._block;
				}
				return *this;
			}

			constexpr header_t& operator=(header_t&& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_block = std::move(a_rhs._block);
				}
				return *this;
			}

			[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

			[[nodiscard]] constexpr decltype(auto) archive_type() const noexcept { return _block.archiveType; }
			[[nodiscard]] constexpr std::size_t directory_count() const noexcept { return static_cast<std::size_t>(_block.directoryCount); }
			[[nodiscard]] constexpr std::size_t directory_names_length() const noexcept { return static_cast<std::size_t>(_block.directoryNamesLength); }
			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t>(_block.fileCount); }
			[[nodiscard]] constexpr std::size_t file_names_length() const noexcept { return static_cast<std::size_t>(_block.fileNamesLength); }
			[[nodiscard]] constexpr decltype(auto) flags() const noexcept { return _block.flags; }
			[[nodiscard]] constexpr std::size_t header_size() const noexcept { return static_cast<std::size_t>(_block.headerSize); }
			[[nodiscard]] constexpr std::string_view tag() const { return std::string_view(_block.tag, sizeof(_block.tag)); }
			[[nodiscard]] constexpr std::size_t version() const noexcept { return static_cast<std::size_t>(_block.version); }

			[[nodiscard]] constexpr bool compressed() const noexcept { return (_block.flags & compressed_bit) != 0; }
			[[nodiscard]] constexpr bool directory_strings() const noexcept { return (_block.flags & directory_strings_bit) != 0; }
			[[nodiscard]] constexpr bool embedded_file_names() const noexcept { return (_block.flags & embedded_file_names_bit) != 0; }
			[[nodiscard]] constexpr bool file_strings() const noexcept { return (_block.flags & file_strings_bit) != 0; }
			[[nodiscard]] constexpr bool retain_directory_names() const noexcept { return (_block.flags & retain_directory_names_bit) != 0; }
			[[nodiscard]] constexpr bool retain_file_names() const noexcept { return (_block.flags & retain_file_names_bit) != 0; }
			[[nodiscard]] constexpr bool retain_file_name_offsets() const noexcept { return (_block.flags & retain_file_name_offsets_bit) != 0; }
			[[nodiscard]] constexpr bool retain_strings_during_startup() const noexcept { return (_block.flags & retain_strings_during_startup_bit) != 0; }
			[[nodiscard]] constexpr bool xbox_archive() const noexcept { return (_block.flags & xbox_archive_bit) != 0; }
			[[nodiscard]] constexpr bool xbox_compressed() const noexcept { return (_block.flags & xbox_compressed_bit) != 0; }

			[[nodiscard]] constexpr bool fonts() const noexcept { return (_block.archiveType & fontsbit) != 0; }
			[[nodiscard]] constexpr bool meshes() const noexcept { return (_block.archiveType & meshesbit) != 0; }
			[[nodiscard]] constexpr bool menus() const noexcept { return (_block.archiveType & menusbit) != 0; }
			[[nodiscard]] constexpr bool misc() const noexcept { return (_block.archiveType & miscbit) != 0; }
			[[nodiscard]] constexpr bool shaders() const noexcept { return (_block.archiveType & shadersbit) != 0; }
			[[nodiscard]] constexpr bool sounds() const noexcept { return (_block.archiveType & soundsbit) != 0; }
			[[nodiscard]] constexpr bool textures() const noexcept { return (_block.archiveType & texturesbit) != 0; }
			[[nodiscard]] constexpr bool trees() const noexcept { return (_block.archiveType & treesbit) != 0; }
			[[nodiscard]] constexpr bool voices() const noexcept { return (_block.archiveType & voicesbit) != 0; }

			inline bool read(istream_t& a_input)
			{
				if (!a_input.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
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
					directoryCount(0),
					fileCount(0),
					directoryNamesLength(0),
					fileNamesLength(0),
					archiveType(0)
				{}

				constexpr block_t(const block_t& a_rhs) noexcept :
					tag{ '\0' },
					version(a_rhs.version),
					headerSize(a_rhs.headerSize),
					flags(a_rhs.flags),
					directoryCount(a_rhs.directoryCount),
					fileCount(a_rhs.fileCount),
					directoryNamesLength(a_rhs.directoryNamesLength),
					fileNamesLength(a_rhs.fileNamesLength),
					archiveType(a_rhs.archiveType)
				{
					for (std::size_t i = 0; i < sizeof(tag); ++i) {
						tag[i] = a_rhs.tag[i];
					}
				}

				constexpr block_t(block_t&& a_rhs) noexcept :
					tag{ '\0' },
					version(std::move(a_rhs.version)),
					headerSize(std::move(a_rhs.headerSize)),
					flags(std::move(a_rhs.flags)),
					directoryCount(std::move(a_rhs.directoryCount)),
					fileCount(std::move(a_rhs.fileCount)),
					directoryNamesLength(std::move(a_rhs.directoryNamesLength)),
					fileNamesLength(std::move(a_rhs.fileNamesLength)),
					archiveType(std::move(a_rhs.archiveType))
				{
					for (std::size_t i = 0; i < sizeof(tag); ++i) {
						tag[i] = std::move(a_rhs.tag[i]);
					}
				}

				constexpr block_t& operator=(const block_t& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						for (std::size_t i = 0; i < sizeof(tag); ++i) {
							tag[i] = a_rhs.tag[i];
						}
						version = a_rhs.version;
						headerSize = a_rhs.headerSize;
						flags = a_rhs.flags;
						directoryCount = a_rhs.directoryCount;
						fileCount = a_rhs.fileCount;
						directoryNamesLength = a_rhs.directoryNamesLength;
						fileNamesLength = a_rhs.fileNamesLength;
						archiveType = a_rhs.archiveType;
					}
					return *this;
				}

				constexpr block_t& operator=(block_t&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						for (std::size_t i = 0; i < sizeof(tag); ++i) {
							tag[i] = std::move(a_rhs.tag[i]);
						}
						version = std::move(a_rhs.version);
						headerSize = std::move(a_rhs.headerSize);
						flags = std::move(a_rhs.flags);
						directoryCount = std::move(a_rhs.directoryCount);
						fileCount = std::move(a_rhs.fileCount);
						directoryNamesLength = std::move(a_rhs.directoryNamesLength);
						fileNamesLength = std::move(a_rhs.fileNamesLength);
						archiveType = std::move(a_rhs.archiveType);
					}
					return *this;
				}

				char tag[4];
				std::uint32_t version;
				std::uint32_t headerSize;
				std::uint32_t flags;
				std::uint32_t directoryCount;
				std::uint32_t fileCount;
				std::uint32_t directoryNamesLength;
				std::uint32_t fileNamesLength;
				std::uint16_t archiveType;
			};

			block_t _block;
		};


		class hash_t
		{
		public:
			constexpr hash_t() noexcept :
				_impl()
			{}

			constexpr hash_t(const hash_t& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			constexpr hash_t(hash_t&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			constexpr hash_t& operator=(const hash_t& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			constexpr hash_t& operator=(hash_t&& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			[[nodiscard]] friend constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() == a_rhs.numeric(); }
			[[nodiscard]] friend constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] friend constexpr bool operator<(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() < a_rhs.numeric(); }
			[[nodiscard]] friend constexpr bool operator>(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_rhs < a_lhs; }
			[[nodiscard]] friend constexpr bool operator<=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
			[[nodiscard]] friend constexpr bool operator>=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

			[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

			[[nodiscard]] constexpr decltype(auto) crc() const noexcept { return _impl.block.crc; }
			[[nodiscard]] constexpr decltype(auto) first() const noexcept { return _impl.block.first; }
			[[nodiscard]] constexpr decltype(auto) last() const noexcept { return _impl.block.last; }
			[[nodiscard]] constexpr decltype(auto) last2() const noexcept { return _impl.block.last2; }
			[[nodiscard]] constexpr decltype(auto) length() const noexcept { return _impl.block.length; }

			[[nodiscard]] constexpr std::uint64_t numeric() const noexcept { return _impl.numeric; }

			inline bool read(istream_t& a_input)
			{
				if (!a_input.read(reinterpret_cast<char*>(&_impl.block), sizeof(_impl.block))) {
					return false;
				}

				return true;
			}

		protected:
			friend class dir_hasher;
			friend class file_hasher;

			struct block_t	// BSHash
			{
				std::int8_t last;
				std::int8_t last2;
				std::int8_t length;
				std::int8_t first;
				std::int32_t crc;
			};

			[[nodiscard]] constexpr block_t& block_ref() noexcept { return _impl.block; }
			[[nodiscard]] constexpr const block_t& block_ref() const noexcept { return _impl.block; }

		private:
			union NumericBlock
			{
				constexpr NumericBlock() noexcept :
					numeric(0)
				{}

				constexpr NumericBlock(const NumericBlock& a_rhs) noexcept :
					numeric(a_rhs.numeric)
				{}

				constexpr NumericBlock(NumericBlock&& a_rhs) noexcept :
					numeric(std::move(a_rhs.numeric))
				{}

				constexpr NumericBlock& operator=(const NumericBlock& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						numeric = a_rhs.numeric;
					}
					return *this;
				}

				constexpr NumericBlock& operator=(NumericBlock&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						numeric = std::move(a_rhs.numeric);
					}
					return *this;
				}

				std::int64_t numeric;
				block_t block;
			};

			NumericBlock _impl;
		};


		class file_t
		{
		public:
			inline file_t() noexcept :
				_hash(),
				_block(),
				_name()
			{}

			inline file_t(const file_t& a_rhs) :
				_hash(a_rhs._hash),
				_block(a_rhs._block),
				_name(a_rhs._name)
			{}

			inline file_t(file_t&& a_rhs) noexcept :
				_hash(std::move(a_rhs._hash)),
				_block(std::move(a_rhs._block)),
				_name(std::move(a_rhs._name))
			{}

			inline file_t& operator=(const file_t& a_rhs)
			{
				if (this != &a_rhs) {
					_hash = a_rhs._hash;
					_block = a_rhs._block;
					_name = a_rhs._name;
				}
				return *this;
			}

			inline file_t& operator=(file_t&& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_hash = std::move(a_rhs._hash);
					_block = std::move(a_rhs._block);
					_name = std::move(a_rhs._name);
				}
				return *this;
			}

			[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t) + hash_t::block_size(); }

			[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

			[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
			[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
			[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

			[[nodiscard]] constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(_block.size); }

			[[nodiscard]] inline std::string str() const { return _name; }
			[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

			inline bool read(istream_t& a_input)
			{
				if (!_hash.read(a_input)) {
					return false;
				}

				if (!a_input.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
					return false;
				}

				return true;
			}

			inline bool read_name(istream_t& a_input)
			{
				char ch;
				do {
					a_input.get(ch);
					_name.push_back(ch);
				} while (ch != '\0' && a_input);
				_name.pop_back();
				return !a_input.fail();
			}

		private:
			struct block_t	// BSFileEntry
			{
				constexpr block_t() noexcept :
					size(0),
					offset(0)
				{}

				constexpr block_t(const block_t& a_rhs) noexcept :
					size(a_rhs.size),
					offset(a_rhs.offset)
				{}

				constexpr block_t(block_t&& a_rhs) noexcept :
					size(std::move(a_rhs.size)),
					offset(std::move(a_rhs.offset))
				{}

				constexpr block_t& operator=(const block_t& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						size = a_rhs.size;
						offset = a_rhs.offset;
					}
					return *this;
				}

				constexpr block_t& operator=(block_t&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						size = std::move(a_rhs.size);
						offset = std::move(a_rhs.offset);
					}
					return *this;
				}

				std::uint32_t size;
				std::uint32_t offset;
			};

			hash_t _hash;
			block_t _block;
			std::string _name;
		};


		class directory_t
		{
		public:
			inline directory_t() noexcept :
				_hash(),
				_block(),
				_name(),
				_files()
			{}

			inline directory_t(const directory_t& a_rhs) :
				_hash(a_rhs._hash),
				_block(a_rhs._block),
				_name(a_rhs._name),
				_files(a_rhs._files)
			{}

			inline directory_t(directory_t&& a_rhs) noexcept :
				_hash(std::move(a_rhs._hash)),
				_block(std::move(a_rhs._block)),
				_name(std::move(a_rhs._name)),
				_files(std::move(a_rhs._files))
			{}

			inline directory_t& operator=(const directory_t& a_rhs)
			{
				if (this != &a_rhs) {
					_hash = a_rhs._hash;
					_block = a_rhs._block;
					_name = a_rhs._name;
					_files = a_rhs._files;
				}
				return *this;
			}

			inline directory_t& operator=(directory_t&& a_rhs) noexcept
			{
				if (this != &a_rhs) {
					_hash = std::move(a_rhs._hash);
					_block = std::move(a_rhs._block);
					_name = std::move(a_rhs._name);
					_files = std::move(a_rhs._files);
				}
				return *this;
			}

			[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t) + hash_t::block_size(); }

			[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t>(_block.filesCount); }
			[[nodiscard]] constexpr std::uint64_t file_offset() const noexcept { return static_cast<std::uint64_t>(_block.fileOffset); }

			[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
			[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
			[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

			[[nodiscard]] inline std::string str() const { return _name; }
			[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

			template <class F, typename std::enable_if_t<std::is_invocable_r_v<void, std::decay_t<F>, file_t&>, int> = 0>
			inline void for_each_file(F a_fn)
			{
				for (auto& file : _files) {
					a_fn(file);
				}
			}

			template <class F, typename std::enable_if_t<std::is_invocable_r_v<void, std::decay_t<F>, const file_t&>, int> = 0>
			inline void for_each_file(F a_fn) const
			{
				for (auto& file : _files) {
					a_fn(file);
				}
			}

			inline bool read(istream_t& a_input, const header_t& a_header)
			{
				if (!_hash.read(a_input)) {
					return false;
				}

				if (!a_input.read(reinterpret_cast<char*>(&_block), sizeof(_block))) {
					return false;
				}

				if (!a_header.directory_strings() && file_count() == 0) {
					return true;
				}

				auto pos = a_input.tellg();
				a_input.seekg(file_offset() - a_header.file_names_length(), std::ios_base::beg);

				if (a_header.directory_strings()) {
					std::int8_t length;
					if (!a_input.read(reinterpret_cast<char*>(&length), sizeof(length))) {
						return false;
					}

					_name.resize(static_cast<std::size_t>(length) - 1);
					if (!a_input.read(_name.data(), length)) {
						return false;
					}
				}

				for (std::size_t i = 0; i < file_count(); ++i) {
					file_t file;
					if (!file.read(a_input)) {
						return false;
					}
					_files.push_back(std::move(file));
				}

				a_input.seekg(pos, std::ios_base::beg);

				return true;
			}

		private:
			struct block_t	// BSDirectoryEntry
			{
				constexpr block_t() noexcept :
					filesCount(0),
					pad(0),
					fileOffset(0)
				{}

				constexpr block_t(const block_t& a_rhs) noexcept :
					filesCount(a_rhs.filesCount),
					pad(a_rhs.pad),
					fileOffset(a_rhs.fileOffset)
				{}

				constexpr block_t(block_t&& a_rhs) noexcept :
					filesCount(std::move(a_rhs.filesCount)),
					pad(std::move(a_rhs.pad)),
					fileOffset(std::move(a_rhs.fileOffset))
				{}

				constexpr block_t& operator=(const block_t& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						filesCount = a_rhs.filesCount;
						pad = a_rhs.pad;
						fileOffset = a_rhs.fileOffset;
					}
					return *this;
				}

				constexpr block_t& operator=(block_t&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						filesCount = std::move(a_rhs.filesCount);
						pad = std::move(a_rhs.pad);
						fileOffset = std::move(a_rhs.fileOffset);
					}
					return *this;
				}

				std::uint32_t filesCount;
				std::uint32_t pad;
				std::uint64_t fileOffset;
			};

			hash_t _hash;
			block_t _block;
			std::string _name;
			std::vector<file_t> _files;
		};


		class dir_hasher
		{
		public:
			dir_hasher() = default;
			dir_hasher(const dir_hasher&) = default;
			dir_hasher(dir_hasher&&) = default;

			dir_hasher& operator=(const dir_hasher&) = default;
			dir_hasher& operator=(dir_hasher&&) = default;

			[[nodiscard]] inline hash_t operator()(std::string a_path) const
			{
				auto fullPath = normalize(std::move(a_path));
				return hash(fullPath);
			}

		protected:
			[[nodiscard]] inline hash_t hash(const std::string& a_fullPath) const noexcept
			{
				hash_t hash;
				auto& block = hash.block_ref();
				switch (std::min<std::size_t>(a_fullPath.length(), 3)) {
				case 3:
					block.last2 = a_fullPath[a_fullPath.length() - 2];
					[[fallthrough]];
				case 2:
					block.last = a_fullPath.back();
					[[fallthrough]];
				case 1:
					block.first = a_fullPath.front();
					[[fallthrough]];
				default:
					break;
				}

				block.length = static_cast<std::int8_t>(std::min<std::size_t>(a_fullPath.length(), std::numeric_limits<std::int8_t>::max()));
				if (block.length <= 3) {
					return hash;
				}

				// skip first and last two chars
				for (auto it = a_fullPath.begin() + 1; it != a_fullPath.end() - 2; ++it) {
					block.crc = *it + block.crc * HASH_CONSTANT;
				}

				return hash;
			}

			static constexpr std::uint32_t HASH_CONSTANT = 0x1003F;

		private:
			[[nodiscard]] inline std::string normalize(std::string a_path) const
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
			file_hasher() = default;
			file_hasher(const file_hasher&) = default;
			file_hasher(file_hasher&&) = default;

			file_hasher& operator=(const file_hasher&) = default;
			file_hasher& operator=(file_hasher&&) = default;

			[[nodiscard]] inline hash_t operator()(std::string a_path) const
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
						c[idx] = '\0';
						++idx;
					}
				}

				char c[4];
				std::uint32_t i;
			};

			[[nodiscard]] inline std::pair<std::string, std::string> normalize(std::string a_path) const
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

			[[nodiscard]] inline hash_t hash(const std::string& a_stem, const std::string& a_extension) const noexcept
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
				auto& block = hash.block_ref();

				std::uint32_t extCRC = 0;
				for (auto& ch : a_extension) {
					extCRC = ch + extCRC * HASH_CONSTANT;
				}
				block.crc += extCRC;

				extension_t ext(a_extension);
				for (std::uint8_t i = 0; i < SIZE; ++i) {
					if (ext.i == EXTENSIONS[i].i) {
						block.first += 32 * (i & 0xFC);
						block.last += (i & 0xFE) << 6;
						block.last2 += i << 7;
						break;
					}
				}

				return hash;
			}
		};


		using hash_ref = std::reference_wrapper<hash_t>;
		using file_ref = std::reference_wrapper<file_t>;
		using directory_ref = std::reference_wrapper<directory_t>;
	}


	class archive;
	class directory;
	class directory_iterator;
	class file;
	class hash;


	class hash
	{
	public:
		inline hash(const hash& a_rhs) noexcept :
			_impl(a_rhs._impl)
		{}

		inline hash(hash&& a_rhs) noexcept :
			_impl(std::move(a_rhs._impl))
		{}

		inline hash& operator=(const hash& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = a_rhs._impl;
			}
			return *this;
		}

		inline hash& operator=(hash&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = std::move(a_rhs._impl);
			}
			return *this;
		}

		[[nodiscard]] inline std::uint32_t crc() const noexcept { return _impl.get().crc(); }
		[[nodiscard]] inline char first_char() const noexcept { return static_cast<char>(_impl.get().first()); }
		[[nodiscard]] inline char last_char() const noexcept { return static_cast<char>(_impl.get().last()); }
		[[nodiscard]] inline std::int8_t length() const noexcept { return _impl.get().length(); }
		[[nodiscard]] inline std::int64_t numeric_hash() const noexcept { return _impl.get().numeric(); }
		[[nodiscard]] inline char second_to_last_char() const noexcept { return static_cast<char>(_impl.get().last2()); }

	protected:
		friend class directory;
		friend class file;

		inline hash(detail::hash_t& a_rhs) noexcept :
			_impl(std::ref(a_rhs))
		{}

		inline hash(const detail::hash_ref& a_rhs) noexcept :
			_impl(a_rhs)
		{}

	private:
		detail::hash_ref _impl;
	};


	class file
	{
	public:
		inline file(const file& a_rhs) noexcept :
			_impl(a_rhs._impl)
		{}

		inline file(file&& a_rhs) noexcept :
			_impl(std::move(a_rhs._impl))
		{}

		inline file& operator=(const file& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = a_rhs._impl;
			}
			return *this;
		}

		inline file& operator=(file&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = std::move(a_rhs._impl);
			}
			return *this;
		}

		[[nodiscard]] inline const char* c_str() const noexcept { return _impl.get().c_str(); }
		[[nodiscard]] inline hash hash_value() const noexcept { return _impl.get().hash_ref(); }
		[[nodiscard]] inline std::size_t size() const noexcept { return _impl.get().size(); }
		[[nodiscard]] inline const std::string& string() const noexcept { return _impl.get().str_ref(); }

	protected:
		friend class directory_iterator;

		explicit inline file(detail::file_t& a_rhs) noexcept :
			_impl(std::ref(a_rhs))
		{}

		explicit inline file(const detail::file_ref& a_rhs) noexcept :
			_impl(a_rhs)
		{}

	private:
		detail::file_ref _impl;
	};


	class directory
	{
	public:
		inline directory(const directory& a_rhs) noexcept :
			_impl(a_rhs._impl)
		{}

		inline directory(directory&& a_rhs) noexcept :
			_impl(std::move(a_rhs._impl))
		{}

		inline directory& operator=(const directory& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = a_rhs._impl;
			}
			return *this;
		}

		inline directory& operator=(directory&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_impl = std::move(a_rhs._impl);
			}
			return *this;
		}

		[[nodiscard]] inline const char* c_str() const noexcept { return _impl.get().c_str(); }
		[[nodiscard]] inline std::size_t file_count() const noexcept { return _impl.get().file_count(); }
		[[nodiscard]] inline hash hash_value() const noexcept { return _impl.get().hash_ref(); }
		[[nodiscard]] inline const std::string& string() const noexcept { return _impl.get().str_ref(); }

	protected:
		friend class directory_iterator;

		explicit inline directory(detail::directory_t& a_rhs) noexcept :
			_impl(std::ref(a_rhs))
		{}

		explicit inline directory(const detail::directory_ref& a_rhs) noexcept :
			_impl(a_rhs)
		{}

	private:
		detail::directory_ref _impl;
	};


	class archive
	{
	public:
		inline archive(std::filesystem::path a_path) :
			_dirs()
		{
			std::ifstream file(a_path, std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			if (file.is_open()) {
				read(file);
			}
		}

		inline void read(istream_t& a_input)
		{
			assert(a_input);
			_dirs.clear();

			_header.read(a_input);

			a_input.seekg(_header.header_size(), std::ios_base::beg);
			for (std::size_t i = 0; i < _header.directory_count(); ++i) {
				detail::directory_t dir;
				dir.read(a_input, _header);
				_dirs.push_back(std::move(dir));
			}

			std::streampos offset = _header.directory_names_length() + _header.directory_count();	// include prefixed length byte
			offset += _header.file_count() * detail::file_t::block_size();
			a_input.seekg(offset, std::ios_base::cur);

			if (_header.file_strings()) {
				for (auto& dir : _dirs) {
					dir.for_each_file([&](detail::file_t& a_file)
					{
						a_file.read_name(a_input);
					});
				}
			}

			assert(sanity_check());

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

	protected:
		friend class directory_iterator;

		using directory_container = std::vector<detail::directory_t>;
		using iterator = typename directory_container::iterator;
		using const_iterator = typename directory_container::const_iterator;

		[[nodiscard]] inline iterator begin() noexcept { return _dirs.begin(); }
		[[nodiscard]] inline const_iterator begin() const noexcept { return _dirs.begin(); }
		[[nodiscard]] inline const_iterator cbegin() const noexcept { return _dirs.cbegin(); }

		[[nodiscard]] inline iterator end() noexcept { return _dirs.end(); }
		[[nodiscard]] inline const_iterator end() const noexcept { return _dirs.end(); }
		[[nodiscard]] inline const_iterator cend() const noexcept { return _dirs.cend(); }

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

		inline bool sanity_check()
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

			return true;
		}

		detail::header_t _header;
		directory_container _dirs;
	};


	class directory_iterator
	{
	public:
		using reference = directory&;
		using pointer = directory*;

		inline directory_iterator() noexcept :
			_dirs(std::nullopt),
			_pos(NPOS)
		{}

		inline directory_iterator(const directory_iterator& a_rhs) :
			_dirs(a_rhs._dirs),
			_pos(a_rhs._pos)
		{}

		inline directory_iterator(directory_iterator&& a_rhs) noexcept :
			_dirs(std::move(a_rhs._dirs)),
			_pos(std::move(a_rhs._pos))
		{
			a_rhs._pos = NPOS;
		}

		explicit inline directory_iterator(const archive& a_archive) :
			_dirs(std::in_place_t()),
			_pos(0)
		{
			auto& arch = const_cast<archive&>(a_archive);
			for (auto& dir : arch) {
				_dirs->push_back(directory(dir));
			}
		}

		inline directory_iterator& operator=(const directory_iterator& a_rhs)
		{
			if (this != &a_rhs) {
				_dirs = a_rhs._dirs;
				_pos = a_rhs._pos;
			}
			return *this;
		}

		inline directory_iterator& operator=(directory_iterator&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_dirs = std::move(a_rhs._dirs);
				_pos = std::move(a_rhs._pos);
				a_rhs._pos = NPOS;
			}
			return *this;
		}

		[[nodiscard]] friend constexpr bool operator==(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept { return !a_lhs._dirs && !a_rhs._dirs; }
		[[nodiscard]] friend constexpr bool operator!=(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

		[[nodiscard]] inline reference operator*() { return fetch(); }
		[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

		inline directory_iterator& operator++()
		{
			++_pos;
			if (_pos >= _dirs->size()) {
				_dirs.reset();
				_pos = NPOS;
			}
			return *this;
		}

		[[nodiscard]] friend inline directory_iterator begin(directory_iterator a_iter) noexcept
		{
			return a_iter;
		}

		[[nodiscard]] friend inline directory_iterator end([[maybe_unused]] const directory_iterator&) noexcept
		{
			return {};
		}

	private:
		inline reference fetch() { return _dirs.value()[_pos]; }

		static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

		std::optional<std::vector<directory>> _dirs;
		std::size_t _pos;
	};
}
