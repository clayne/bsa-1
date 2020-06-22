#pragma once

#include "bsa/common.hpp"
#include "bsa/stl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>


namespace bsa
{
	namespace fo4  // Fallout 4
	{
		using archive_version = std::size_t;
		BSA_CXX17_INLINE constexpr archive_version v1{ 1 };


		namespace detail
		{
			using namespace bsa::detail;


			class file_hasher;
			class general_t;
			class hash_t;
			class header_t;
			class texture_t;


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

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x18; }

				BSA_NODISCARD BSA_CXX17_CONSTEXPR bool directx() const { return format() == DIRECTX; }
				BSA_NODISCARD BSA_CXX17_CONSTEXPR bool general() const { return format() == GENERAL; }

				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return narrow_cast<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr stl::string_view format() const { return stl::string_view(_block.contentsFormat.data(), _block.contentsFormat.size()); }
				BSA_NODISCARD constexpr bool has_string_table() const noexcept { return string_table_offset() != 0; }
				BSA_NODISCARD constexpr stl::string_view magic() const { return stl::string_view(_block.magic.data(), _block.magic.size()); }
				BSA_NODISCARD constexpr std::uint64_t string_table_offset() const noexcept { return _block.stringTableOffset; }
				BSA_NODISCARD constexpr archive_version version() const noexcept { return narrow_cast<archive_version>(_block.version); }

				constexpr void clear() noexcept { _block = block_t(); }

				inline void read(istream_t& a_input)
				{
					_block.read(a_input);
					if (magic() != MAGIC) {
						throw input_error();
					}
				}

			private:
				struct block_t	// BSResource::Archive2::Header
				{
					constexpr block_t() noexcept :
						magic{ '\0', '\0', '\0', '\0' },
						version(0),
						contentsFormat{ '\0', '\0', '\0', '\0' },
						fileCount(0),
						stringTableOffset(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						magic(a_rhs.magic),
						version(a_rhs.version),
						contentsFormat(a_rhs.contentsFormat),
						fileCount(a_rhs.fileCount),
						stringTableOffset(a_rhs.stringTableOffset)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						magic(std::move(a_rhs.magic)),
						version(std::move(a_rhs.version)),
						contentsFormat(std::move(a_rhs.contentsFormat)),
						fileCount(std::move(a_rhs.fileCount)),
						stringTableOffset(std::move(a_rhs.stringTableOffset))
					{}

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							magic = a_rhs.magic;
							version = a_rhs.version;
							contentsFormat = a_rhs.contentsFormat;
							fileCount = a_rhs.fileCount;
							stringTableOffset = a_rhs.stringTableOffset;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							magic = std::move(a_rhs.magic);
							version = std::move(a_rhs.version);
							contentsFormat = std::move(a_rhs.contentsFormat);
							fileCount = std::move(a_rhs.fileCount);
							stringTableOffset = std::move(a_rhs.stringTableOffset);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							magic >>
							version >>
							contentsFormat >>
							fileCount >>
							stringTableOffset;
					}

					std::array<char, 4> magic;
					std::uint32_t version;
					std::array<char, 4> contentsFormat;
					std::uint32_t fileCount;
					std::uint64_t stringTableOffset;
				};

				static constexpr stl::string_view DIRECTX{ "DX10", 4 };
				static constexpr stl::string_view GENERAL{ "GNRL", 4 };
				static constexpr stl::string_view MAGIC{ "BTDX", 4 };

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

				BSA_NODISCARD friend constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept
				{
					if (a_lhs.file_hash() != a_rhs.file_hash() ||
						a_lhs.directory_hash() != a_rhs.directory_hash()) {
						return false;
					} else {
						return std::char_traits<char>::compare(
								   a_lhs._block.ext.data(),
								   a_rhs._block.ext.data(),
								   a_lhs._block.ext.size()) == 0;
					}
				}

				BSA_NODISCARD friend constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0xC; }

				BSA_NODISCARD constexpr std::uint32_t directory_hash() const noexcept { return _block.dir; }
				BSA_NODISCARD constexpr stl::string_view extension() const { return stl::string_view(_block.ext.data(), _block.ext.size()); }
				BSA_NODISCARD constexpr std::uint32_t file_hash() const noexcept { return _block.file; }

