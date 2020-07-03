#pragma once

#include "bsa/common.hpp"
#include "bsa/stl.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace bsa
{
	namespace tes4	// The Elder Scrolls IV: Oblivion
	{
		enum class archive_flag : std::uint32_t	 // BSArchive::ARCHIVE_FLAGS
		{
			none = 0,

			directory_strings = 1 << 0,
			file_strings = 1 << 1,
			compressed = 1 << 2,
			retain_directory_names = 1 << 3,
			retain_file_names = 1 << 4,
			retain_file_name_offsets = 1 << 5,
			xbox_archive = 1 << 6,
			retain_strings_during_startup = 1 << 7,
			embedded_file_names = 1 << 8,
			xbox_compressed = 1 << 9,
			unknown = 1 << 10,

			all =
#if 0
				static_cast<stl::underlying_type_t<archive_flag>>(-1)
#else
				directory_strings |
				file_strings |
				compressed |
				retain_directory_names |
				retain_file_names |
				retain_file_name_offsets |
				xbox_archive |
				retain_strings_during_startup |
				embedded_file_names |
				xbox_compressed |
				unknown
#endif
		};

		BSA_MAKE_ALL_ENUM_OPERATORS(archive_flag);

		enum class archive_type : std::uint16_t	 // ARCHIVE_TYPE_INDEX
		{
			none = 0,

			meshes = 1 << 0,
			textures = 1 << 1,
			menus = 1 << 2,
			sounds = 1 << 3,
			voices = 1 << 4,
			shaders = 1 << 5,
			trees = 1 << 6,
			fonts = 1 << 7,
			misc = 1 << 8,

			all =
#if 0
				static_cast<stl::underlying_type_t<archive_type>>(-1)
#else
				meshes |
				textures |
				menus |
				sounds |
				voices |
				shaders |
				trees |
				fonts |
				misc
#endif
		};

		BSA_MAKE_ALL_ENUM_OPERATORS(archive_type);

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

			class header_t final
			{
			public:
				constexpr header_t() noexcept = default;
				constexpr header_t(const header_t&) noexcept = default;
				constexpr header_t(header_t&&) noexcept = default;

				~header_t() noexcept = default;

				constexpr header_t& operator=(const header_t&) noexcept = default;
				constexpr header_t& operator=(header_t&&) noexcept = default;

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x24; }

				BSA_NODISCARD constexpr std::size_t directory_count() const noexcept { return zero_extend<std::size_t>(_block.directoryCount); }
				BSA_NODISCARD constexpr std::size_t directory_names_length() const noexcept { return zero_extend<std::size_t>(_block.directoryNamesLength); }
				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return zero_extend<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr std::size_t file_names_length() const noexcept { return zero_extend<std::size_t>(_block.fileNamesLength); }
				BSA_NODISCARD constexpr archive_flag flags() const noexcept { return zero_extend<archive_flag>(_block.flags); }
				BSA_NODISCARD constexpr std::size_t header_size() const noexcept { return zero_extend<std::size_t>(_block.headerSize); }
				BSA_NODISCARD constexpr stl::string_view tag() const { return stl::string_view(_block.tag.data(), _block.tag.size()); }
				BSA_NODISCARD constexpr archive_type types() const noexcept { return zero_extend<archive_type>(_block.archiveTypes); }
				BSA_NODISCARD constexpr archive_version version() const noexcept { return zero_extend<archive_version>(_block.version); }

				constexpr void directory_count(std::size_t a_count)
				{
					if (a_count > max_int32) {
						throw size_error{};
					} else {
						_block.directoryCount = zero_extend<std::uint32_t>(a_count);
					}
				}

				constexpr void directory_names_length(std::size_t a_length)
				{
					if (a_length > max_int32) {
						throw size_error{};
					} else {
						_block.directoryNamesLength = zero_extend<std::uint32_t>(a_length);
					}
				}

				constexpr void file_count(std::size_t a_count)
				{
					if (a_count > max_int32) {
						throw size_error{};
					} else {
						_block.fileCount = zero_extend<std::uint32_t>(a_count);
					}
				}

				constexpr void file_names_length(std::size_t a_length)
				{
					if (a_length > max_int32) {
						throw size_error{};
					} else {
						_block.fileNamesLength = zero_extend<std::uint32_t>(a_length);
					}
				}

				constexpr archive_flag flags(archive_flag a_flags)
				{
					if ((a_flags & ~archive_flag::all) != archive_flag::none) {
						throw exception{};
					} else {
						auto old = flags();
						_block.flags = a_flags;
						return old;
					}
				}

				constexpr archive_type types(archive_type a_types)
				{
					if ((a_types & ~archive_type::all) != archive_type::none) {
						throw exception{};
					} else {
						auto old = types();
						_block.archiveTypes = a_types;
						return old;
					}
				}

				constexpr void version(archive_version a_version)
				{
					switch (a_version) {
					case v103:
					case v104:
					case v105:
						_block.version = zero_extend<std::uint32_t>(a_version);
						break;
					default:
						throw exception{};
					}
				}

				BSA_NODISCARD constexpr bool compressed() const noexcept { return (flags() & archive_flag::compressed) != archive_flag::none; }
				BSA_NODISCARD constexpr bool directory_strings() const noexcept { return (flags() & archive_flag::directory_strings) != archive_flag::none; }

				BSA_NODISCARD constexpr bool embedded_file_names() const noexcept
				{
					switch (version()) {
					case v103:
						return false;
					case v104:
					case v105:
						return (flags() & archive_flag::embedded_file_names) != archive_flag::none;
					default:
						return false;
					}
				}

				BSA_NODISCARD constexpr bool file_strings() const noexcept { return (flags() & archive_flag::file_strings) != archive_flag::none; }
				BSA_NODISCARD constexpr bool retain_directory_names() const noexcept { return (flags() & archive_flag::retain_directory_names) != archive_flag::none; }
				BSA_NODISCARD constexpr bool retain_file_names() const noexcept { return (flags() & archive_flag::retain_file_names) != archive_flag::none; }
				BSA_NODISCARD constexpr bool retain_file_name_offsets() const noexcept { return (flags() & archive_flag::retain_file_name_offsets) != archive_flag::none; }
				BSA_NODISCARD constexpr bool retain_strings_during_startup() const noexcept { return (flags() & archive_flag::retain_strings_during_startup) != archive_flag::none; }
				BSA_NODISCARD constexpr bool xbox_archive() const noexcept { return (flags() & archive_flag::xbox_archive) != archive_flag::none; }

				BSA_NODISCARD constexpr bool xbox_compressed() const noexcept
				{
					switch (version()) {
					case v103:
						return false;
					case v104:
					case v105:
						return (flags() & archive_flag::xbox_compressed) != archive_flag::none;
					default:
						return false;
					}
				}

				constexpr bool compressed(bool a_set) noexcept { return set_flags(a_set, archive_flag::compressed); }
				constexpr bool directory_strings(bool a_set) noexcept { return set_flags(a_set, archive_flag::directory_strings); }
				constexpr bool embedded_file_names(bool a_set) noexcept { return set_flags(a_set, archive_flag::embedded_file_names); }
				constexpr bool file_strings(bool a_set) noexcept { return set_flags(a_set, archive_flag::file_strings); }
				constexpr bool retain_directory_names(bool a_set) noexcept { return set_flags(a_set, archive_flag::retain_directory_names); }
				constexpr bool retain_file_names(bool a_set) noexcept { return set_flags(a_set, archive_flag::retain_file_names); }
				constexpr bool retain_file_name_offsets(bool a_set) noexcept { return set_flags(a_set, archive_flag::retain_file_name_offsets); }
				constexpr bool retain_strings_during_startup(bool a_set) noexcept { return set_flags(a_set, archive_flag::retain_strings_during_startup); }
				constexpr bool xbox_archive(bool a_set) noexcept { return set_flags(a_set, archive_flag::xbox_archive); }
				constexpr bool xbox_compressed(bool a_set) noexcept { return set_flags(a_set, archive_flag::xbox_compressed); }

				BSA_NODISCARD constexpr bool fonts() const noexcept { return (types() & archive_type::fonts) != archive_type::none; }
				BSA_NODISCARD constexpr bool meshes() const noexcept { return (types() & archive_type::meshes) != archive_type::none; }
				BSA_NODISCARD constexpr bool menus() const noexcept { return (types() & archive_type::menus) != archive_type::none; }
				BSA_NODISCARD constexpr bool misc() const noexcept { return (types() & archive_type::misc) != archive_type::none; }
				BSA_NODISCARD constexpr bool shaders() const noexcept { return (types() & archive_type::shaders) != archive_type::none; }
				BSA_NODISCARD constexpr bool sounds() const noexcept { return (types() & archive_type::sounds) != archive_type::none; }
				BSA_NODISCARD constexpr bool textures() const noexcept { return (types() & archive_type::textures) != archive_type::none; }
				BSA_NODISCARD constexpr bool trees() const noexcept { return (types() & archive_type::trees) != archive_type::none; }
				BSA_NODISCARD constexpr bool voices() const noexcept { return (types() & archive_type::voices) != archive_type::none; }

				constexpr bool fonts(bool a_set) noexcept { return set_types(a_set, archive_type::fonts); }
				constexpr bool meshes(bool a_set) noexcept { return set_types(a_set, archive_type::meshes); }
				constexpr bool menus(bool a_set) noexcept { return set_types(a_set, archive_type::menus); }
				constexpr bool misc(bool a_set) noexcept { return set_types(a_set, archive_type::misc); }
				constexpr bool shaders(bool a_set) noexcept { return set_types(a_set, archive_type::shaders); }
				constexpr bool sounds(bool a_set) noexcept { return set_types(a_set, archive_type::sounds); }
				constexpr bool textures(bool a_set) noexcept { return set_types(a_set, archive_type::textures); }
				constexpr bool trees(bool a_set) noexcept { return set_types(a_set, archive_type::trees); }
				constexpr bool voices(bool a_set) noexcept { return set_types(a_set, archive_type::voices); }

				constexpr void clear() noexcept { _block = block_t(); }

				inline void read(istream_t& a_input)
				{
					_block.read(a_input);
					if (tag() != BSA) {
						throw input_error();
					}
				}

				inline void write(ostream_t& a_output) const
				{
					_block.write(a_output, directory_strings(), file_strings());
				}

			private:
				struct block_t final  // BSArchiveHeader
				{
					constexpr block_t() noexcept :
						tag{ '\0', '\0', '\0', '\0' },
						version(0),
						headerSize(zero_extend<std::uint32_t>(block_size())),
						flags(archive_flag::none),	// TODO: gotchas with certain bits?
						directoryCount(0),
						fileCount(0),
						directoryNamesLength(0),
						fileNamesLength(0),
						archiveTypes(archive_type::none)
#ifdef BSA_PRESERVE_PADDING
						,
						pad(0)
#endif
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

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
#ifdef BSA_PRESERVE_PADDING
						a_input >> pad;
#else
						a_input.seek_rel(2);
#endif
					}

					inline void write(ostream_t& a_output, bool a_directoryStrings, bool a_fileStrings) const
					{
						const auto myDirectoryNamesLength = a_directoryStrings ? directoryNamesLength : 0;
						const auto myFileNamesLength = a_fileStrings ? fileNamesLength : 0;

						a_output
							<< tag
							<< version
							<< headerSize
							<< flags
							<< directoryCount
							<< fileCount
							<< myDirectoryNamesLength
							<< myFileNamesLength
							<< archiveTypes;

#ifndef BSA_PRESERVE_PADDING
						constexpr std::uint16_t pad{ 0 };
#endif
						a_output << pad;
					}

					std::array<char, 4> tag;
					std::uint32_t version;
					std::uint32_t headerSize;
					archive_flag flags;
					std::uint32_t directoryCount;
					std::uint32_t fileCount;
					std::uint32_t directoryNamesLength;
					std::uint32_t fileNamesLength;
					archive_type archiveTypes;
#ifdef BSA_PRESERVE_PADDING
					std::uint16_t pad;
#endif
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
						_block.archiveTypes |= a_mask;
					} else {
						_block.archiveTypes &= ~a_mask;
					}
					return old;
				}

				static constexpr stl::string_view BSA{ "BSA\0", 4 };

				block_t _block;
			};

			class hash_t final
			{
			public:
				constexpr hash_t() noexcept = default;
				constexpr hash_t(const hash_t&) noexcept = default;
				constexpr hash_t(hash_t&&) noexcept = default;

				~hash_t() noexcept = default;

				constexpr hash_t& operator=(const hash_t&) noexcept = default;
				constexpr hash_t& operator=(hash_t&&) noexcept = default;

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x8; }

				BSA_NODISCARD constexpr auto crc() const noexcept { return _block.crc; }
				BSA_NODISCARD constexpr auto first() const noexcept { return _block.first; }
				BSA_NODISCARD constexpr auto last() const noexcept { return _block.last; }
				BSA_NODISCARD constexpr auto last2() const noexcept { return _block.last2; }
				BSA_NODISCARD constexpr auto length() const noexcept { return _block.length; }

				BSA_NODISCARD constexpr std::uint64_t numeric() const noexcept
				{
					return zero_extend<std::uint64_t>(
						zero_extend<std::uint64_t>(_block.last) << 0 * byte_v |
						zero_extend<std::uint64_t>(_block.last2) << 1 * byte_v |
						zero_extend<std::uint64_t>(_block.length) << 2 * byte_v |
						zero_extend<std::uint64_t>(_block.first) << 3 * byte_v |
						zero_extend<std::uint64_t>(_block.crc) << 4 * byte_v);
				}

				inline void read(istream_t& a_input, const header_t& a_header) { _block.read(a_input, a_header); }
				inline void write(ostream_t& a_output, const header_t& a_header) const { _block.write(a_output, a_header); }

			protected:
				friend class dir_hasher;
				friend class file_hasher;

				struct block_t final  // BSHash
				{
					constexpr block_t() noexcept :
						last(0),
						last2(0),
						length(0),
						first(0),
						crc(0)
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

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

					inline void write(ostream_t& a_output, const header_t& a_header) const
					{
						a_output
							<< last
							<< last2
							<< length
							<< first;

						if (a_header.xbox_archive()) {
							a_output << endian::big << crc << endian::little;
						} else {
							a_output << crc;
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

			BSA_NODISCARD constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() == a_rhs.numeric(); }
			BSA_NODISCARD constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			BSA_NODISCARD constexpr bool operator<(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() < a_rhs.numeric(); }
			BSA_NODISCARD constexpr bool operator>(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_rhs < a_lhs; }
			BSA_NODISCARD constexpr bool operator<=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
			BSA_NODISCARD constexpr bool operator>=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

			class file_t final
			{
			public:
				file_t() noexcept = default;
				file_t(const file_t&) = default;
				file_t(file_t&&) noexcept = default;

				~file_t() = default;

				file_t& operator=(const file_t&) = default;
				file_t& operator=(file_t&&) noexcept = default;

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x8 + hash_t::block_size(); }

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

				BSA_NODISCARD constexpr bool compressed() const noexcept { return _block.compressed; }

				BSA_NODISCARD constexpr bool empty() const noexcept
				{
					switch (_data.index()) {
					case iview:
					case ifile:
						return false;
					case inull:
					default:
						return true;
					}
				}

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD BSA_CXX20_CONSTEXPR std::size_t name_size() const noexcept { return _name.size() + 1; }

				BSA_NODISCARD constexpr std::size_t offset() const noexcept { return zero_extend<std::size_t>(_block.offset); }

				constexpr void offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.offset = zero_extend<std::uint32_t>(a_offset);
					}
				}

				BSA_NODISCARD constexpr std::size_t size() const noexcept { return zero_extend<std::size_t>(_block.size); }

				BSA_NODISCARD constexpr std::size_t uncompressed_size() const noexcept
				{
					return _uncompressedSize ?
							   zero_extend<std::size_t>(*_uncompressedSize) :
							   zero_extend<std::size_t>(_block.size);
				}

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				BSA_NODISCARD inline stl::span<const stl::byte> get_data() const
				{
					switch (_data.index()) {
					case iview:
						return stl::get<iview>(_data);
					case ifile:
						return stl::get<ifile>(_data).subspan();
					case iarchive:
						return stl::get<iarchive>(_data).first;
					case inull:
						return {};
					default:
						throw exception();
					}
				}

				inline void set_data(stl::span<const stl::byte> a_data, bool a_compressed)
				{
					if (a_data.size() > max_int32) {
						throw size_error();
					} else {
						_data.emplace<iview>(std::move(a_data));
						_block.size = zero_extend<std::uint32_t>(a_data.size());
						_block.compressed = a_compressed;

						if (a_compressed) {
							_uncompressedSize.emplace(_block.size);	 // TODO: Get actual uncompressed size
						} else {
							_uncompressedSize.reset();
						}
					}
				}

				inline void set_data(istream_t a_input, bool a_compressed)
				{
					if (a_input.size() > max_int32) {
						throw size_error();
					} else {
						_data.emplace<ifile>(std::move(a_input));
						_block.size = zero_extend<std::uint32_t>(a_input.size());
						_block.compressed = a_compressed;

						if (a_compressed) {
							_uncompressedSize.emplace(_block.size);	 // TODO: Get actual uncompressed size
						} else {
							_uncompressedSize.reset();
						}
					}
				}

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

				inline void read_data(istream_t& a_input, const header_t& a_header)
				{
					const restore_point p(a_input);

					a_input.seek_abs(offset());

					if (a_header.embedded_file_names()) {
						std::uint8_t len{ 0 };
						a_input >> len;
						a_input.seek_rel(len);
						_block.size -= (std::min)(_block.size, zero_extend<std::uint32_t>(len + 1));
					}

					if (compressed()) {
						std::uint32_t t{ 0 };
						a_input >> t;
						_uncompressedSize.emplace(t);
						_block.size -= (std::min<std::uint32_t>)(_block.size, 4);
					}

					_data.emplace<iarchive>(
						a_input.subspan(size()),
						a_input);
				}

				inline void extract(std::ostream& a_file)
				{
					const auto data = get_data();
					if (!data.empty()) {
						const auto ssize = zero_extend<std::streamsize>(size());
						a_file.write(reinterpret_cast<const char*>(data.data()), ssize);
					} else {
						throw output_error();
					}

					if (!a_file) {
						throw output_error();
					}
				}

				inline void write(ostream_t& a_output, const header_t& a_header, std::size_t a_dirLength) const
				{
					_hash.write(a_output, a_header);
					_block.write(a_output, a_header, a_dirLength, _name.length());
				}

				inline void write_name(ostream_t& a_output) const
				{
					a_output << stl::string_view{ _name.data(), name_size() };
				}

				inline void write_data(ostream_t& a_output, const header_t& a_header, const std::string& a_dirPath) const
				{
					if (a_header.embedded_file_names()) {  // bstring
						std::size_t length = a_dirPath.length();
						length += 1;  // directory separator
						length += _name.length();
						a_output << zero_extend<std::uint8_t>(length);
						a_output << stl::string_view{ a_dirPath };
						a_output << '\\';
						a_output << stl::string_view{ _name };
					}

					if (compressed()) {
						if (!_uncompressedSize) {
							throw output_error();
						} else {
							a_output << *_uncompressedSize;
						}
					}

					const auto data = get_data();
					a_output << data;
				}

			private:
				enum : std::size_t
				{
					inull,
					iview,
					ifile,
					iarchive
				};

				using null_type = stl::monostate;
				using view_type = stl::span<const stl::byte>;
				using file_type = istream_t;
				using archive_type = std::pair<stl::span<const stl::byte>, istream_t>;

				struct block_t final  // BSFileEntry
				{
					enum : std::uint32_t
					{
						icompression = static_cast<std::uint32_t>(1) << 30,
						ichecked = static_cast<std::uint32_t>(1) << 31
					};

					constexpr block_t() noexcept :
						size(0),
						offset(0),
						compressed(false)
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

					inline void read(istream_t& a_input, const header_t& a_header)
					{
						a_input >>
							size >>
							offset;

						compressed = size & icompression ? !a_header.compressed() : a_header.compressed();
						size &= ~(icompression | ichecked);
					}

					inline void write(ostream_t& a_output, const header_t& a_header, std::size_t a_dirLength, std::size_t a_fileLength) const
					{
						std::uint32_t mySize = size;
						if (a_header.embedded_file_names()) {
							mySize += 1;  // prefixed length
							mySize += zero_extend<std::uint32_t>(a_dirLength);
							mySize += 1;  // directory separator
							mySize += zero_extend<std::uint32_t>(a_fileLength);
						}

						if (compressed) {
							mySize += 4;  // uncompressed size
						}

						if (compressed != a_header.compressed()) {
							mySize |= icompression;
						}

						a_output
							<< mySize
							<< offset;
					}

					std::uint32_t size;
					std::uint32_t offset;
					bool compressed;
				};

				hash_t _hash;
				block_t _block;
				std::string _name;
				stl::variant<null_type, view_type, file_type, archive_type> _data;
				stl::optional<std::uint32_t> _uncompressedSize;	 // TODO: size() == compressed or uncompressed size?
			};
			using file_ptr = std::shared_ptr<file_t>;

			class directory_t final
			{
			public:
				using container_type = std::vector<file_ptr>;
				using iterator = typename container_type::iterator;
				using const_iterator = typename container_type::const_iterator;

				directory_t() noexcept = default;

				directory_t(const directory_t&) = default;
				directory_t(directory_t&&) noexcept = default;

				~directory_t() = default;

				directory_t& operator=(const directory_t&) = default;
				directory_t& operator=(directory_t&&) noexcept = default;

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

				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return zero_extend<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr std::size_t file_offset() const noexcept { return zero_extend<std::size_t>(_block.fileOffset); }

				constexpr void file_offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.fileOffset = zero_extend<std::uint32_t>(a_offset);
					}
				}

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD BSA_CXX20_CONSTEXPR std::size_t name_size() const noexcept { return _name.size() + 1; }

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				BSA_NODISCARD inline iterator begin() noexcept { return _files.begin(); }
				BSA_NODISCARD inline const_iterator begin() const noexcept { return _files.begin(); }
				BSA_NODISCARD inline const_iterator cbegin() const noexcept { return _files.cbegin(); }

				BSA_NODISCARD inline iterator end() noexcept { return _files.end(); }
				BSA_NODISCARD inline const_iterator end() const noexcept { return _files.end(); }
				BSA_NODISCARD inline const_iterator cend() const noexcept { return _files.cend(); }

				BSA_NODISCARD inline bool empty() const noexcept { return _files.empty(); }

				inline void sort() { std::sort(_files.begin(), _files.end(), file_sorter()); }

				inline void read(istream_t& a_input, const header_t& a_header)
				{
					_hash.read(a_input, a_header);
					_block.read(a_input, a_header);
					if (a_header.directory_strings() || file_count() > 0) {
						read_extra(a_input, a_header);
					}
				}

				inline void read_file_names(istream_t& a_input)
				{
					for (auto& file : _files) {
						file->read_name(a_input);
					}
				}

				inline void read_file_data(istream_t& a_input, const header_t& a_header)
				{
					for (auto& file : _files) {
						file->read_data(a_input, a_header);
					}
				}

				inline void write(ostream_t& a_output, const header_t& a_header) const
				{
					_hash.write(a_output, a_header);
					_block.write(a_output, a_header);
				}

				inline void write_extra(ostream_t& a_output, const header_t& a_header) const
				{
					if (a_header.directory_strings()) {
						const auto len = name_size();
						a_output << zero_extend<std::uint8_t>(len);
						a_output << stl::string_view{ _name.c_str(), len };
					}

					for (const auto& file : _files) {
						file->write(a_output, a_header, _name.length());
					}
				}

				inline void write_file_names(ostream_t& a_output) const
				{
					for (const auto& file : _files) {
						file->write_name(a_output);
					}
				}

				inline void write_file_data(ostream_t& a_output, const header_t& a_header) const
				{
					for (const auto& file : _files) {
						file->write_data(a_output, a_header, _name);
					}
				}

			private:
				class file_sorter final
				{
				public:
					using value_type = detail::file_ptr;

					BSA_NODISCARD inline bool operator()(const value_type& a_lhs, const value_type& a_rhs) const noexcept
					{
						return a_lhs->hash_ref() < a_rhs->hash_ref();
					}

					BSA_NODISCARD inline bool operator()(const value_type& a_lhs, const detail::hash_t& a_rhs) const noexcept
					{
						return a_lhs->hash_ref() < a_rhs;
					}
				};

				struct block_t final  // BSDirectoryEntry
				{
					constexpr block_t() noexcept :
						fileCount(0),
#ifdef BSA_PRESERVE_PADDING
						pad1(0),
#endif
						fileOffset(0)
#ifdef BSA_PRESERVE_PADDING
						,
						pad2(0)
#endif
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

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
#ifdef BSA_PRESERVE_PADDING
							a_input >> pad1;
#else
							a_input.seek_rel(4);
#endif
							a_input >> fileOffset;
#ifdef BSA_PRESERVE_PADDING
							a_input >> pad2;
#else
							a_input.seek_rel(4);
#endif
							break;
						default:
							throw version_error();
						}
					}

					inline void write(ostream_t& a_output, const header_t& a_header) const
					{
#ifndef BSA_PRESERVE_PADDING
						constexpr std::uint32_t pad1{ 0 };
						constexpr std::uint32_t pad2{ 0 };
#endif
						switch (a_header.version()) {
						case v103:
						case v104:
							a_output
								<< fileCount
								<< fileOffset;
							break;
						case v105:
							a_output
								<< fileCount
								<< pad1
								<< fileOffset
								<< pad2;
							break;
						default:
							throw output_error();
						}
					}

					std::uint32_t fileCount;
