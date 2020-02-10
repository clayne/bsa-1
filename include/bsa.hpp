#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
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
#include <variant>
#include <vector>


namespace bsa
{
	using archive_flag = std::uint32_t;	// BSArchive::ARCHIVE_FLAGS
	static constexpr archive_flag directory_strings_bit = 1 << 0;
	static constexpr archive_flag file_strings_bit = 1 << 1;
	static constexpr archive_flag compressed_bit = 1 << 2;
	static constexpr archive_flag retain_directory_names_bit = 1 << 3;
	static constexpr archive_flag retain_file_names_bit = 1 << 4;
	static constexpr archive_flag retain_file_name_offsets_bit = 1 << 5;
	static constexpr archive_flag xbox_archive_bit = 1 << 6;
	static constexpr archive_flag retain_strings_during_startup_bit = 1 << 7;
	static constexpr archive_flag embedded_file_names_bit = 1 << 8;
	static constexpr archive_flag xbox_compressed_bit = 1 << 9;


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


	class exception : public std::exception
	{
	public:
		inline exception() noexcept : exception("base bsa exception") {}
		inline exception(const exception&) noexcept = default;
		inline exception(const char* a_what) noexcept : std::exception(), _what(a_what) {}
		virtual ~exception() noexcept = default;

		exception& operator=(const exception&) noexcept = default;

		virtual const char* what() const noexcept override { return _what; }

	private:
		const char* _what;
	};


	class io_error : public exception
	{
	public:
		inline io_error() noexcept : io_error("failure while performing i/o with bsa") {}
		inline io_error(const io_error&) noexcept = default;
		inline io_error(const char* a_what) noexcept : exception(a_what) {}
		virtual ~io_error() noexcept = default;
	};


	class input_error : public io_error
	{
	public:
		inline input_error() noexcept : input_error("failure while parsing bsa") {}
		inline input_error(const input_error&) noexcept = default;
		inline input_error(const char* a_what) noexcept : io_error(a_what) {}
		virtual ~input_error() noexcept = default;
	};


	class version_error : public input_error
	{
	public:
		inline version_error() noexcept : version_error("encountered unhandled version") {}
		inline version_error(const version_error&) noexcept = default;
		inline version_error(const char* a_what) noexcept : input_error(a_what) {}
		virtual ~version_error() noexcept = default;
	};


	namespace detail
	{
		class dir_hasher;
		class directory_t;
		class file_hasher;
		class file_t;
		class hash_t;
		class header_t;


		template <class T>
		inline void swap_endian(T& a_val)
		{
			auto iter = reinterpret_cast<char*>(std::addressof(a_val));
			std::reverse(iter, iter + sizeof(T));
		}


		// Bethesda uses std::tolower to convert chars to lowercase, however
		// they use the default C locale to convert the characters,
		// so I've emulated this functionality, which enables a constexpr
		// mapping, and allows users to set the locale without encountering
		// unexpected hashing behavior
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


		class istream_t
		{
		public:
			using stream_type = std::istream;
			using reference = stream_type&;
			using const_reference = const stream_type&;
			using pointer = stream_type*;
			using const_pointer = const stream_type*;
			using char_type = typename stream_type::char_type;
			using pos_type = typename stream_type::pos_type;
			using off_type = typename stream_type::off_type;

			inline istream_t() = delete;
			inline istream_t(const istream_t&) = delete;
			inline istream_t(istream_t&&) = delete;
			inline istream_t(stream_type& a_stream) :
				_stream(a_stream),
				_beg(a_stream.tellg())
			{
				if (!_stream) {
					throw input_error();
				}
			}

			istream_t& operator=(const istream_t&) = delete;
			istream_t& operator=(istream_t&&) = delete;

			[[nodiscard]] constexpr reference operator*() noexcept { return _stream; }
			[[nodiscard]] constexpr const_reference operator*() const noexcept { return _stream; }