				inline void read(istream_t& a_input) { _block.read(a_input); }

			protected:
				friend class file_hasher;

				struct block_t	// BSResource::ID
				{
					constexpr block_t() noexcept :
						file(0),
						ext{ '\0', '\0', '\0', '\0' },
						dir(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						file(a_rhs.file),
						ext(a_rhs.ext),
						dir(a_rhs.dir)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						file(std::move(a_rhs.file)),
						ext(std::move(a_rhs.ext)),
						dir(std::move(a_rhs.dir))
					{}

					~block_t() = default;

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							ext = a_rhs.ext;
							file = a_rhs.file;
							dir = a_rhs.dir;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							ext = std::move(a_rhs.ext);
							file = std::move(a_rhs.file);
							dir = std::move(a_rhs.dir);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							file >>
							ext >>
							dir;
					}

					std::uint32_t file;
					std::array<char, 4> ext;
					std::uint32_t dir;
				};

				BSA_NODISCARD constexpr block_t& block_ref() noexcept { return _block; }
				BSA_NODISCARD constexpr const block_t& block_ref() const noexcept { return _block; }

			private:
				block_t _block;
			};
			using hash_ref = std::reference_wrapper<hash_t>;


			class general_t
			{
			public:
				inline general_t() noexcept :
					_hash(),
					_header(),
					_chunks()
				{}

				inline general_t(const general_t& a_rhs) :
					_hash(a_rhs._hash),
					_header(a_rhs._header),
					_chunks(a_rhs._chunks)
				{}

				inline general_t(general_t&& a_rhs) noexcept :
					_hash(std::move(a_rhs._hash)),
					_header(std::move(a_rhs._header)),
					_chunks(std::move(a_rhs._chunks))
				{}

				~general_t() = default;

				inline general_t& operator=(const general_t& a_rhs)
				{
					if (this != std::addressof(a_rhs)) {
						_hash = a_rhs._hash;
						_header = a_rhs._header;
						_chunks = a_rhs._chunks;
					}
					return *this;
				}

				inline general_t& operator=(general_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_hash = std::move(a_rhs._hash);
						_header = std::move(a_rhs._header);
						_chunks = std::move(a_rhs._chunks);
					}
					return *this;
				}

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

				BSA_NODISCARD constexpr std::ptrdiff_t chunk_count() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.chunkCount); }
				BSA_NODISCARD constexpr std::size_t chunk_offset() const noexcept { return narrow_cast<std::size_t>(_header.chunkOffsetOrType); }

				BSA_NODISCARD constexpr std::ptrdiff_t data_file_index() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.dataFileIndex); }

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				inline void read(istream_t& a_input)
				{
					_hash.read(a_input);
					_header.read(a_input);
					if (chunk_count() > 0) {
						_chunks.resize(narrow_cast<std::size_t>(chunk_count()));
						for (auto& chunk : _chunks) {
							chunk.read(a_input);
						}
					}
				}

				inline void read_name(istream_t& a_input)
				{
					std::uint16_t length;
					a_input >> length;
					_name.resize(length);
					a_input.read(_name.begin(), _name.length());
				}

			private:
				struct header_t	 // BSResource::Archive2::Index::EntryHeader
				{
					constexpr header_t() noexcept :
						dataFileIndex(0),
						chunkCount(0),
						chunkOffsetOrType(0)
					{}

					constexpr header_t(const header_t& a_rhs) noexcept :
						dataFileIndex(a_rhs.dataFileIndex),
						chunkCount(a_rhs.chunkCount),
						chunkOffsetOrType(a_rhs.chunkOffsetOrType)
					{}

					constexpr header_t(header_t&& a_rhs) noexcept :
						dataFileIndex(std::move(a_rhs.dataFileIndex)),
						chunkCount(std::move(a_rhs.chunkCount)),
						chunkOffsetOrType(std::move(a_rhs.chunkOffsetOrType))
					{}

					~header_t() = default;

					constexpr header_t& operator=(const header_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileIndex = a_rhs.dataFileIndex;
							chunkCount = a_rhs.chunkCount;
							chunkOffsetOrType = a_rhs.chunkOffsetOrType;
						}
						return *this;
					}

					constexpr header_t& operator=(header_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileIndex = std::move(a_rhs.dataFileIndex);
							chunkCount = std::move(a_rhs.chunkCount);
							chunkOffsetOrType = std::move(a_rhs.chunkOffsetOrType);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							dataFileIndex >>
							chunkCount >>
							chunkOffsetOrType;
					}

					std::int8_t dataFileIndex;
					std::int8_t chunkCount;
					std::uint16_t chunkOffsetOrType;
				};

				struct chunk_t	// BSResource::Archive2::Index::Chunk
				{
					constexpr chunk_t() noexcept :
						dataFileOffset(0),
						compressedSize(0),
						uncompressedSize(0)
					{}

					constexpr chunk_t(const chunk_t& a_rhs) noexcept :
						dataFileOffset(a_rhs.dataFileOffset),
						compressedSize(a_rhs.compressedSize),
						uncompressedSize(a_rhs.uncompressedSize)
					{}

					constexpr chunk_t(chunk_t&& a_rhs) noexcept :
						dataFileOffset(std::move(a_rhs.dataFileOffset)),
						compressedSize(std::move(a_rhs.compressedSize)),
						uncompressedSize(std::move(a_rhs.uncompressedSize))
					{}

					~chunk_t() = default;

					constexpr chunk_t& operator=(const chunk_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileOffset = a_rhs.dataFileOffset;
							compressedSize = a_rhs.compressedSize;
							uncompressedSize = a_rhs.uncompressedSize;
						}
						return *this;
					}

					constexpr chunk_t& operator=(chunk_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileOffset = std::move(a_rhs.dataFileOffset);
							compressedSize = std::move(a_rhs.compressedSize);
							uncompressedSize = std::move(a_rhs.uncompressedSize);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							dataFileOffset >>
							compressedSize >>
							uncompressedSize;

						std::uint32_t sentinel;
						a_input >> sentinel;
						if (sentinel != BAADFOOD) {
							throw input_error();
						}
					}

					static constexpr auto BAADFOOD{ narrow_cast<std::uint32_t>(0xBAADF00D) };

					std::uint64_t dataFileOffset;
					std::uint32_t compressedSize;
					std::uint32_t uncompressedSize;
				};

				hash_t _hash;
				header_t _header;
				std::vector<chunk_t> _chunks;
				std::string _name;
			};
			using general_ptr = std::shared_ptr<general_t>;


			class texture_t
			{
			public:
				inline texture_t() noexcept :
					_hash(),
					_header(),
					_chunks()
				{}

				inline texture_t(const texture_t& a_rhs) :
					_hash(a_rhs._hash),
					_header(a_rhs._header),
					_chunks(a_rhs._chunks)
				{}

				inline texture_t(texture_t&& a_rhs) noexcept :
					_hash(std::move(a_rhs._hash)),
					_header(std::move(a_rhs._header)),
					_chunks(std::move(a_rhs._chunks))
				{}

				~texture_t() = default;

				inline texture_t& operator=(const texture_t& a_rhs)
				{
					if (this != std::addressof(a_rhs)) {
						_hash = a_rhs._hash;
						_header = a_rhs._header;
						_chunks = a_rhs._chunks;
					}
					return *this;
				}

				inline texture_t& operator=(texture_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_hash = std::move(a_rhs._hash);
						_header = std::move(a_rhs._header);
						_chunks = std::move(a_rhs._chunks);
					}
					return *this;
				}

				BSA_NODISCARD constexpr std::ptrdiff_t chunk_count() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.chunkCount); }
				BSA_NODISCARD constexpr std::size_t chunk_offset() const noexcept { return narrow_cast<std::size_t>(_header.chunkOffset); }

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

				BSA_NODISCARD constexpr std::ptrdiff_t data_file_index() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.dataFileIndex); }

				BSA_NODISCARD constexpr std::ptrdiff_t flags() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.flags); }

				BSA_NODISCARD constexpr std::ptrdiff_t format() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.format); }

				BSA_NODISCARD constexpr std::size_t height() const noexcept { return narrow_cast<std::size_t>(_header.height); }

				BSA_NODISCARD constexpr hash_t hash() const noexcept { return _hash; }
				BSA_NODISCARD constexpr hash_t& hash_ref() noexcept { return _hash; }
				BSA_NODISCARD constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				BSA_NODISCARD constexpr std::ptrdiff_t mip_count() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.mipCount); }

				BSA_NODISCARD inline std::string str() const { return _name; }
				BSA_NODISCARD constexpr std::string& str_ref() noexcept { return _name; }
				BSA_NODISCARD constexpr const std::string& str_ref() const noexcept { return _name; }

				BSA_NODISCARD constexpr std::ptrdiff_t tile_mode() const noexcept { return narrow_cast<std::ptrdiff_t>(_header.tilemode); }

				BSA_NODISCARD constexpr std::size_t width() const noexcept { return narrow_cast<std::size_t>(_header.width); }

				inline void read(istream_t& a_input)
				{
					_hash.read(a_input);
					_header.read(a_input);
					if (chunk_count() > 0) {
						_chunks.resize(narrow_cast<std::size_t>(chunk_count()));
						for (auto& chunk : _chunks) {
							chunk.read(a_input);
						}
					}
				}

				inline void read_name(istream_t& a_input)
				{
					std::uint16_t length;
					a_input >> length;
					_name.resize(length);
					a_input.read(_name.begin(), _name.length());
				}

			private:
				struct header_t	 // BSTextureStreamer::NativeDesc<BSGraphics::TextureHeader>
				{
					constexpr header_t() noexcept :
						dataFileIndex(0),
						chunkCount(0),
						chunkOffset(0),
						height(0),
						width(0),
						mipCount(0),
						format(0),
						flags(0),
						tilemode(0)
					{}

					constexpr header_t(const header_t& a_rhs) noexcept :
						dataFileIndex(a_rhs.dataFileIndex),
						chunkCount(a_rhs.chunkCount),
						chunkOffset(a_rhs.chunkOffset),
						height(a_rhs.height),
						width(a_rhs.width),
						mipCount(a_rhs.mipCount),
						format(a_rhs.format),
						flags(a_rhs.flags),
						tilemode(a_rhs.tilemode)
					{}

					constexpr header_t(header_t&& a_rhs) noexcept :
						dataFileIndex(std::move(a_rhs.dataFileIndex)),
						chunkCount(std::move(a_rhs.chunkCount)),
						chunkOffset(std::move(a_rhs.chunkOffset)),
						height(std::move(a_rhs.height)),
						width(std::move(a_rhs.width)),
						mipCount(std::move(a_rhs.mipCount)),
						format(std::move(a_rhs.format)),
						flags(std::move(a_rhs.flags)),
						tilemode(std::move(a_rhs.tilemode))
					{}

					~header_t() = default;

					constexpr header_t& operator=(const header_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileIndex = a_rhs.dataFileIndex;
							chunkCount = a_rhs.chunkCount;
							chunkOffset = a_rhs.chunkOffset;
							height = a_rhs.height;
							width = a_rhs.width;
							mipCount = a_rhs.mipCount;
							format = a_rhs.format;
							flags = a_rhs.flags;
							tilemode = a_rhs.tilemode;
						}
						return *this;
					}

					constexpr header_t& operator=(header_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileIndex = std::move(a_rhs.dataFileIndex);
							chunkCount = std::move(a_rhs.chunkCount);
							chunkOffset = std::move(a_rhs.chunkOffset);
							height = std::move(a_rhs.height);
							width = std::move(a_rhs.width);
							mipCount = std::move(a_rhs.mipCount);
							format = std::move(a_rhs.format);
							flags = std::move(a_rhs.flags);
							tilemode = std::move(a_rhs.tilemode);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							dataFileIndex >>
							chunkCount >>
							chunkOffset >>
							height >>
							width >>
							mipCount >>
							format >>
							flags >>
							tilemode;
					}

					std::int8_t dataFileIndex;
					std::int8_t chunkCount;
					std::uint16_t chunkOffset;
					std::uint16_t height;
					std::uint16_t width;
					std::int8_t mipCount;
					std::int8_t format;
					std::int8_t flags;
					std::int8_t tilemode;
				};

				struct chunk_t	// BSTextureStreamer::ChunkDesc
				{
					constexpr chunk_t() noexcept :
						dataFileOffset(0),
						size(0),
						uncompressedSize(0),
						mipFirst(0),
						mipLast(0),
						sentinel(BAADFOOD)
					{}

					constexpr chunk_t(const chunk_t& a_rhs) noexcept :
						dataFileOffset(a_rhs.dataFileOffset),
						size(a_rhs.size),
						uncompressedSize(a_rhs.uncompressedSize),
						mipFirst(a_rhs.mipFirst),
						mipLast(a_rhs.mipLast),
						sentinel(BAADFOOD)
					{}

					constexpr chunk_t(chunk_t&& a_rhs) noexcept :
						dataFileOffset(std::move(a_rhs.dataFileOffset)),
						size(std::move(a_rhs.size)),
						uncompressedSize(std::move(a_rhs.uncompressedSize)),
						mipFirst(std::move(a_rhs.mipFirst)),
						mipLast(std::move(a_rhs.mipLast)),
						sentinel(BAADFOOD)
					{}

					~chunk_t() = default;

					constexpr chunk_t& operator=(const chunk_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileOffset = a_rhs.dataFileOffset;
							size = a_rhs.size;
							uncompressedSize = a_rhs.uncompressedSize;
							mipFirst = a_rhs.mipFirst;
							mipLast = a_rhs.mipLast;
						}
						return *this;
					}

					constexpr chunk_t& operator=(chunk_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							dataFileOffset = std::move(a_rhs.dataFileOffset);
							size = std::move(a_rhs.size);
							uncompressedSize = std::move(a_rhs.uncompressedSize);
							mipFirst = std::move(a_rhs.mipFirst);
							mipLast = std::move(a_rhs.mipLast);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input >>
							dataFileOffset >>
							size >>
							uncompressedSize >>
							mipFirst >>
							mipLast >>
							sentinel;

						if (sentinel != BAADFOOD) {
							throw input_error();
						}
					}

					static constexpr auto BAADFOOD{ narrow_cast<std::uint32_t>(0xBAADF00D) };

					std::uint64_t dataFileOffset;
					std::uint32_t size;
					std::uint32_t uncompressedSize;
					std::uint16_t mipFirst;
					std::uint16_t mipLast;
					std::uint32_t sentinel;
				};

				hash_t _hash;
				header_t _header;
				std::vector<chunk_t> _chunks;
				std::string _name;
			};
			using texture_ptr = std::shared_ptr<texture_t>;


			class file_hasher
			{
			public:
				file_hasher() = default;
				file_hasher(const file_hasher&) = default;
				file_hasher(file_hasher&&) = default;

				file_hasher& operator=(const file_hasher&) = default;
				file_hasher& operator=(file_hasher&&) = default;

				BSA_NODISCARD inline hash_t operator()(stl::string_view a_path) const
				{
					for (auto& ch : a_path) {
						if (ch < 0) {
							throw hash_non_ascii();
						}
					}

					std::string file;
					std::string extension;
					std::string directory;
					std::tie(file, extension, directory) = normalize(std::move(a_path));

					hash_t hash;
					auto& block = hash.block_ref();

					block.file = hash_string(file);
					block.dir = hash_string(directory);
					for (std::size_t i = 0; i < (std::min)(extension.size(), block.ext.size()); ++i) {
						block.ext[i] = extension[i];
					}

					return hash;
				}

			private:
				static constexpr std::array<std::uint32_t, 256> CRCTABLE = {
					{ 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
						0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
						0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
						0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
						0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
						0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
						0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
						0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
						0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
						0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
						0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
						0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
						0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
						0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
						0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
						0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
						0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
						0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
						0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
						0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
						0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
						0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
						0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
						0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
						0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
						0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
						0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
						0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
						0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
						0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
						0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
						0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D }
				};

				BSA_NODISCARD constexpr std::uint32_t hash_string(stl::string_view a_string) const
				{
					std::uint32_t hash = 0;
					for (auto& ch : a_string) {
						hash = (hash >> 8) ^ CRCTABLE[(hash ^ narrow_cast<std::uint8_t>(ch)) & 0xFF];
					}
					return hash;
				}

				BSA_NODISCARD inline std::tuple<std::string, std::string, std::string> normalize(stl::string_view a_path) const
				{
					stl::filesystem::path path(a_path.begin(), a_path.end());
					path = path.lexically_normal();

					std::string file;
					if (path.has_stem()) {
						file = path.stem().string();
						tidy_string(file);
					}

					std::string extension;
					if (path.has_extension()) {
						extension = path.extension().string();
						for (auto& ch : extension) {
							ch = mapchar(ch);
						}
						if (!extension.empty() && extension.front() == '.') {
							extension = extension.substr(1);
						}
					}

					std::string directory;
					if (path.has_parent_path()) {
						directory = path.parent_path().string();
						tidy_string(directory);
					}

					return std::make_tuple(std::move(file), std::move(extension), std::move(directory));
				}

				inline void tidy_string(std::string& a_str) const
				{
					for (auto& ch : a_str) {
						ch = mapchar(ch);
					}
					while (!a_str.empty() && a_str.back() == '\\') {
						a_str.pop_back();
					}
					while (!a_str.empty() && a_str.front() == '\\') {
						a_str = a_str.substr(1);
					}
				}
			};
		}


		class directory;
		class file_entry;
		class file_iterator;
		class general_file;
		class hash;
		class texture_file;


		class hash
		{
		public:
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

			BSA_NODISCARD inline std::uint32_t directory_hash() const noexcept { return _impl.get().directory_hash(); }
			BSA_NODISCARD inline stl::string_view extension() const { return _impl.get().extension(); }
			BSA_NODISCARD inline std::uint32_t file_hash() const noexcept { return _impl.get().file_hash(); }

		protected:
			friend class general_file;
			friend class texture_file;

			inline hash(detail::hash_t& a_rhs) noexcept :
				_impl(std::ref(a_rhs))
			{}

			inline hash(const detail::hash_ref& a_rhs) noexcept :
				_impl(a_rhs)
			{}

		private:
			detail::hash_ref _impl;
		};


		class general_file
		{
		public:
			general_file() = delete;

			inline general_file(const general_file& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline general_file(general_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			~general_file() = default;

			inline general_file& operator=(const general_file& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline general_file& operator=(general_file&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD inline std::ptrdiff_t chunk_count() const noexcept { return _impl->chunk_count(); }
			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline hash hash_value() const noexcept { return hash{ _impl->hash_ref() }; }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }

		protected:
			friend class file_iterator;

			using value_type = detail::general_ptr;

			explicit inline general_file(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};


		class texture_file
		{
		public:
			texture_file() = delete;

			inline texture_file(const texture_file& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline texture_file(texture_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			~texture_file() = default;

			inline texture_file& operator=(const texture_file& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline texture_file& operator=(texture_file&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD inline std::ptrdiff_t chunk_count() const noexcept { return _impl->chunk_count(); }
			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl->c_str(); }
			BSA_NODISCARD inline std::ptrdiff_t flags() const noexcept { return _impl->flags(); }
			BSA_NODISCARD inline std::ptrdiff_t format() const noexcept { return _impl->format(); }
			BSA_NODISCARD inline hash hash_value() const noexcept { return hash{ _impl->hash_ref() }; }
			BSA_NODISCARD inline std::size_t height() const noexcept { return _impl->height(); }
			BSA_NODISCARD inline std::ptrdiff_t mip_count() const noexcept { return _impl->mip_count(); }
			BSA_NODISCARD inline const std::string& string() const noexcept { return _impl->str_ref(); }
			BSA_NODISCARD inline std::ptrdiff_t tile_mode() const noexcept { return _impl->tile_mode(); }
			BSA_NODISCARD inline std::size_t width() const noexcept { return _impl->width(); }

		protected:
			friend class file_iterator;

			using value_type = detail::texture_ptr;

			explicit inline texture_file(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};


		class file_entry
		{
		private:
			using general_file_t = general_file;
			using texture_file_t = texture_file;

		public:
			constexpr file_entry() noexcept :
				_impl()
			{}

			inline file_entry(const file_entry& a_rhs) :
				_impl(a_rhs._impl)
			{}

			inline file_entry(file_entry&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			explicit inline file_entry(const general_file& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline file_entry(general_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

			explicit inline file_entry(const texture_file& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline file_entry(texture_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

			~file_entry() = default;

			inline file_entry& operator=(const file_entry& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline file_entry& operator=(file_entry&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			BSA_NODISCARD constexpr bool is_general_file() const noexcept { return _impl.index() == igeneral; }
			BSA_NODISCARD constexpr bool is_texture_file() const noexcept { return _impl.index() == itexture; }

			BSA_NODISCARD constexpr const general_file& general_file() const { return stl::get<igeneral>(_impl); }
			BSA_NODISCARD constexpr const texture_file& texture_file() const { return stl::get<itexture>(_impl); }

			BSA_NODISCARD const char* c_str() const noexcept
			{
				switch (_impl.index()) {
				case igeneral:
					return stl::get<igeneral>(_impl).c_str();
				case itexture:
					return stl::get<itexture>(_impl).c_str();
				default:
					return "";
				}
			}

			BSA_NODISCARD const std::string& string() const noexcept
			{
				static std::string defaultStr;

				switch (_impl.index()) {
				case igeneral:
					return stl::get<igeneral>(_impl).string();
				case itexture:
					return stl::get<itexture>(_impl).string();
				default:
					return defaultStr;
				}
			}

		private:
			enum : std::size_t
			{
				imono,
				igeneral,
				itexture
			};

			stl::variant<stl::monostate, general_file_t, texture_file_t> _impl;
		};


		class file_iterator
		{
		public:
			using value_type = file_entry;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::input_iterator_tag;

			BSA_CXX17_CONSTEXPR file_iterator() noexcept :
				_files(stl::nullopt),
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

			~file_iterator() = default;

			inline file_iterator& operator=(const file_iterator& a_rhs)
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

			BSA_NODISCARD friend BSA_CXX17_CONSTEXPR bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !a_lhs._files && !a_rhs._files; }
			BSA_NODISCARD friend BSA_CXX17_CONSTEXPR bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			BSA_NODISCARD inline reference operator*() { return fetch(); }
			BSA_NODISCARD inline pointer operator->() { return std::addressof(fetch()); }

			// prefix
			inline file_iterator& operator++()
			{
				++_pos;
				if (_pos >= _files->size()) {
					_files.reset();
					_pos = NPOS;
				}
				return *this;
			}

			// postfix
			BSA_NODISCARD inline file_iterator operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			friend inline void swap(file_iterator& a_lhs, file_iterator& a_rhs) noexcept
			{
				auto tmp = std::move(a_lhs);
				a_lhs = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class archive;

			explicit inline file_iterator(const std::vector<detail::general_ptr>& a_general) :
				_files(stl::in_place),
				_pos(0)
			{
				if (!a_general.empty()) {
					for (auto& general : a_general) {
						_files->emplace_back(general_file(general));
					}
				} else {
					_files.reset();
					_pos = NPOS;
				}
			}

			explicit inline file_iterator(const std::vector<detail::texture_ptr>& a_textures) :
				_files(stl::in_place),
				_pos(0)
			{
				if (!a_textures.empty()) {
					for (auto& texture : a_textures) {
						_files->emplace_back(texture_file(texture));
					}
				} else {
					_files.reset();
					_pos = NPOS;
				}
			}

		private:
			inline reference fetch() { return _files.value()[_pos]; }

			static constexpr auto NPOS{ (std::numeric_limits<std::size_t>::max)() };

			stl::optional<std::vector<value_type>> _files;
			std::size_t _pos;
		};


		class archive
		{
		public:
			using iterator = file_iterator;
			using const_iterator = iterator;

			inline archive() noexcept :
				_files(),
				_header()
			{}

			inline archive(const archive& a_archive) :
				_files(a_archive._files),
				_header(a_archive._header)
			{}

			inline archive(archive&& a_archive) noexcept :
				_files(std::move(a_archive._files)),
				_header(std::move(a_archive._header))
			{}

			inline archive(const stl::filesystem::path& a_path) :
				_files(),
				_header()
			{
				read(a_path);
			}

			~archive() = default;

			inline archive& operator=(const archive& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_header = a_rhs._header;
				}
				return *this;
			}

			inline archive& operator=(archive&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_header = std::move(a_rhs._header);
				}
				return *this;
			}

			BSA_NODISCARD inline iterator begin() const
			{
				switch (_files.index()) {
				case igeneral:
					return iterator(stl::get<igeneral>(_files));
				case itexture:
					return iterator(stl::get<itexture>(_files));
				default:
					return end();
				}
			}

			BSA_NODISCARD inline iterator end() const noexcept { return iterator{}; }

			BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			BSA_NODISCARD constexpr stl::string_view format() const { return _header.format(); }
			BSA_NODISCARD constexpr stl::string_view tag() const { return _header.magic(); }
			BSA_NODISCARD constexpr archive_version version() const noexcept { return _header.version(); }

			BSA_NODISCARD constexpr bool file_strings() const noexcept { return _header.has_string_table(); }

			BSA_NODISCARD BSA_CXX17_CONSTEXPR bool directx() const { return _header.directx(); }
			BSA_NODISCARD BSA_CXX17_CONSTEXPR bool general() const { return _header.general(); }

			inline void clear() noexcept
			{
				try {
					if (_files.valueless_by_exception()) {
						_files.emplace<0>();
					} else {
						stl::visit([](auto&& a_arg) noexcept { a_arg.clear(); }, _files);
					}
				} catch (...) {}

				_header.clear();
			}

			inline void read(const stl::filesystem::path& a_path)
			{
				detail::istream_t input(a_path);

				clear();

				_header.read(input);
				switch (_header.version()) {
				case v1:
					break;
				default:
					throw version_error();
				}

				if (_header.general()) {
					_files.emplace<cgeneral>(_header.file_count());
					for (auto& file : stl::get<cgeneral>(_files)) {
						file = std::make_shared<detail::general_t>();
						file->read(input);
					}
				} else if (_header.directx()) {
					_files.emplace<ctexture>(_header.file_count());
					for (auto& file : stl::get<ctexture>(_files)) {
						file = std::make_shared<detail::texture_t>();
						file->read(input);
					}
				} else {
					throw input_error();
				}

				if (_header.has_string_table()) {
					input.seek_beg(_header.string_table_offset());

					switch (_files.index()) {
					case igeneral:
						for (auto& file : stl::get<igeneral>(_files)) {
							file->read_name(input);
						}
						break;
					case itexture:
						for (auto& file : stl::get<itexture>(_files)) {
							file->read_name(input);
						}
						break;
					default:
						throw stl::bad_variant_access();
					}
				}

				assert(sanity_check());
			}

		private:
			using cgeneral = std::vector<detail::general_ptr>;
			using ctexture = std::vector<detail::texture_ptr>;

			enum : std::size_t
			{
				igeneral,
				itexture
			};

			inline bool sanity_check()
			{
				switch (_files.index()) {
				case igeneral:
				case itexture:
					stl::visit([](auto&& a_files) {
						for (const auto& file : a_files) {
							try {
								const auto hash = detail::file_hasher()(file->str_ref());
								if (hash != file->hash_ref()) {
									assert(false);
								}
							} catch (const hash_error&) {
								continue;
							}
						}
					},
						_files);
					break;
				default:
					assert(false);
					break;
				}

				return true;
			}

			stl::variant<cgeneral, ctexture> _files;
			detail::header_t _header;
		};
	}
}