#ifdef BSA_PRESERVE_PADDING
					std::uint32_t pad1;	 // x64
#endif
					std::uint32_t fileOffset;
#ifdef BSA_PRESERVE_PADDING
					std::uint32_t pad2;	 // x64
#endif
				};

				inline void read_extra(istream_t& a_input, const header_t& a_header)
				{
					const restore_point p(a_input);
					a_input.seek_beg(file_offset() - a_header.file_names_length());

					if (a_header.directory_strings()) {
						std::uint8_t length;
						a_input >> length;
						const auto xLength = zero_extend<std::size_t>(length) - 1;	// skip null terminator
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
				std::string _name;	// bzstring
				container_type _files;
			};
			using directory_ptr = std::shared_ptr<directory_t>;

			class dir_hasher final
			{
			public:
				constexpr dir_hasher() noexcept = default;
				constexpr dir_hasher(const dir_hasher&) noexcept = default;
				constexpr dir_hasher(dir_hasher&&) noexcept = default;

				~dir_hasher() noexcept = default;

				constexpr dir_hasher& operator=(const dir_hasher&) noexcept = default;
				constexpr dir_hasher& operator=(dir_hasher&&) noexcept = default;

				BSA_NODISCARD inline hash_t operator()(stl::string_view a_path) const
				{
					verify_path(a_path);
					auto fullPath = normalize(std::move(a_path));
					return hash(fullPath);
				}

			protected:
				friend class file_hasher;

				BSA_NODISCARD inline hash_t hash(stl::string_view a_fullPath) const
				{
					constexpr auto LEN_MAX{
						zero_extend<std::size_t>(
							(std::numeric_limits<std::int8_t>::max)())
					};

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

					block.length =
						zero_extend<std::int8_t>(
							(std::min)(a_fullPath.length(), LEN_MAX));
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
							throw hash_non_ascii();
						}
					}
				}

				static constexpr auto HASH_CONSTANT{ zero_extend<std::uint32_t>(0x1003F) };

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

			class file_hasher final
			{
			public:
				constexpr file_hasher() noexcept = default;
				constexpr file_hasher(const file_hasher&) noexcept = default;
				constexpr file_hasher(file_hasher&&) noexcept = default;

				~file_hasher() noexcept = default;

				constexpr file_hasher& operator=(const file_hasher&) noexcept = default;
				constexpr file_hasher& operator=(file_hasher&&) noexcept = default;

				BSA_NODISCARD inline hash_t operator()(stl::string_view a_path) const
				{
					_dirHasher.verify_path(a_path);
					std::string stem;
					std::string extension;
					std::tie(stem, extension) = normalize(std::move(a_path));
					return hash(stem, extension);
				}

			private:
				BSA_NODISCARD static constexpr std::uint32_t make_extension(stl::string_view a_val) noexcept
				{
					using integer_t =
						stl::conditional_t<
							(sizeof(std::uint32_t) > sizeof(unsigned int)),
							std::uint32_t,
							unsigned int>;

					integer_t tmp = 0;
					for (std::size_t i = 0; i < (std::min)(a_val.size(), sizeof(std::uint32_t)); ++i) {
						tmp |= zero_extend<integer_t>(a_val[i]) << i * byte_v;
					}

					return zero_extend<std::uint32_t>(tmp);
				}

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
					constexpr std::array<std::uint32_t, 6> EXTENSIONS{
						make_extension(""),
						make_extension(".nif"),
						make_extension(".kf"),
						make_extension(".dds"),
						make_extension(".wav"),
						make_extension(".adp")
					};

					auto hash = _dirHasher.hash(a_stem);
					auto& block = hash.block_ref();

					std::uint32_t extCRC = 0;
					for (auto& ch : a_extension) {
						extCRC = ch + extCRC * dir_hasher::HASH_CONSTANT;
					}
					block.crc += extCRC;

					const auto ext = make_extension(a_extension);
					for (std::uint8_t i = 0; i < EXTENSIONS.size(); ++i) {
						if (ext == EXTENSIONS[i]) {
							block.first += 32 * (i & 0xFC);
							block.last += (i & 0xFE) << 6;
							block.last2 += i << 7;
							break;
						}
					}

					return hash;
				}

				BSA_NO_UNIQUE_ADDRESS dir_hasher _dirHasher;
			};
		}

		class archive;
		class directory;
		class directory_iterator;
		class file;
		class file_iterator;
		class hash;

		class hash final
		{
		public:
			hash() = delete;

			hash(const hash&) noexcept = default;
			hash(hash&&) noexcept = default;

			~hash() noexcept = default;

			hash& operator=(const hash&) noexcept = default;
			hash& operator=(hash&&) noexcept = default;

			BSA_NODISCARD inline std::uint32_t crc() const noexcept { return _impl.get().crc(); }
			BSA_NODISCARD inline char first_char() const noexcept { return static_cast<char>(_impl.get().first()); }
			BSA_NODISCARD inline char last_char() const noexcept { return static_cast<char>(_impl.get().last()); }
			BSA_NODISCARD inline std::int8_t length() const noexcept { return _impl.get().length(); }
			BSA_NODISCARD inline std::uint64_t numeric_hash() const noexcept { return _impl.get().numeric(); }
			BSA_NODISCARD inline char second_to_last_char() const noexcept { return static_cast<char>(_impl.get().last2()); }

			inline void swap(hash& a_rhs) noexcept { std::swap(*this, a_rhs); }

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

		inline void swap(hash& a_lhs, hash& a_rhs) { a_lhs.swap(a_rhs); }

		class file final
		{
		public:
			file() = delete;
			file(const file&) noexcept = default;
			file(file&&) noexcept = default;

			~file() noexcept = default;

			file& operator=(const file&) noexcept = default;
			file& operator=(file&&) noexcept = default;

			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline tes4::hash hash() const noexcept { return tes4::hash{ _impl->hash_ref() }; }
			BSA_NODISCARD inline std::size_t size() const noexcept { return _impl->size(); }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }

			inline void swap(file& a_rhs) noexcept { std::swap(*this, a_rhs); }

		protected:
			friend class file_iterator;

			using value_type = detail::file_ptr;

			explicit inline file(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};

		inline void swap(file& a_lhs, file& a_rhs) { a_lhs.swap(a_rhs); }

		class file_iterator final
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

			file_iterator(const file_iterator&) noexcept = default;

			inline file_iterator(file_iterator&& a_rhs) noexcept :
				_files(std::move(a_rhs._files)),
				_pos(std::move(a_rhs._pos))
			{
				a_rhs._pos = NPOS;
			}

			~file_iterator() noexcept = default;

			file_iterator& operator=(const file_iterator&) noexcept = default;

			inline file_iterator& operator=(file_iterator&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_files = std::move(a_rhs._files);
					_pos = std::move(a_rhs._pos);
					a_rhs._pos = NPOS;
				}
				return *this;
			}

			friend bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept;

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

			inline void swap(file_iterator& a_rhs) noexcept { std::swap(*this, a_rhs); }

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

		BSA_NODISCARD inline bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept
		{
			return a_lhs._files == a_rhs._files &&
				   a_lhs._pos == a_rhs._pos;
		}

		BSA_NODISCARD inline bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

		inline void swap(file_iterator& a_lhs, file_iterator& a_rhs) noexcept { a_lhs.swap(a_rhs); }

		class directory final
		{
		public:
			using iterator = file_iterator;
			using const_iterator = iterator;

			directory() = delete;
			directory(const directory&) noexcept = default;
			directory(directory&&) noexcept = default;

			~directory() noexcept = default;

			directory& operator=(const directory&) noexcept = default;
			directory& operator=(directory&&) noexcept = default;

			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline std::size_t file_count() const noexcept { return _impl->file_count(); }
			BSA_NODISCARD inline tes4::hash hash() const noexcept { return tes4::hash{ _impl->hash_ref() }; }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }

			BSA_NODISCARD inline iterator begin() const { return iterator(_impl); }
			BSA_NODISCARD inline iterator end() const noexcept { return iterator(); }

			inline void swap(directory& a_rhs) noexcept { std::swap(*this, a_rhs); }

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

		inline void swap(directory& a_lhs, directory& a_rhs) noexcept { a_lhs.swap(a_rhs); }

		class directory_iterator final
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

			directory_iterator(const directory_iterator&) noexcept = default;

			inline directory_iterator(directory_iterator&& a_rhs) noexcept :
				_dirs(std::move(a_rhs._dirs)),
				_pos(std::move(a_rhs._pos))
			{
				a_rhs._pos = NPOS;
			}

			~directory_iterator() noexcept = default;

			directory_iterator& operator=(const directory_iterator&) noexcept = default;

			inline directory_iterator& operator=(directory_iterator&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_dirs = std::move(a_rhs._dirs);
					_pos = std::move(a_rhs._pos);
					a_rhs._pos = NPOS;
				}
				return *this;
			}

			friend inline bool operator==(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept;

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

			inline void swap(directory_iterator& a_rhs) noexcept { std::swap(*this, a_rhs); }

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

		BSA_NODISCARD inline bool operator==(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept
		{
			return a_lhs._dirs == a_rhs._dirs &&
				   a_lhs._pos == a_rhs._pos;
		}

		BSA_NODISCARD inline bool operator!=(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

		inline void swap(directory_iterator& a_lhs, directory_iterator& a_rhs) noexcept { a_lhs.swap(a_rhs); }

		class archive final
		{
		public:
			using iterator = directory_iterator;
			using const_iterator = iterator;

			archive() noexcept = default;
			archive(const archive&) = default;
			archive(archive&&) noexcept = default;

			inline archive(const stl::filesystem::path& a_path) :
				_dirs(),
				_header()
			{
				read(a_path);
			}

			~archive() = default;

			archive& operator=(const archive&) = default;
			archive& operator=(archive&&) noexcept = default;

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

			constexpr archive_flag flags(archive_flag a_flags) { return _header.flags(a_flags); }
			constexpr archive_type types(archive_type a_types) { return _header.types(a_types); }
			constexpr void version(archive_version a_version) { return _header.version(a_version); }

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
				detail::istream_t input{ a_path };

				clear();

				_header.read(input);
				switch (version()) {
				case v103:
				case v104:
				case v105:
					break;
				default:
					throw version_error();
				}

				input.seek_beg(header_size());
				for (std::size_t i = 0; i < directory_count(); ++i) {
					const auto dir = std::make_shared<detail::directory_t>();
					dir->read(input, _header);
					_dirs.push_back(std::move(dir));
				}

				auto offset = directory_names_length() + directory_count();	 // include prefixed length byte
				offset += file_count() * detail::file_t::block_size();
				input.seek_rel(offset);

				if (file_strings()) {
					for (const auto& dir : _dirs) {
						dir->read_file_names(input);
					}
				}

				for (const auto& dir : _dirs) {
					dir->read_file_data(input, _header);
				}

				sort();

				assert(sanity_check());
			}

			inline void write(const stl::filesystem::path& a_path)
			{
				std::ofstream file{ a_path.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc };
				if (!file.is_open()) {
					throw output_error{};
				} else {
					write(file);
				}
			}

			inline void write(std::ostream& a_output)
			{
				detail::ostream_t output{ a_output };

				prepare_for_write();

				_header.write(output);

				for (const auto& dir : _dirs) {
					dir->write(output, _header);
				}

				for (const auto& dir : _dirs) {
					dir->write_extra(output, _header);
				}

				if (file_strings()) {
					for (const auto& dir : _dirs) {
						dir->write_file_names(output);
					}
				}

				for (const auto& dir : _dirs) {
					dir->write_file_data(output, _header);
				}
			}

		private:
			using container_t = std::vector<detail::directory_ptr>;
			using iterator_t = typename container_t::iterator;

			class directory_sorter final
			{
			public:
				using value_type = detail::directory_ptr;

				BSA_NODISCARD inline bool operator()(const value_type& a_lhs, const value_type& a_rhs) const noexcept
				{
					return a_lhs->hash_ref() < a_rhs->hash_ref();
				}

				BSA_NODISCARD inline bool operator()(const value_type& a_lhs, const detail::hash_t& a_rhs) const noexcept
				{
					return a_lhs->hash_ref() < a_rhs;
				}
			};

			inline iterator_t binary_find(const detail::hash_t& a_hash)
			{
				auto it = _dirs.begin();
				const auto itEnd = _dirs.end();
				it = std::lower_bound(it, itEnd, a_hash, directory_sorter());

				return it != _dirs.end() && (*it)->hash_ref() == a_hash ? it : itEnd;
			}

			BSA_NODISCARD inline std::size_t calc_directory_count() const noexcept
			{
				return _dirs.size();
			}

			BSA_NODISCARD inline std::size_t calc_directory_names_length() const noexcept
			{
				std::size_t length = 0;
				for (const auto& dir : _dirs) {
					length += dir->name_size();
				}
				return length;
			}

			BSA_NODISCARD inline std::size_t calc_file_count() const noexcept
			{
				std::size_t count = 0;
				for (const auto& dir : _dirs) {
					count += dir->file_count();
				}
				return count;
			}

			BSA_NODISCARD inline std::size_t calc_file_names_length() const noexcept
			{
				std::size_t length = 0;
				for (const auto& dir : _dirs) {
					for (const auto& file : *dir) {
						length += file->name_size();
					}
				}
				return length;
			}

			inline void prepare_for_write()
			{
				update_header();
				update_directories();
				update_files();
			}

			inline void sort()
			{
				std::sort(_dirs.begin(), _dirs.end(), directory_sorter());
				for (auto& dir : _dirs) {
					dir->sort();
				}
			}

			inline bool sanity_check()
			{
				detail::hash_t dHash;
				for (const auto& dir : _dirs) {
					dHash = detail::dir_hasher()(dir->str_ref());
					if (dHash != dir->hash()) {
						return false;
					}

					for (const auto& file : *dir) {
						try {
							const auto fHash = detail::file_hasher()(file->str_ref());
							if (fHash != file->hash()) {
								return false;
							}
						} catch (const hash_non_ascii&) {
							continue;
						}
					}
				}

				return true;
			}

			inline void update_directories()
			{
				std::size_t offset{ 0 };
				offset += file_names_length();
				offset += detail::header_t::block_size();
				offset += detail::directory_t::block_size(version()) * directory_count();

				for (const auto& dir : _dirs) {
					dir->file_offset(offset);
					if (directory_strings()) {
						offset += dir->name_size() + 1;
					}
					offset += detail::file_t::block_size() * dir->file_count();
				}
			}

			inline void update_files()
			{
				std::size_t offset{ 0 };
				offset += detail::header_t::block_size();
				offset += detail::directory_t::block_size(version()) * directory_count();
				if (directory_strings()) {
					offset += directory_names_length();
					offset += directory_count();  // bzstring prefix bytes
				}

				offset += detail::file_t::block_size() * file_count();
				if (file_strings()) {
					offset += file_names_length();
				}

				for (const auto& dir : _dirs) {
					for (const auto& file : *dir) {
						file->offset(offset);
						if (embedded_file_names()) {  // bstring
							offset += 1 + (dir->name_size() - 1) + 1 + (file->name_size() - 1);
						}

						if (file->compressed()) {
							offset += 4;
						}

						offset += file->size();
					}
				}
			}

			inline void update_header()
			{
				_header.directory_count(
					calc_directory_count());
				_header.directory_names_length(
					calc_directory_names_length());

				_header.file_count(
					calc_file_count());
				_header.file_names_length(
					calc_file_names_length());
			}

			container_t _dirs;
			detail::header_t _header;
		};

		inline archive& operator<<(archive& a_archive, const stl::filesystem::path& a_path)
		{
			a_archive.read(a_path);
			return a_archive;
		}

		inline archive& operator>>(archive& a_archive, const stl::filesystem::path& a_path)
		{
			a_archive.write(a_path);
			return a_archive;
		}

		inline archive& operator>>(archive& a_archive, std::ostream& a_stream)
		{
			a_archive.write(a_stream);
			return a_archive;
		}
	}
}