			[[nodiscard]] constexpr pointer operator->() noexcept { return std::addressof(_stream); }
			[[nodiscard]] constexpr const_pointer operator->() const noexcept { return std::addressof(_stream); }

			[[nodiscard]] bool operator!() const { return _stream.fail(); }
			[[nodiscard]] explicit inline operator bool() const { return !_stream.fail(); }

			inline istream_t& read(char_type* a_str, std::streamsize a_count) { if (!_stream.read(a_str, a_count)) throw input_error(); return *this; }

			inline istream_t& seekg_abs(pos_type a_pos) { if (!_stream.seekg(a_pos)) throw input_error(); return *this; }
			inline istream_t& seekg_beg() { if (!_stream.seekg(_beg)) throw input_error(); return *this; }
			inline istream_t& seekg_rel(off_type a_off) { if (!_stream.seekg(a_off, std::ios_base::cur)) throw input_error(); return *this; }

		private:
			stream_type& _stream;
			pos_type _beg;
		};


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

			[[nodiscard]] constexpr std::size_t directory_count() const noexcept { return static_cast<std::size_t>(_block.directoryCount); }
			[[nodiscard]] constexpr std::size_t directory_names_length() const noexcept { return static_cast<std::size_t>(_block.directoryNamesLength); }
			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t>(_block.fileCount); }
			[[nodiscard]] constexpr std::size_t file_names_length() const noexcept { return static_cast<std::size_t>(_block.fileNamesLength); }
			[[nodiscard]] constexpr archive_flag flags() const noexcept { return static_cast<archive_flag>(_block.flags); }
			[[nodiscard]] constexpr std::size_t header_size() const noexcept { return static_cast<std::size_t>(_block.headerSize); }
			[[nodiscard]] constexpr std::string_view tag() const { return std::string_view(_block.tag, sizeof(_block.tag)); }
			[[nodiscard]] constexpr archive_type types() const noexcept { return static_cast<archive_type>(_block.archiveTypes); }
			[[nodiscard]] constexpr std::size_t version() const noexcept { return static_cast<std::size_t>(_block.version); }

			[[nodiscard]] constexpr bool compressed() const noexcept { return (flags() & compressed_bit) != 0; }
			[[nodiscard]] constexpr bool directory_strings() const noexcept { return (flags() & directory_strings_bit) != 0; }
			[[nodiscard]] constexpr bool embedded_file_names() const noexcept { return (flags() & embedded_file_names_bit) != 0; }
			[[nodiscard]] constexpr bool file_strings() const noexcept { return (flags() & file_strings_bit) != 0; }
			[[nodiscard]] constexpr bool retain_directory_names() const noexcept { return (flags() & retain_directory_names_bit) != 0; }
			[[nodiscard]] constexpr bool retain_file_names() const noexcept { return (flags() & retain_file_names_bit) != 0; }
			[[nodiscard]] constexpr bool retain_file_name_offsets() const noexcept { return (flags() & retain_file_name_offsets_bit) != 0; }
			[[nodiscard]] constexpr bool retain_strings_during_startup() const noexcept { return (flags() & retain_strings_during_startup_bit) != 0; }
			[[nodiscard]] constexpr bool xbox_archive() const noexcept { return (flags() & xbox_archive_bit) != 0; }
			[[nodiscard]] constexpr bool xbox_compressed() const noexcept { return (flags() & xbox_compressed_bit) != 0; }

			[[nodiscard]] constexpr bool fonts() const noexcept { return (types() & fontsbit) != 0; }
			[[nodiscard]] constexpr bool meshes() const noexcept { return (types() & meshesbit) != 0; }
			[[nodiscard]] constexpr bool menus() const noexcept { return (types() & menusbit) != 0; }
			[[nodiscard]] constexpr bool misc() const noexcept { return (types() & miscbit) != 0; }
			[[nodiscard]] constexpr bool shaders() const noexcept { return (types() & shadersbit) != 0; }
			[[nodiscard]] constexpr bool sounds() const noexcept { return (types() & soundsbit) != 0; }
			[[nodiscard]] constexpr bool textures() const noexcept { return (types() & texturesbit) != 0; }
			[[nodiscard]] constexpr bool trees() const noexcept { return (types() & treesbit) != 0; }
			[[nodiscard]] constexpr bool voices() const noexcept { return (types() & voicesbit) != 0; }

