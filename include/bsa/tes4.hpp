#pragma once

#include "bsa/common.hpp"
#include "bsa/stl.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace bsa
{
	namespace tes4	// The Elder Scrolls IV: Oblivion
	{
		using archive_flag = std::uint32_t;	 // BSArchive::ARCHIVE_FLAGS
		BSA_CXX17_INLINE constexpr archive_flag directory_strings_bit{ 1 << 0 };
		BSA_CXX17_INLINE constexpr archive_flag file_strings_bit{ 1 << 1 };
		BSA_CXX17_INLINE constexpr archive_flag compressed_bit{ 1 << 2 };
		BSA_CXX17_INLINE constexpr archive_flag retain_directory_names_bit{ 1 << 3 };
		BSA_CXX17_INLINE constexpr archive_flag retain_file_names_bit{ 1 << 4 };
		BSA_CXX17_INLINE constexpr archive_flag retain_file_name_offsets_bit{ 1 << 5 };
		BSA_CXX17_INLINE constexpr archive_flag xbox_archive_bit{ 1 << 6 };
		BSA_CXX17_INLINE constexpr archive_flag retain_strings_during_startup_bit{ 1 << 7 };
		BSA_CXX17_INLINE constexpr archive_flag embedded_file_names_bit{ 1 << 8 };
		BSA_CXX17_INLINE constexpr archive_flag xbox_compressed_bit{ 1 << 9 };

		using archive_type = std::uint16_t;	 // ARCHIVE_TYPE_INDEX
		BSA_CXX17_INLINE constexpr archive_type meshesbit{ 1 << 0 };
		BSA_CXX17_INLINE constexpr archive_type texturesbit{ 1 << 1 };
		BSA_CXX17_INLINE constexpr archive_type menusbit{ 1 << 2 };
		BSA_CXX17_INLINE constexpr archive_type soundsbit{ 1 << 3 };
		BSA_CXX17_INLINE constexpr archive_type voicesbit{ 1 << 4 };
		BSA_CXX17_INLINE constexpr archive_type shadersbit{ 1 << 5 };
		BSA_CXX17_INLINE constexpr archive_type treesbit{ 1 << 6 };
		BSA_CXX17_INLINE constexpr archive_type fontsbit{ 1 << 7 };
		BSA_CXX17_INLINE constexpr archive_type miscbit{ 1 << 8 };

		using archive_version = std::size_t;
		BSA_CXX17_INLINE constexpr archive_version v103{ 103 };
		BSA_CXX17_INLINE constexpr archive_version v104{ 104 };
		BSA_CXX17_INLINE constexpr archive_version v105{ 105 };


		namespace detail
		{
			using namespace bsa::detail;


			class dir_hasher;
			class directory_t;
			class file_hasher;
			class file_t;
			class hash_t;
			class header_t;


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

				~header_t() = default;

				constexpr header_t& operator=(const header_t& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_block = a_rhs._block;
					}
					return *this;
				}

				constexpr header_t& operator=(header_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_block = std::move(a_rhs._block);
					}
					return *this;
				}

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x24; }

				BSA_NODISCARD constexpr std::size_t directory_count() const noexcept { return narrow_cast<std::size_t>(_block.directoryCount); }
				BSA_NODISCARD constexpr std::size_t directory_names_length() const noexcept { return narrow_cast<std::size_t>(_block.directoryNamesLength); }
				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return narrow_cast<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr std::size_t file_names_length() const noexcept { return narrow_cast<std::size_t>(_block.fileNamesLength); }
				BSA_NODISCARD constexpr archive_flag flags() const noexcept { return narrow_cast<archive_flag>(_block.flags); }
				BSA_NODISCARD constexpr std::size_t header_size() const noexcept { return narrow_cast<std::size_t>(_block.headerSize); }
				BSA_NODISCARD constexpr stl::string_view tag() const { return stl::string_view(_block.tag.data(), _block.tag.size()); }
				BSA_NODISCARD constexpr archive_type types() const noexcept { return narrow_cast<archive_type>(_block.archiveTypes); }
				BSA_NODISCARD constexpr archive_version version() const noexcept { return narrow_cast<archive_version>(_block.version); }

				constexpr archive_flag flags(archive_flag a_flags) noexcept
				{
					assert((a_flags >> 10) == 0);  // all flags?
					auto old = flags();
					_block.flags = a_flags;
					return old;
				}

				constexpr archive_type types(archive_type a_types) noexcept
				{
					assert((a_types >> 9) == 0);
					auto old = types();
					_block.archiveTypes = a_types;
					return old;
				}

				constexpr void version(archive_version a_version) noexcept
				{
					assert(a_version == v103 || a_version == v104 || a_version == v105);
					_block.version = narrow_cast<std::uint32_t>(a_version);
				}

				BSA_NODISCARD constexpr bool compressed() const noexcept { return (flags() & compressed_bit) != 0; }
				BSA_NODISCARD constexpr bool directory_strings() const noexcept { return (flags() & directory_strings_bit) != 0; }
				BSA_NODISCARD constexpr bool embedded_file_names() const noexcept { return (flags() & embedded_file_names_bit) != 0; }
				BSA_NODISCARD constexpr bool file_strings() const noexcept { return (flags() & file_strings_bit) != 0; }
				BSA_NODISCARD constexpr bool retain_directory_names() const noexcept { return (flags() & retain_directory_names_bit) != 0; }
				BSA_NODISCARD constexpr bool retain_file_names() const noexcept { return (flags() & retain_file_names_bit) != 0; }
				BSA_NODISCARD constexpr bool retain_file_name_offsets() const noexcept { return (flags() & retain_file_name_offsets_bit) != 0; }
				BSA_NODISCARD constexpr bool retain_strings_during_startup() const noexcept { return (flags() & retain_strings_during_startup_bit) != 0; }
				BSA_NODISCARD constexpr bool xbox_archive() const noexcept { return (flags() & xbox_archive_bit) != 0; }
				BSA_NODISCARD constexpr bool xbox_compressed() const noexcept { return (flags() & xbox_compressed_bit) != 0; }

				constexpr bool compressed(bool a_set) noexcept { return set_flags(a_set, compressed_bit); }
				constexpr bool directory_strings(bool a_set) noexcept { return set_flags(a_set, directory_strings_bit); }
				constexpr bool embedded_file_names(bool a_set) noexcept { return set_flags(a_set, embedded_file_names_bit); }
				constexpr bool file_strings(bool a_set) noexcept { return set_flags(a_set, file_strings_bit); }
				constexpr bool retain_directory_names(bool a_set) noexcept { return set_flags(a_set, retain_directory_names_bit); }
				constexpr bool retain_file_names(bool a_set) noexcept { return set_flags(a_set, retain_file_names_bit); }
				constexpr bool retain_file_name_offsets(bool a_set) noexcept { return set_flags(a_set, retain_file_name_offsets_bit); }
				constexpr bool retain_strings_during_startup(bool a_set) noexcept { return set_flags(a_set, retain_strings_during_startup_bit); }
				constexpr bool xbox_archive(bool a_set) noexcept { return set_flags(a_set, xbox_archive_bit); }
				constexpr bool xbox_compressed(bool a_set) noexcept { return set_flags(a_set, xbox_compressed_bit); }

				BSA_NODISCARD constexpr bool fonts() const noexcept { return (types() & fontsbit) != 0; }
				BSA_NODISCARD constexpr bool meshes() const noexcept { return (types() & meshesbit) != 0; }
				BSA_NODISCARD constexpr bool menus() const noexcept { return (types() & menusbit) != 0; }
				BSA_NODISCARD constexpr bool misc() const noexcept { return (types() & miscbit) != 0; }
				BSA_NODISCARD constexpr bool shaders() const noexcept { return (types() & shadersbit) != 0; }
				BSA_NODISCARD constexpr bool sounds() const noexcept { return (types() & soundsbit) != 0; }
				BSA_NODISCARD constexpr bool textures() const noexcept { return (types() & texturesbit) != 0; }
				BSA_NODISCARD constexpr bool trees() const noexcept { return (types() & treesbit) != 0; }
				BSA_NODISCARD constexpr bool voices() const noexcept { return (types() & voicesbit) != 0; }

				constexpr bool fonts(bool a_set) noexcept { return set_types(a_set, fontsbit); }
				constexpr bool meshes(bool a_set) noexcept { return set_types(a_set, meshesbit); }
				constexpr bool menus(bool a_set) noexcept { return set_types(a_set, menusbit); }
				constexpr bool misc(bool a_set) noexcept { return set_types(a_set, miscbit); }
				constexpr bool shaders(bool a_set) noexcept { return set_types(a_set, shadersbit); }
				constexpr bool sounds(bool a_set) noexcept { return set_types(a_set, soundsbit); }
				constexpr bool textures(bool a_set) noexcept { return set_types(a_set, texturesbit); }
				constexpr bool trees(bool a_set) noexcept { return set_types(a_set, treesbit); }
				constexpr bool voices(bool a_set) noexcept { return set_types(a_set, voicesbit); }

				constexpr void clear() noexcept { _block = block_t(); }

				inline void read(istream_t& a_input)
				{
					_block.read(a_input);
					if (tag() != BSA) {
						throw input_error();
					}
				}

			private:
				struct block_t	// BSArchiveHeader
				{
					constexpr block_t() noexcept :
						tag{ '\0', '\0', '\0', '\0' },
						version(0),
						headerSize(0),
						flags(0),  // TODO: gotchas with certain bits?
						directoryCount(0),
						fileCount(0),
						directoryNamesLength(0),
						fileNamesLength(0),
						archiveTypes(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						tag(a_rhs.tag),
						version(a_rhs.version),
						headerSize(a_rhs.headerSize),
						flags(a_rhs.flags),
						directoryCount(a_rhs.directoryCount),
						fileCount(a_rhs.fileCount),
						directoryNamesLength(a_rhs.directoryNamesLength),
						fileNamesLength(a_rhs.fileNamesLength),
						archiveTypes(a_rhs.archiveTypes)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						tag(std::move(a_rhs.tag)),
						version(std::move(a_rhs.version)),
						headerSize(std::move(a_rhs.headerSize)),
						flags(std::move(a_rhs.flags)),
						directoryCount(std::move(a_rhs.directoryCount)),
						fileCount(std::move(a_rhs.fileCount)),
						directoryNamesLength(std::move(a_rhs.directoryNamesLength)),
						fileNamesLength(std::move(a_rhs.fileNamesLength)),
						archiveTypes(std::move(a_rhs.archiveTypes))
					{}

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							tag = a_rhs.tag;
							version = a_rhs.version;
							headerSize = a_rhs.headerSize;
							flags = a_rhs.flags;
							directoryCount = a_rhs.directoryCount;
							fileCount = a_rhs.fileCount;
							directoryNamesLength = a_rhs.directoryNamesLength;
							fileNamesLength = a_rhs.fileNamesLength;
							archiveTypes = a_rhs.archiveTypes;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							tag = std::move(a_rhs.tag);
							version = std::move(a_rhs.version);
							headerSize = std::move(a_rhs.headerSize);
							flags = std::move(a_rhs.flags);
							directoryCount = std::move(a_rhs.directoryCount);
							fileCount = std::move(a_rhs.fileCount);
							directoryNamesLength = std::move(a_rhs.directoryNamesLength);
							fileNamesLength = std::move(a_rhs.fileNamesLength);
							archiveTypes = std::move(a_rhs.archiveTypes);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							tag >>
							version >>
							headerSize >>
							flags >>
							directoryCount >>
							fileCount >>
							directoryNamesLength >>
							fileNamesLength >>
							archiveTypes;
						a_input.seek_rel(2);
					}

					std::array<char, 4> tag;
					std::uint32_t version;
					std::uint32_t headerSize;
					std::uint32_t flags;
					std::uint32_t directoryCount;
					std::uint32_t fileCount;
					std::uint32_t directoryNamesLength;
					std::uint32_t fileNamesLength;
					std::uint16_t archiveTypes;
					//uint16_t pad;
				};

				constexpr bool set_flags(bool a_set, archive_flag a_mask) noexcept
				{
					auto old = (flags() & a_mask) == a_mask;
					if (a_set) {
						_block.flags |= a_mask;
					} else {
						_block.flags &= ~a_mask;
					}
					return old;
				}

				constexpr bool set_types(bool a_set, archive_type a_mask) noexcept
				{
					auto old = (types() & a_mask) == a_mask;
					if (a_set) {
						_block.flags |= a_mask;
					} else {
						_block.flags &= ~a_mask;
					}
					return old;
				}

				static constexpr stl::string_view BSA{ "BSA\0", 4 };

				block_t _block;
			};


			class hash_t
			{
			public:
				constexpr hash_t() noexcept :
					_block()
				{}

				constexpr hash_t(const hash_t& a_rhs) noexcept :
					_block(a_rhs._block)
				{}

				constexpr hash_t(hash_t&& a_rhs) noexcept :
					_block(std::move(a_rhs._block))
				{}

				~hash_t() = default;

				constexpr hash_t& operator=(const hash_t& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_block = a_rhs._block;
					}
					return *this;
				}

				constexpr hash_t& operator=(hash_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_block = std::move(a_rhs._block);
					}
					return *this;
				}

				BSA_NODISCARD friend constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() == a_rhs.numeric(); }
				BSA_NODISCARD friend constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

				BSA_NODISCARD friend constexpr bool operator<(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() < a_rhs.numeric(); }
				BSA_NODISCARD friend constexpr bool operator>(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_rhs < a_lhs; }
				BSA_NODISCARD friend constexpr bool operator<=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
				BSA_NODISCARD friend constexpr bool operator>=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x8; }

				BSA_NODISCARD constexpr auto crc() const noexcept { return _block.crc; }
				BSA_NODISCARD constexpr auto first() const noexcept { return _block.first; }
				BSA_NODISCARD constexpr auto last() const noexcept { return _block.last; }
				BSA_NODISCARD constexpr auto last2() const noexcept { return _block.last2; }
				BSA_NODISCARD constexpr auto length() const noexcept { return _block.length; }

				BSA_NODISCARD constexpr std::uint64_t numeric() const noexcept
				{
					return narrow_cast<std::uint64_t>(
						narrow_cast<std::uint64_t>(_block.last) << 0 * byte_v |
						narrow_cast<std::uint64_t>(_block.last2) << 1 * byte_v |
						narrow_cast<std::uint64_t>(_block.length) << 2 * byte_v |
						narrow_cast<std::uint64_t>(_block.first) << 3 * byte_v |
						narrow_cast<std::uint64_t>(_block.crc) << 4 * byte_v);
				}

				inline void read(istream_t& a_input, const header_t& a_header)
				{
					_block.read(a_input, a_header);
				}

			protected:
				friend class dir_hasher;
				friend class file_hasher;

				struct block_t	// BSHash
				{
					constexpr block_t() noexcept :
						last(0),
						last2(0),
						length(0),
						first(0),
						crc(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						last(a_rhs.last),
						last2(a_rhs.last2),
						length(a_rhs.length),
						first(a_rhs.first),
						crc(a_rhs.crc)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						last(std::move(a_rhs.last)),
						last2(std::move(a_rhs.last2)),
						length(std::move(a_rhs.length)),
						first(std::move(a_rhs.first)),
						crc(std::move(a_rhs.crc))
					{}

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							last = a_rhs.last;
							last2 = a_rhs.last2;
							length = a_rhs.length;
							first = a_rhs.first;
							crc = a_rhs.crc;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							last = std::move(a_rhs.last);
							last2 = std::move(a_rhs.last2);
							length = std::move(a_rhs.length);
							first = std::move(a_rhs.first);
							crc = std::move(a_rhs.crc);
						}
						return *this;
					}

					inline void read(istream_t& a_input, const header_t& a_header)
					{
						a_input >>
							last >>
							last2 >>
							length >>
							first;

						if (a_header.xbox_archive()) {
							a_input >> endian::big >> crc >> endian::little;
						} else {
							a_input >> crc;
						}
					}

					std::int8_t last;
					std::int8_t last2;
					std::int8_t length;
					std::int8_t first;
					std::uint32_t crc;
				};

				BSA_NODISCARD constexpr block_t& block_ref() noexcept { return _block; }
				BSA_NODISCARD constexpr const block_t& block_ref() const noexcept { return _block; }

			private:
				block_t _block;
			};
			using hash_ref = std::reference_wrapper<hash_t>;


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

				~file_t() = default;

				inline file_t& operator=(const file_t& a_rhs)
				{
					if (this != std::addressof(a_rhs)) {
						_hash = a_rhs._hash;
						_block = a_rhs._block;
						_name = a_rhs._name;
					}
					return *this;
				}

				inline file_t& operator=(file_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_hash = std::move(a_rhs._hash);
						_block = std::move(a_rhs._block);
						_name = std::move(a_rhs._name);
					}
					return *this;
				}

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x8 + hash_t::block_size(); }

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD constexpr std::size_t offset() const noexcept { return narrow_cast<std::size_t>(_block.offset); }

				BSA_NODISCARD constexpr std::size_t size() const noexcept { return narrow_cast<std::size_t>(_block.size); }

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				inline void read(istream_t& a_input, const header_t& a_header)
				{
					_hash.read(a_input, a_header);
					_block.read(a_input, a_header);
				}

				inline void read_name(istream_t& a_input)
				{
					char ch;
					do {
						a_input.get(ch);
						_name.push_back(ch);
					} while (ch != '\0');
					_name.pop_back();  // discard null terminator
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

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							size = a_rhs.size;
							offset = a_rhs.offset;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							size = std::move(a_rhs.size);
							offset = std::move(a_rhs.offset);
						}
						return *this;
					}

					inline void read(istream_t& a_input, const header_t&)
					{
						a_input >>
							size >>
							offset;
					}

					std::uint32_t size;
					std::uint32_t offset;
				};

				hash_t _hash;
				block_t _block;
				std::string _name;
			};
			using file_ptr = std::shared_ptr<file_t>;


			class directory_t
			{
			public:
				using container_type = std::vector<file_ptr>;
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

				~directory_t() = default;

				inline directory_t& operator=(const directory_t& a_rhs)
				{
					if (this != std::addressof(a_rhs)) {
						_hash = a_rhs._hash;
						_block = a_rhs._block;
						_name = a_rhs._name;
						_files = a_rhs._files;
					}
					return *this;
				}

				inline directory_t& operator=(directory_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_hash = std::move(a_rhs._hash);
						_block = std::move(a_rhs._block);
						_name = std::move(a_rhs._name);
						_files = std::move(a_rhs._files);
					}
					return *this;
				}

				BSA_NODISCARD static constexpr std::size_t block_size(archive_version a_version)
				{
					switch (a_version) {
					case v103:
					case v104:
						return 0x8 + hash_t::block_size();
					case v105:
						return 0x10 + hash_t::block_size();
					default:
						throw version_error();
					}
				}

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return narrow_cast<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr std::size_t file_offset() const noexcept { return narrow_cast<std::size_t>(_block.fileOffset); }

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				BSA_NODISCARD inline iterator begin() noexcept { return _files.begin(); }
				BSA_NODISCARD inline const_iterator begin() const noexcept { return _files.begin(); }
				BSA_NODISCARD inline const_iterator cbegin() const noexcept { return _files.cbegin(); }

				BSA_NODISCARD inline iterator end() noexcept { return _files.end(); }
				BSA_NODISCARD inline const_iterator end() const noexcept { return _files.end(); }
				BSA_NODISCARD inline const_iterator cend() const noexcept { return _files.cend(); }

				BSA_NODISCARD inline bool empty() const noexcept { return _files.empty(); }

				inline void read(istream_t& a_input, const header_t& a_header)
				{
					_hash.read(a_input, a_header);
					_block.read(a_input, a_header);
					if (a_header.directory_strings() || file_count() > 0) {
						read_extra(a_input, a_header);
					}
				}

			private:
				struct block_t	// BSDirectoryEntry
				{
					constexpr block_t() noexcept :
						fileCount(0),
						fileOffset(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						fileCount(a_rhs.fileCount),
						fileOffset(a_rhs.fileOffset)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						fileCount(std::move(a_rhs.fileCount)),
						fileOffset(std::move(a_rhs.fileOffset))
					{}

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = a_rhs.fileCount;
							fileOffset = a_rhs.fileOffset;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = std::move(a_rhs.fileCount);
							fileOffset = std::move(a_rhs.fileOffset);
						}
						return *this;
					}

					inline void read(istream_t& a_input, const header_t& a_header)
					{
						switch (a_header.version()) {
						case v103:
						case v104:
							a_input >>
								fileCount >>
								fileOffset;
							break;
						case v105:

							a_input >> fileCount;
							a_input.seek_rel(4);
							a_input >> fileOffset;
							a_input.seek_rel(4);
							break;
						default:
							throw version_error();
						}
					}

					std::uint32_t fileCount;
					//uint32_t pad;	x64
					std::uint32_t fileOffset;
					//uint32_t pad;	x64
				};

				inline void read_extra(istream_t& a_input, const header_t& a_header)
				{
					restore_point p(a_input);
					a_input.seek_beg(file_offset() - a_header.file_names_length());

					if (a_header.directory_strings()) {
						// bzstring
						std::uint8_t length;
						a_input >> length;
						const auto xLength = narrow_cast<std::size_t>(length) - 1;
						_name.resize(xLength);
						a_input.read(_name.begin(), xLength);
						a_input.seek_rel(1);
					}

					for (std::size_t i = 0; i < file_count(); ++i) {
						auto file = std::make_shared<file_t>();
						file->read(a_input, a_header);
						_files.push_back(std::move(file));
					}
				}

				hash_t _hash;
				block_t _block;
				std::string _name;
				container_type _files;
			};
			using directory_ptr = std::shared_ptr<directory_t>;


			class dir_hasher
			{
			public:
				dir_hasher() = default;
				dir_hasher(const dir_hasher&) = default;
				dir_hasher(dir_hasher&&) = default;

				dir_hasher& operator=(const dir_hasher&) = default;
				dir_hasher& operator=(dir_hasher&&) = default;

				BSA_NODISCARD inline hash_t operator()(stl::string_view a_path) const
				{
					verify_path(a_path);
					auto fullPath = normalize(std::move(a_path));
					return hash(fullPath);
				}

			protected:
				BSA_NODISCARD inline hash_t hash(stl::string_view a_fullPath) const
				{
					constexpr auto LEN_MAX{ narrow_cast<std::size_t>((std::numeric_limits<std::int8_t>::max)()) };

					hash_t hash;
					auto& block = hash.block_ref();
					switch (std::min<std::size_t>(a_fullPath.length(), 3)) {
					case 3:
						block.last2 = a_fullPath[a_fullPath.length() - 2];
						BSA_FALLTHROUGH;
					case 2:
					case 1:
						block.last = a_fullPath.back();
						block.first = a_fullPath.front();
						BSA_FALLTHROUGH;
					default:
						break;
					}

					block.length = narrow_cast<std::int8_t>((std::min)(a_fullPath.length(), LEN_MAX));
					if (block.length <= 3) {
						return hash;
					}

					// skip first and last two chars
					for (auto it = a_fullPath.begin() + 1; it != a_fullPath.end() - 2; ++it) {
						block.crc = *it + block.crc * HASH_CONSTANT;
					}

					return hash;
				}

				inline constexpr void verify_path(const stl::string_view& a_path) const
				{
					for (auto& ch : a_path) {
						if (ch < 0) {
							throw hash_error();
						}
					}
				}

				static constexpr auto HASH_CONSTANT{ narrow_cast<std::uint32_t>(0x1003F) };

			private:
				BSA_NODISCARD inline std::string normalize(stl::string_view a_path) const
				{
					stl::filesystem::path path(a_path.begin(), a_path.end());
					path = path.lexically_normal();

					auto fullPath = path.string();
					for (auto& ch : fullPath) {
						ch = mapchar(ch);
					}
					if (fullPath.empty()) {
						fullPath.push_back('.');
					}
					while (!fullPath.empty() && fullPath.back() == '\\') {
						fullPath.pop_back();
					}
					while (!fullPath.empty() && fullPath.front() == '\\') {
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

				BSA_NODISCARD inline hash_t operator()(stl::string_view a_path) const
				{
					verify_path(a_path);
					std::string stem;
					std::string extension;
					std::tie(stem, extension) = normalize(std::move(a_path));
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

					constexpr extension_t(const char (&a_val)[5]) noexcept :
						c{ '\0' }
					{
						for (std::size_t idx = 0; idx < 4; ++idx) {
							at(c, idx) = at(a_val, idx);
						}
					}

					constexpr extension_t(const stl::string_view& a_val) noexcept :
						c{ '\0' }
					{
						std::size_t idx = 0;
						while (idx < std::min<std::size_t>(a_val.length(), 4)) {
							at(c, idx) = a_val[idx];
							++idx;
						}
						while (idx < 4) {
							at(c, idx) = '\0';
							++idx;
						}
					}

					char c[4];
					std::uint32_t i;
				};

				BSA_NODISCARD inline std::pair<std::string, std::string> normalize(stl::string_view a_path) const
				{
					stl::filesystem::path path(a_path.begin(), a_path.end());
					path = path.lexically_normal();

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

				BSA_NODISCARD inline hash_t hash(stl::string_view a_stem, stl::string_view a_extension) const
				{
					constexpr std::array<extension_t, 6> EXTENSIONS{
						extension_t{ "\0\0\0\0" },
						extension_t{ ".nif" },
						extension_t{ ".kf\0" },
						extension_t{ ".dds" },
						extension_t{ ".wav" },
						extension_t{ ".adp" }
					};

					auto hash = dir_hasher::hash(a_stem);
					auto& block = hash.block_ref();

					std::uint32_t extCRC = 0;
					for (auto& ch : a_extension) {
						extCRC = ch + extCRC * HASH_CONSTANT;
					}
					block.crc += extCRC;

					const extension_t ext(a_extension);
					for (std::uint8_t i = 0; i < EXTENSIONS.size(); ++i) {
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
			hash() = delete;

			inline hash(const hash& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline hash(hash&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			~hash() = default;

			inline hash& operator=(const hash& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline hash& operator=(hash&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD inline std::uint32_t crc() const noexcept { return _impl.get().crc(); }
			BSA_NODISCARD inline char first_char() const noexcept { return static_cast<char>(_impl.get().first()); }
			BSA_NODISCARD inline char last_char() const noexcept { return static_cast<char>(_impl.get().last()); }
			BSA_NODISCARD inline std::int8_t length() const noexcept { return _impl.get().length(); }
			BSA_NODISCARD inline std::uint64_t numeric_hash() const noexcept { return _impl.get().numeric(); }
			BSA_NODISCARD inline char second_to_last_char() const noexcept { return static_cast<char>(_impl.get().last2()); }

		protected:
			friend class directory;
			friend class file;

			explicit inline hash(detail::hash_t& a_rhs) noexcept :
				_impl(std::ref(a_rhs))
			{}

			explicit inline hash(const detail::hash_ref& a_rhs) noexcept :
				_impl(a_rhs)
			{}

		private:
			detail::hash_ref _impl;
		};


		class file
		{
		public:
			file() = delete;

			inline file(const file& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline file(file&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			~file() = default;

			inline file& operator=(const file& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline file& operator=(file&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline hash hash_value() const noexcept { return hash{ _impl->hash_ref() }; }
			BSA_NODISCARD inline std::size_t size() const noexcept { return _impl->size(); }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }

		protected:
			friend class file_iterator;

			using value_type = detail::file_ptr;

			explicit inline file(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};


		class file_iterator
		{
		public:
			using value_type = file;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::input_iterator_tag;

			constexpr file_iterator() noexcept :
				_files(nullptr),
				_pos(NPOS)
			{}

			inline file_iterator(const file_iterator& a_rhs) noexcept :
				_files(a_rhs._files),
				_pos(a_rhs._pos)
			{}

			inline file_iterator(file_iterator&& a_rhs) noexcept :
				_files(std::move(a_rhs._files)),
				_pos(std::move(a_rhs._pos))
			{
				a_rhs._pos = NPOS;
			}

			~file_iterator() = default;

			inline file_iterator& operator=(const file_iterator& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_files = a_rhs._files;
					_pos = a_rhs._pos;
				}
				return *this;
			}

			inline file_iterator& operator=(file_iterator&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_files = std::move(a_rhs._files);
					_pos = std::move(a_rhs._pos);
					a_rhs._pos = NPOS;
				}
				return *this;
			}

			BSA_NODISCARD friend inline bool operator==(
				const file_iterator& a_lhs,
				const file_iterator& a_rhs) noexcept
			{
				return a_lhs._files == a_rhs._files &&
					   a_lhs._pos == a_rhs._pos;
			}

			BSA_NODISCARD friend inline bool operator!=(
				const file_iterator& a_lhs,
				const file_iterator& a_rhs) noexcept
			{
				return !(a_lhs == a_rhs);
			}

			BSA_NODISCARD inline reference operator*() { return fetch(); }
			BSA_NODISCARD inline pointer operator->() { return std::addressof(fetch()); }

			// prefix
			inline file_iterator& operator++() noexcept
			{
				++_pos;
				if (_pos >= _files->size()) {
					_files.reset();
					_pos = NPOS;
				}
				return *this;
			}

			// postfix
			BSA_NODISCARD inline file_iterator operator++(int) noexcept
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

		protected:
			friend class directory;

			explicit inline file_iterator(detail::directory_ptr a_directory) :
				_files(nullptr),
				_pos(NPOS)
			{
				if (a_directory && !a_directory->empty()) {
					_files = std::make_shared<container_type>();
					_pos = 0;
					for (auto& file : *a_directory) {
						_files->push_back(value_type(file));
					}
				}
			}

		private:
			using container_type = std::vector<value_type>;

			BSA_NODISCARD inline reference fetch()
			{
				assert(_files);
				return (*_files)[_pos];
			}

			static constexpr std::size_t NPOS{ (std::numeric_limits<std::size_t>::max)() };

			std::shared_ptr<container_type> _files;
			std::size_t _pos;
		};


		inline void swap(file_iterator& a_lhs, file_iterator& a_rhs) noexcept
		{
			auto tmp = std::move(a_lhs);
			a_lhs = std::move(a_rhs);
			a_rhs = std::move(tmp);
		}


		class directory
		{
		public:
			using iterator = file_iterator;
			using const_iterator = iterator;

			directory() = delete;

			inline directory(const directory& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline directory(directory&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			~directory() = default;

			inline directory& operator=(const directory& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline directory& operator=(directory&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline std::size_t file_count() const noexcept { return _impl->file_count(); }
			BSA_NODISCARD inline hash hash_value() const noexcept { return hash(_impl->hash_ref()); }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }

			BSA_NODISCARD inline iterator begin() const { return iterator(_impl); }
			BSA_NODISCARD inline iterator end() const noexcept { return iterator(); }

		protected:
			friend class archive;
			friend class directory_iterator;

			using value_type = detail::directory_ptr;

			explicit inline directory(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};


		class directory_iterator
		{
		public:
			using value_type = directory;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::input_iterator_tag;

			constexpr directory_iterator() noexcept :
				_dirs(nullptr),
				_pos(NPOS)
			{}

			inline directory_iterator(const directory_iterator& a_rhs) noexcept :
				_dirs(a_rhs._dirs),
				_pos(a_rhs._pos)
			{}

			inline directory_iterator(directory_iterator&& a_rhs) noexcept :
				_dirs(std::move(a_rhs._dirs)),
				_pos(std::move(a_rhs._pos))
			{
				a_rhs._pos = NPOS;
			}

			~directory_iterator() = default;

			inline directory_iterator& operator=(const directory_iterator& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_dirs = a_rhs._dirs;
					_pos = a_rhs._pos;
				}
				return *this;
			}

			inline directory_iterator& operator=(directory_iterator&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_dirs = std::move(a_rhs._dirs);
					_pos = std::move(a_rhs._pos);
					a_rhs._pos = NPOS;
				}
				return *this;
			}

			BSA_NODISCARD friend inline bool operator==(
				const directory_iterator& a_lhs,
				const directory_iterator& a_rhs) noexcept
			{
				return a_lhs._dirs == a_rhs._dirs &&
					   a_lhs._pos == a_rhs._pos;
			}

			BSA_NODISCARD friend inline bool operator!=(
				const directory_iterator& a_lhs,
				const directory_iterator& a_rhs) noexcept
			{
				return !(a_lhs == a_rhs);
			}

			BSA_NODISCARD inline reference operator*() { return fetch(); }
			BSA_NODISCARD inline pointer operator->() { return std::addressof(fetch()); }

			// prefix
			inline directory_iterator& operator++() noexcept
			{
				++_pos;
				if (_pos >= _dirs->size()) {
					_dirs.reset();
					_pos = NPOS;
				}
				return *this;
			}

			// postfix
			BSA_NODISCARD inline directory_iterator operator++(int) noexcept
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

		protected:
			friend class archive;

			template <class InputIt>
			explicit inline directory_iterator(InputIt a_first, InputIt a_last) :
				_dirs(nullptr),
				_pos(NPOS)
			{
				if (a_first != a_last) {
					_dirs = std::make_shared<container_type>();
					_pos = 0;
					do {
						_dirs->push_back(
							value_type(
								static_cast<const detail::directory_ptr&>(*a_first)));
						++a_first;
					} while (a_first != a_last);
				}
			}

		private:
			using container_type = std::vector<value_type>;

			BSA_NODISCARD inline reference fetch()
			{
				assert(_dirs);
				return (*_dirs)[_pos];
			}

			static constexpr std::size_t NPOS{ (std::numeric_limits<std::size_t>::max)() };

			std::shared_ptr<container_type> _dirs;
			std::size_t _pos;
		};


		inline void swap(directory_iterator& a_lhs, directory_iterator& a_rhs) noexcept
		{
			auto tmp = std::move(a_lhs);
			a_lhs = std::move(a_rhs);
			a_rhs = std::move(tmp);
		}


		class archive
		{
		public:
			using iterator = directory_iterator;
			using const_iterator = iterator;

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

			inline archive(const stl::filesystem::path& a_path) :
				_dirs(),
				_header()
			{
				read(a_path);
			}

			~archive() = default;

			inline archive& operator=(const archive& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					_header = a_rhs._header;
					_dirs = a_rhs._dirs;
				}
				return *this;
			}

			inline archive& operator=(archive&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_header = std::move(a_rhs._header);
					_dirs = std::move(a_rhs._dirs);
				}
				return *this;
			}

			friend inline archive& operator>>(archive& a_archive, const stl::filesystem::path& a_path)
			{
				a_archive.read(a_path);
				return a_archive;
			}

			BSA_NODISCARD inline directory front() const noexcept
			{
				assert(!empty());
				return directory(_dirs.front());
			}

			BSA_NODISCARD inline directory back() const noexcept
			{
				assert(!empty());
				return directory(_dirs.back());
			}

			BSA_NODISCARD inline iterator begin() const { return iterator(_dirs.begin(), _dirs.end()); }
			BSA_NODISCARD inline iterator end() const noexcept { return iterator(); }

			BSA_NODISCARD constexpr std::size_t size() const noexcept { return file_count(); }
			BSA_NODISCARD inline bool empty() const noexcept { return _dirs.size() == 0; }	// TODO: not in terms of size

			inline void clear() noexcept
			{
				_dirs.clear();
				_header.clear();
			}

			BSA_NODISCARD constexpr std::size_t directory_count() const noexcept { return _header.directory_count(); }
			BSA_NODISCARD constexpr std::size_t directory_names_length() const noexcept { return _header.directory_names_length(); }
			BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			BSA_NODISCARD constexpr std::size_t file_names_length() const noexcept { return _header.file_names_length(); }
			BSA_NODISCARD constexpr archive_flag flags() const noexcept { return _header.flags(); }
			BSA_NODISCARD constexpr std::size_t header_size() const noexcept { return _header.header_size(); }
			BSA_NODISCARD constexpr stl::string_view tag() const { return _header.tag(); }
			BSA_NODISCARD constexpr archive_type types() const noexcept { return _header.types(); }
			BSA_NODISCARD constexpr archive_version version() const noexcept { return _header.version(); }

			constexpr archive_flag flags(archive_flag a_flags) noexcept { return _header.flags(a_flags); }
			constexpr archive_type types(archive_type a_types) noexcept { return _header.types(a_types); }
			constexpr void version(archive_version a_version) noexcept { return _header.version(a_version); }

			BSA_NODISCARD constexpr bool compressed() const noexcept { return _header.compressed(); }
			BSA_NODISCARD constexpr bool directory_strings() const noexcept { return _header.directory_strings(); }
			BSA_NODISCARD constexpr bool embedded_file_names() const noexcept { return _header.embedded_file_names(); }
			BSA_NODISCARD constexpr bool file_strings() const noexcept { return _header.file_strings(); }
			BSA_NODISCARD constexpr bool retain_directory_names() const noexcept { return _header.retain_directory_names(); }
			BSA_NODISCARD constexpr bool retain_file_names() const noexcept { return _header.retain_file_names(); }
			BSA_NODISCARD constexpr bool retain_file_name_offsets() const noexcept { return _header.retain_file_name_offsets(); }
			BSA_NODISCARD constexpr bool retain_strings_during_startup() const noexcept { return _header.retain_strings_during_startup(); }
			BSA_NODISCARD constexpr bool xbox_archive() const noexcept { return _header.xbox_archive(); }
			BSA_NODISCARD constexpr bool xbox_compressed() const noexcept { return _header.xbox_compressed(); }

			constexpr bool compressed(bool a_set) noexcept { return _header.compressed(a_set); }
			constexpr bool directory_strings(bool a_set) noexcept { return _header.directory_strings(a_set); }
			constexpr bool embedded_file_names(bool a_set) noexcept { return _header.embedded_file_names(a_set); }
			constexpr bool file_strings(bool a_set) noexcept { return _header.file_strings(a_set); }
			constexpr bool retain_directory_names(bool a_set) noexcept { return _header.retain_directory_names(a_set); }
			constexpr bool retain_file_names(bool a_set) noexcept { return _header.retain_file_names(a_set); }
			constexpr bool retain_file_name_offsets(bool a_set) noexcept { return _header.retain_file_name_offsets(a_set); }
			constexpr bool retain_strings_during_startup(bool a_set) noexcept { return _header.retain_strings_during_startup(a_set); }
			constexpr bool xbox_archive(bool a_set) noexcept { return _header.xbox_archive(a_set); }
			constexpr bool xbox_compressed(bool a_set) noexcept { return _header.xbox_compressed(a_set); }

			BSA_NODISCARD constexpr bool fonts() const noexcept { return _header.fonts(); }
			BSA_NODISCARD constexpr bool meshes() const noexcept { return _header.meshes(); }
			BSA_NODISCARD constexpr bool menus() const noexcept { return _header.menus(); }
			BSA_NODISCARD constexpr bool misc() const noexcept { return _header.misc(); }
			BSA_NODISCARD constexpr bool shaders() const noexcept { return _header.shaders(); }
			BSA_NODISCARD constexpr bool sounds() const noexcept { return _header.sounds(); }
			BSA_NODISCARD constexpr bool textures() const noexcept { return _header.textures(); }
			BSA_NODISCARD constexpr bool trees() const noexcept { return _header.trees(); }
			BSA_NODISCARD constexpr bool voices() const noexcept { return _header.voices(); }

			constexpr bool fonts(bool a_set) noexcept { return _header.fonts(a_set); }
			constexpr bool meshes(bool a_set) noexcept { return _header.meshes(a_set); }
			constexpr bool menus(bool a_set) noexcept { return _header.menus(a_set); }
			constexpr bool misc(bool a_set) noexcept { return _header.misc(a_set); }
			constexpr bool shaders(bool a_set) noexcept { return _header.shaders(a_set); }
			constexpr bool sounds(bool a_set) noexcept { return _header.sounds(a_set); }
			constexpr bool textures(bool a_set) noexcept { return _header.textures(a_set); }
			constexpr bool trees(bool a_set) noexcept { return _header.trees(a_set); }
			constexpr bool voices(bool a_set) noexcept { return _header.voices(a_set); }

			inline void read(const stl::filesystem::path& a_path)
			{
				detail::istream_t input(a_path);

				clear();

				_header.read(input);
				switch (_header.version()) {
				case v103:
				case v104:
				case v105:
					break;
				default:
					throw version_error();
				}

				input.seek_beg(_header.header_size());
				for (std::size_t i = 0; i < _header.directory_count(); ++i) {
					auto dir = std::make_shared<detail::directory_t>();
					dir->read(input, _header);
					_dirs.push_back(std::move(dir));
				}

				auto offset = _header.directory_names_length() + _header.directory_count();	 // include prefixed length byte
				offset += _header.file_count() * detail::file_t::block_size();
				input.seek_rel(offset);

				if (_header.file_strings()) {
					for (auto& dir : _dirs) {
						for (auto& file : *dir) {
							file->read_name(input);
						}
					}
				}

				assert(sanity_check());
			}

		private:
			inline bool sanity_check()
			{
				detail::hash_t dHash;
				for (const auto& dir : _dirs) {
					dHash = detail::dir_hasher()(dir->str_ref());
					if (dHash != dir->hash()) {
						assert(false);
					}

					for (const auto& file : *dir) {
						try {
							const auto fHash = detail::file_hasher()(file->str_ref());
							if (fHash != file->hash()) {
								assert(false);
							}
						} catch (const hash_error&) {
							continue;
						}
					}
				}

				return true;
			}

			std::vector<detail::directory_ptr> _dirs;
			detail::header_t _header;
		};
	}
}