			constexpr void clear() noexcept { _block = block_t(); }

			inline void read(istream_t& a_input)
			{
				_block.read(a_input);
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
					archiveTypes(0),
					pad(0)
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
					archiveTypes(a_rhs.archiveTypes),
					pad(a_rhs.pad)
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
					archiveTypes(std::move(a_rhs.archiveTypes)),
					pad(std::move(a_rhs.pad))
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
						archiveTypes = a_rhs.archiveTypes;
						pad = a_rhs.pad;
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
						archiveTypes = std::move(a_rhs.archiveTypes);
						pad = std::move(a_rhs.pad);
					}
					return *this;
				}

				inline void read(istream_t& a_input)
				{
					a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
				}

				char tag[4];
				std::uint32_t version;
				std::uint32_t headerSize;
				std::uint32_t flags;
				std::uint32_t directoryCount;
				std::uint32_t fileCount;
				std::uint32_t directoryNamesLength;
				std::uint32_t fileNamesLength;
				std::uint16_t archiveTypes;
				std::uint16_t pad;
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

			inline void read(istream_t& a_input, const header_t& a_header)
			{
				_impl.read(a_input);

				if (a_header.xbox_archive()) {
					byte_swap();
				}
			}

			inline void byte_swap()
			{
				swap_endian(_impl.block.crc);
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
				std::uint32_t crc;
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

				inline void read(istream_t& a_input)
				{
					a_input.read(reinterpret_cast<char*>(this), sizeof(NumericBlock));
				}

				std::uint64_t numeric;
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

			[[nodiscard]] constexpr std::size_t offset() const noexcept { return static_cast<std::size_t>(_block.offset); }

			[[nodiscard]] constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(_block.size); }

			[[nodiscard]] inline std::string str() const { return _name; }
			[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

			inline void read(istream_t& a_input, const header_t& a_header)
			{
				_hash.read(a_input, a_header);
				_block.read(a_input, a_header);
			}

			inline void read_name(istream_t& a_input)
			{
				char ch;
				do {
					a_input->get(ch);
					_name.push_back(ch);
				} while (ch != '\0' && a_input);
				_name.pop_back();

				if (!a_input) {
					throw input_error();
				}
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

				inline void read(istream_t& a_input, [[maybe_unused]] const header_t& a_header)
				{
					a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
				}

				inline void byte_swap()
				{
					swap_endian(size);
					swap_endian(offset);
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
			using container_type = std::vector<file_t>;
			using iterator = typename container_type::iterator;
			using const_iterator = typename container_type::const_iterator;

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

			[[nodiscard]] static constexpr std::size_t block_size(std::size_t a_version)
			{
				switch (a_version) {
				case 103:
				case 104:
					return sizeof(block103_t) + hash_t::block_size();
				case 105:
					return sizeof(block105_t) + hash_t::block_size();
				default:
					throw version_error();
				}
			}

			[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

			[[nodiscard]] constexpr std::size_t file_count() const noexcept
			{
				switch (_block.index()) {
				case v103:
					return static_cast<std::size_t>(std::get<v103>(_block).fileCount);
				case v105:
					return static_cast<std::size_t>(std::get<v105>(_block).fileCount);
				default:
					assert(false);
					return 0;
				}
			}

			[[nodiscard]] constexpr std::uint64_t file_offset() const noexcept
			{
				switch (_block.index()) {
				case v103:
					return static_cast<std::uint64_t>(std::get<v103>(_block).fileOffset);
				case v105:
					return static_cast<std::uint64_t>(std::get<v105>(_block).fileOffset);
				default:
					assert(false);
					return std::numeric_limits<std::uint64_t>::max();
				}
			}

			[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
			[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
			[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

			[[nodiscard]] inline std::string str() const { return _name; }
			[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

			[[nodiscard]] inline iterator begin() noexcept { return _files.begin(); }
			[[nodiscard]] inline const_iterator begin() const noexcept { return _files.begin(); }
			[[nodiscard]] inline const_iterator cbegin() const noexcept { return _files.cbegin(); }

			[[nodiscard]] inline iterator end() noexcept { return _files.end(); }
			[[nodiscard]] inline const_iterator end() const noexcept { return _files.end(); }
			[[nodiscard]] inline const_iterator cend() const noexcept { return _files.cend(); }

			inline void read(istream_t& a_input, const header_t& a_header)
			{
				_hash.read(a_input, a_header);

				switch (a_header.version()) {
				case 103:
				case 104:
					{
						block103_t block;
						block.read(a_input, a_header);
						_block.emplace<v103>(std::move(block));
					}
					break;
				case 105:
					{
						block105_t block;
						block.read(a_input, a_header);
						_block.emplace<v105>(std::move(block));
					}
					break;
				default:
					throw version_error();
				}

				if (a_header.directory_strings() || file_count() > 0) {
					read_extra(a_input, a_header);
				}
			}

		private:
			enum { v103, v105 };

			// same as 104
			struct block103_t	// BSDirectoryEntry x86
			{
				constexpr block103_t() noexcept :
					fileCount(0),
					fileOffset(0)
				{}

				constexpr block103_t(const block103_t& a_rhs) noexcept :
					fileCount(a_rhs.fileCount),
					fileOffset(a_rhs.fileOffset)
				{}

				constexpr block103_t(block103_t&& a_rhs) noexcept :
					fileCount(std::move(a_rhs.fileCount)),
					fileOffset(std::move(a_rhs.fileOffset))
				{}

				constexpr block103_t& operator=(const block103_t& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						fileCount = a_rhs.fileCount;
						fileOffset = a_rhs.fileOffset;
					}
					return *this;
				}

				constexpr block103_t& operator=(block103_t&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						fileCount = std::move(a_rhs.fileCount);
						fileOffset = std::move(a_rhs.fileOffset);
					}
					return *this;
				}

				inline void read(istream_t& a_input, [[maybe_unused]] const header_t& a_header)
				{
					a_input.read(reinterpret_cast<char*>(this), sizeof(block103_t));
				}

				inline void byte_swap()
				{
					swap_endian(fileCount);
					swap_endian(fileOffset);
				}

				std::uint32_t fileCount;
				std::uint32_t fileOffset;
			};

			struct block105_t	// BSDirectoryEntry x64
			{
				constexpr block105_t() noexcept :
					fileCount(0),
					pad(0),
					fileOffset(0)
				{}

				constexpr block105_t(const block105_t& a_rhs) noexcept :
					fileCount(a_rhs.fileCount),
					pad(a_rhs.pad),
					fileOffset(a_rhs.fileOffset)
				{}

				constexpr block105_t(block105_t&& a_rhs) noexcept :
					fileCount(std::move(a_rhs.fileCount)),
					pad(std::move(a_rhs.pad)),
					fileOffset(std::move(a_rhs.fileOffset))
				{}

				constexpr block105_t& operator=(const block105_t& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						fileCount = a_rhs.fileCount;
						pad = a_rhs.pad;
						fileOffset = a_rhs.fileOffset;
					}
					return *this;
				}

				constexpr block105_t& operator=(block105_t&& a_rhs) noexcept
				{
					if (this != &a_rhs) {
						fileCount = std::move(a_rhs.fileCount);
						pad = std::move(a_rhs.pad);
						fileOffset = std::move(a_rhs.fileOffset);
					}
					return *this;
				}

				inline void read(istream_t& a_input, [[maybe_unused]] const header_t& a_header)
				{
					a_input->read(reinterpret_cast<char*>(this), sizeof(block105_t));
				}

				inline void byte_swap()
				{
					swap_endian(fileCount);
					swap_endian(fileOffset);
				}

				std::uint32_t fileCount;
				std::uint32_t pad;
				std::uint64_t fileOffset;
			};

			inline void read_extra(istream_t& a_input, const header_t& a_header)
			{
				auto pos = a_input->tellg();
				a_input.seekg_beg();
				a_input.seekg_rel(file_offset() - a_header.file_names_length());

				if (a_header.directory_strings()) {
					std::int8_t length;
					a_input.read(reinterpret_cast<char*>(&length), sizeof(length));

					_name.resize(static_cast<std::size_t>(length) - 1);
					a_input.read(_name.data(), length);
				}

				for (std::size_t i = 0; i < file_count(); ++i) {
					file_t file;
					file.read(a_input, a_header);
					_files.push_back(std::move(file));
				}

				a_input.seekg_abs(pos);
			}

			hash_t _hash;
			std::variant<block103_t, block105_t> _block;
			std::string _name;
			container_type _files;
		};


		class dir_hasher
		{
		public:
			dir_hasher() = default;
			dir_hasher(const dir_hasher&) = default;
			dir_hasher(dir_hasher&&) = default;

			dir_hasher& operator=(const dir_hasher&) = default;
			dir_hasher& operator=(dir_hasher&&) = default;

			[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
			{
				auto fullPath = normalize(a_path);
				return hash(fullPath);
			}

		protected:
			[[nodiscard]] inline hash_t hash(const std::string& a_fullPath) const noexcept
			{
				constexpr auto LEN_MAX = static_cast<std::size_t>(std::numeric_limits<std::int8_t>::max());

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

				block.length = static_cast<std::int8_t>(std::min(a_fullPath.length(), LEN_MAX));
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
			[[nodiscard]] inline std::string normalize(std::string_view a_path) const
			{
				std::filesystem::path path(a_path);

				std::string fullPath = path.string();
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

			[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
			{
				auto [stem, extension] = normalize(a_path);
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

			[[nodiscard]] inline std::pair<std::string, std::string> normalize(std::string_view a_path) const
			{
				std::filesystem::path path(a_path);

				std::string stem;
				if (path.has_stem()) {
					stem = path.stem().string();
					for (auto& ch : stem) {
						ch = mapchar(ch);
					}
				}

				std::string extension;
				if (path.has_extension()) {
					extension = path.extension().string();
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
	class file_iterator;
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
		[[nodiscard]] inline std::uint64_t numeric_hash() const noexcept { return _impl.get().numeric(); }
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
		friend class file_iterator;

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
		friend class file_iterator;

		explicit inline directory(detail::directory_t& a_rhs) noexcept :
			_impl(std::ref(a_rhs))
		{}

		explicit inline directory(const detail::directory_ref& a_rhs) noexcept :
			_impl(a_rhs)
		{}

		[[nodiscard]] inline detail::directory_t& directory_ref() noexcept { return _impl.get(); }
		[[nodiscard]] inline const detail::directory_t& directory_ref() const noexcept { return _impl.get(); }

	private:
		detail::directory_ref _impl;
	};


	class archive
	{
	public:
		inline archive() noexcept :
			_dirs(),
			_header()
		{}

		inline archive(const archive& a_archive) :
			_dirs(a_archive._dirs),
			_header(a_archive._header)
		{}

		inline archive(archive&& a_archive) noexcept :
			_dirs(std::move(a_archive._dirs)),
			_header(std::move(a_archive._header))
		{}

		inline archive(const std::filesystem::path& a_path) :
			_dirs(),
			_header()
		{
			read(a_path);
		}

		inline archive(std::filesystem::path&& a_path) :
			_dirs(),
			_header()
		{
			read(std::move(a_path));
		}

		inline archive& operator=(const archive& a_rhs)
		{
			if (this != &a_rhs) {
				_header = a_rhs._header;
				_dirs = a_rhs._dirs;
			}
			return *this;
		}

		inline archive& operator=(archive&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_header = std::move(a_rhs._header);
				_dirs = std::move(a_rhs._dirs);
			}
			return *this;
		}

		[[nodiscard]] constexpr std::size_t directory_count() const noexcept { return _header.directory_count(); }
		[[nodiscard]] constexpr std::size_t directory_names_length() const noexcept { return _header.directory_names_length(); }
		[[nodiscard]] constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
		[[nodiscard]] constexpr std::size_t file_names_length() const noexcept { return _header.file_names_length(); }
		[[nodiscard]] constexpr archive_flag flags() const noexcept { return _header.flags(); }
		[[nodiscard]] constexpr std::size_t header_size() const noexcept { return _header.header_size(); }
		[[nodiscard]] constexpr std::string_view tag() const { return _header.tag(); }
		[[nodiscard]] constexpr archive_type types() const noexcept { return _header.types(); }
		[[nodiscard]] constexpr std::size_t version() const noexcept { return _header.version(); }

		[[nodiscard]] constexpr bool compressed() const noexcept { return _header.compressed(); }
		[[nodiscard]] constexpr bool directory_strings() const noexcept { return _header.directory_strings(); }
		[[nodiscard]] constexpr bool embedded_file_names() const noexcept { return _header.embedded_file_names(); }
		[[nodiscard]] constexpr bool file_strings() const noexcept { return _header.file_strings(); }
		[[nodiscard]] constexpr bool retain_directory_names() const noexcept { return _header.retain_directory_names(); }
		[[nodiscard]] constexpr bool retain_file_names() const noexcept { return _header.retain_file_names(); }
		[[nodiscard]] constexpr bool retain_file_name_offsets() const noexcept { return _header.retain_file_name_offsets(); }
		[[nodiscard]] constexpr bool retain_strings_during_startup() const noexcept { return _header.retain_strings_during_startup(); }
		[[nodiscard]] constexpr bool xbox_archive() const noexcept { return _header.xbox_archive(); }
		[[nodiscard]] constexpr bool xbox_compressed() const noexcept { return _header.xbox_compressed(); }

		[[nodiscard]] constexpr bool fonts() const noexcept { return _header.fonts(); }
		[[nodiscard]] constexpr bool meshes() const noexcept { return _header.meshes(); }
		[[nodiscard]] constexpr bool menus() const noexcept { return _header.menus(); }
		[[nodiscard]] constexpr bool misc() const noexcept { return _header.misc(); }
		[[nodiscard]] constexpr bool shaders() const noexcept { return _header.shaders(); }
		[[nodiscard]] constexpr bool sounds() const noexcept { return _header.sounds(); }
		[[nodiscard]] constexpr bool textures() const noexcept { return _header.textures(); }
		[[nodiscard]] constexpr bool trees() const noexcept { return _header.trees(); }
		[[nodiscard]] constexpr bool voices() const noexcept { return _header.voices(); }

		inline void read(const std::filesystem::path& a_path)
		{
			std::ifstream file(a_path, std::ios_base::in | std::ios_base::binary);
			if (file.is_open()) {
				read(file);
			}
		}

		inline void read(std::filesystem::path&& a_path)
		{
			read(a_path);
		}

		inline void read(std::istream& a_input)
		{
			detail::istream_t input(a_input);

			_dirs.clear();
			_header.clear();

			_header.read(input);
			switch (_header.version()) {
			case 103:
			case 104:
			case 105:
				break;
			default:
				throw version_error();
			}

			input.seekg_beg();
			input.seekg_rel(_header.header_size());
			for (std::size_t i = 0; i < _header.directory_count(); ++i) {
				detail::directory_t dir;
				dir.read(input, _header);
				_dirs.push_back(std::move(dir));
			}

			auto offset = static_cast<std::streamoff>(_header.directory_names_length() + _header.directory_count());	// include prefixed length byte
			offset += _header.file_count() * detail::file_t::block_size();
			input.seekg_rel(offset);

			if (_header.file_strings()) {
				for (auto& dir : _dirs) {
					for (auto& file : dir) {
						file.read_name(input);
					}
				}
			}

			assert(sanity_check());
		}

	protected:
		friend class directory_iterator;

		using container_type = std::vector<detail::directory_t>;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;

		[[nodiscard]] inline iterator begin() noexcept { return _dirs.begin(); }
		[[nodiscard]] inline const_iterator begin() const noexcept { return _dirs.begin(); }
		[[nodiscard]] inline const_iterator cbegin() const noexcept { return _dirs.cbegin(); }

		[[nodiscard]] inline iterator end() noexcept { return _dirs.end(); }
		[[nodiscard]] inline const_iterator end() const noexcept { return _dirs.end(); }
		[[nodiscard]] inline const_iterator cend() const noexcept { return _dirs.cend(); }

	private:
		inline bool sanity_check()
		{
			for (const auto& dir : _dirs) {
				auto dHash = detail::dir_hasher()(dir.str_ref());
				if (dHash != dir.hash()) {
					assert(false);
				}

				for (const auto& file : dir) {
					auto fHash = detail::file_hasher()(file.str_ref());
					if (fHash != file.hash()) {
						assert(false);
					}
				}
			}

			return true;
		}

		container_type _dirs;
		detail::header_t _header;
	};


	class directory_iterator
	{
	public:
		using value_type = directory;
		using reference = value_type&;
		using pointer = value_type*;

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
				_dirs->push_back(value_type(dir));
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

		std::optional<std::vector<value_type>> _dirs;
		std::size_t _pos;
	};


	class file_iterator
	{
	public:
		using value_type = file;
		using reference = value_type&;
		using pointer = value_type*;

		inline file_iterator() noexcept :
			_files(std::nullopt),
			_pos(NPOS)
		{}

		inline file_iterator(const file_iterator& a_rhs) :
			_files(a_rhs._files),
			_pos(a_rhs._pos)
		{}

		inline file_iterator(file_iterator&& a_rhs) noexcept :
			_files(std::move(a_rhs._files)),
			_pos(std::move(a_rhs._pos))
		{
			a_rhs._pos = NPOS;
		}

		explicit inline file_iterator(const directory& a_directory) :
			_files(std::in_place_t()),
			_pos(0)
		{
			auto& dir = const_cast<directory&>(a_directory);
			for (auto& file : dir.directory_ref()) {
				_files->push_back(value_type(file));
			}
		}

		inline file_iterator& operator=(const file_iterator& a_rhs)
		{
			if (this != &a_rhs) {
				_files = a_rhs._files;
				_pos = a_rhs._pos;
			}
			return *this;
		}

		inline file_iterator& operator=(file_iterator&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				_files = std::move(a_rhs._files);
				_pos = std::move(a_rhs._pos);
				a_rhs._pos = NPOS;
			}
			return *this;
		}

		[[nodiscard]] friend constexpr bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !a_lhs._files && !a_rhs._files; }
		[[nodiscard]] friend constexpr bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

		[[nodiscard]] inline reference operator*() { return fetch(); }
		[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

		inline file_iterator& operator++()
		{
			++_pos;
			if (_pos >= _files->size()) {
				_files.reset();
				_pos = NPOS;
			}
			return *this;
		}

		[[nodiscard]] friend inline file_iterator begin(file_iterator a_iter) noexcept
		{
			return a_iter;
		}

		[[nodiscard]] friend inline file_iterator end([[maybe_unused]] const file_iterator&) noexcept
		{
			return {};
		}

	private:
		inline reference fetch() { return _files.value()[_pos]; }

		static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

		std::optional<std::vector<value_type>> _files;
		std::size_t _pos;
	};
}
