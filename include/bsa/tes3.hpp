#pragma once

#include "bsa/common.hpp"
#include "bsa/stl.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace bsa
{
	namespace tes3	// The Elder Scrolls III: Morrowind
	{
		using archive_version = std::size_t;
		BSA_CXX17_INLINE constexpr archive_version v256{ 256 };

		class archive;
		class file;
		class file_iterator;
		class hash;

		namespace detail
		{
			using namespace bsa::detail;

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

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0xC; }

				BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return zero_extend<std::size_t>(_block.fileCount); }
				BSA_NODISCARD constexpr std::size_t hash_offset() const noexcept { return zero_extend<std::size_t>(_block.hashOffset); }
				BSA_NODISCARD constexpr archive_version version() const noexcept { return zero_extend<archive_version>(_block.version); }

				constexpr void file_count(std::size_t a_count)
				{
					if (a_count > max_int32) {
						throw size_error();
					} else {
						_block.fileCount = zero_extend<std::uint32_t>(a_count);
					}
				}

				constexpr void hash_offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.hashOffset = zero_extend<std::uint32_t>(a_offset);
					}
				}

				constexpr void clear() noexcept { _block = block_t(); }

				inline void read(istream_t& a_input)
				{
					_block.read(a_input);
				}

				inline void write(ostream_t& a_output) const
				{
					_block.write(a_output);
				}

			private:
				struct block_t final
				{
					constexpr block_t() noexcept :
						version(v256),
						hashOffset(0),
						fileCount(0)
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

					inline void read(istream_t& a_input)
					{
						a_input >>
							version >>
							hashOffset >>
							fileCount;
					}

					inline void write(ostream_t& a_output) const
					{
						a_output
							<< version
							<< hashOffset
							<< fileCount;
					}

					std::uint32_t version;
					std::uint32_t hashOffset;
					std::uint32_t fileCount;
				};

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

				BSA_NODISCARD constexpr std::uint32_t high() const noexcept { return _block.hi; }
				BSA_NODISCARD constexpr std::uint32_t low() const noexcept { return _block.lo; }

				BSA_NODISCARD constexpr std::uint64_t numeric() const noexcept
				{
					return zero_extend<std::uint64_t>(_block.lo) << 0 * byte_v |
						   zero_extend<std::uint64_t>(_block.hi) << 4 * byte_v;
				}

				inline void read(istream_t& a_input) { _block.read(a_input); }
				inline void write(ostream_t& a_output) const { _block.write(a_output); }

			protected:
				friend class file_hasher;

				struct block_t final
				{
					constexpr block_t() noexcept :
						lo(0),
						hi(0)
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

					inline void read(istream_t& a_input)
					{
						a_input >>
							lo >>
							hi;
					}

					inline void write(ostream_t& a_output) const
					{
						a_output
							<< lo
							<< hi;
					}

					std::uint32_t lo;
					std::uint32_t hi;
				};

				BSA_NODISCARD constexpr block_t& block_ref() noexcept { return _block; }
				BSA_NODISCARD constexpr const block_t& block_ref() const noexcept { return _block; }

			private:
				block_t _block;
			};

			BSA_NODISCARD constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() == a_rhs.numeric(); }
			BSA_NODISCARD constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			BSA_NODISCARD constexpr bool operator<(const hash_t& a_lhs, const hash_t& a_rhs) noexcept
			{
				if (a_lhs.low() != a_rhs.low()) {
					return a_lhs.low() < a_rhs.low();
				} else {
					return a_lhs.high() < a_rhs.high();
				}
			}

			BSA_NODISCARD constexpr bool operator>(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_rhs < a_lhs; }
			BSA_NODISCARD constexpr bool operator<=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
			BSA_NODISCARD constexpr bool operator>=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

			class file_hasher final
			{
			public:
				constexpr file_hasher() noexcept = default;
				constexpr file_hasher(const file_hasher&) noexcept = default;
				constexpr file_hasher(file_hasher&&) noexcept = default;

				~file_hasher() noexcept = default;

				constexpr file_hasher& operator=(const file_hasher&) noexcept = default;
				constexpr file_hasher& operator=(file_hasher&&) noexcept = default;

				BSA_NODISCARD inline hash_t operator()(const path_t& a_path) const
				{
					const auto view = a_path.string_view();
					verify(view);
					return hash(view);
				}

			private:
				BSA_NODISCARD constexpr hash_t hash(stl::string_view a_fullPath) const
				{
					hash_t hash;
					auto& block = hash.block_ref();

					const std::size_t midPoint = a_fullPath.size() >> 1;
					std::size_t i = 0;
					while (i < midPoint) {
						// rotate between first 4 bytes
						block.lo ^= zero_extend<std::uint32_t>(a_fullPath[i]) << ((i % 4) * 8);
						++i;
					}

					std::uint32_t rot = 0;
					while (i < a_fullPath.length()) {
						// rotate between first 4 bytes
						rot = zero_extend<std::uint32_t>(a_fullPath[i]) << (((i - midPoint) % 4) * 8);
						block.hi = stl::rotr<std::uint32_t>(block.hi ^ rot, zero_extend<int>(rot));
						++i;
					}

					return hash;
				}

				inline void verify(stl::string_view a_path) const
				{
					if (a_path.empty()) {
						throw empty_file();
					}

					for (auto& ch : a_path) {
						if (ch < 0) {
							throw hash_non_ascii();
						}
					}
				}
			};

			class file_t final
			{
			public:
				file_t() noexcept = default;
				file_t(const file_t&) = default;
				file_t(file_t&&) noexcept = default;

				inline file_t(const boost::filesystem::path& a_relativePath) :
					_hash(),
					_block(),
					_name(),
					_data()
				{
					path_t path(a_relativePath);
					_hash = file_hasher()(path);
					_name = path.string();
				}

				~file_t() = default;

				file_t& operator=(const file_t&) = default;
				file_t& operator=(file_t&&) noexcept = default;

				BSA_NODISCARD static constexpr std::size_t block_size() noexcept { return 0x8; }

				BSA_NODISCARD inline const char* c_str() const noexcept { return _name.c_str(); }

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

				BSA_NODISCARD inline std::size_t name_size() const noexcept { return _name.size() + 1; }

				BSA_NODISCARD constexpr std::size_t offset() const noexcept { return zero_extend<std::size_t>(_block.offset); }

				BSA_NODISCARD constexpr std::size_t size() const noexcept { return zero_extend<std::size_t>(_block.size); }

				BSA_NODISCARD constexpr const std::string& string() const noexcept { return _name; }

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

				inline void set_data(stl::span<const stl::byte> a_data)
				{
					if (a_data.size() > max_int32) {
						throw size_error();
					} else {
						_data.emplace<iview>(std::move(a_data));
						_block.size = zero_extend<std::uint32_t>(a_data.size());
					}
				}

				inline void set_data(istream_t a_input)
				{
					if (a_input.size() > max_int32) {
						throw size_error();
					} else {
						_data.emplace<ifile>(std::move(a_input));
						_block.size = zero_extend<std::uint32_t>(a_input.size());
					}
				}

				constexpr void set_offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.offset = zero_extend<std::uint32_t>(a_offset);
					}
				}

				inline void read(istream_t& a_input) { _block.read(a_input); }

				inline void read_hash(istream_t& a_input) { _hash.read(a_input); }

				inline void read_name(istream_t& a_input)
				{
					char ch;
					do {
						a_input.get(ch);
						_name.push_back(ch);
					} while (ch != '\0');
					_name.pop_back();  // discard null terminator
				}

				inline void read_data(istream_t& a_input)
				{
					const restore_point p(a_input);

					a_input.seek_rel(offset());
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

				inline void write(ostream_t& a_output) const { _block.write(a_output); }

				inline void write_hash(ostream_t& a_output) const { _hash.write(a_output); }

				inline void write_name(ostream_t& a_output) const
				{
					a_output << stl::string_view{ _name.data(), name_size() };
				}

				inline void write_data(ostream_t& a_output) const
				{
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

				struct block_t final
				{
					constexpr block_t() noexcept :
						size(0),
						offset(0)
					{}

					constexpr block_t(const block_t&) noexcept = default;
					constexpr block_t(block_t&&) noexcept = default;

					~block_t() noexcept = default;

					constexpr block_t& operator=(const block_t&) noexcept = default;
					constexpr block_t& operator=(block_t&&) noexcept = default;

					inline void read(istream_t& a_input)
					{
						a_input >>
							size >>
							offset;
					}

					inline void write(ostream_t& a_output) const
					{
						a_output
							<< size
							<< offset;
					}

					std::uint32_t size;
					std::uint32_t offset;
				};

				hash_t _hash;
				block_t _block;
				std::string _name;
				stl::variant<null_type, view_type, file_type, archive_type> _data;
			};
			using file_ptr = std::shared_ptr<file_t>;

			BSA_NODISCARD constexpr bool operator==(const file_t& a_lhs, const file_t& a_rhs) { return a_lhs.hash_ref() == a_rhs.hash_ref(); }
			BSA_NODISCARD constexpr bool operator!=(const file_t& a_lhs, const file_t& a_rhs) { return !(a_lhs == a_rhs); }

			BSA_NODISCARD constexpr bool operator<(const file_t& a_lhs, const file_t& a_rhs) noexcept { return a_lhs.hash_ref() < a_rhs.hash_ref(); }
			BSA_NODISCARD constexpr bool operator>(const file_t& a_lhs, const file_t& a_rhs) noexcept { return a_rhs < a_lhs; }
			BSA_NODISCARD constexpr bool operator<=(const file_t& a_lhs, const file_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
			BSA_NODISCARD constexpr bool operator>=(const file_t& a_lhs, const file_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }
		}

		class hash final
		{
		public:
			constexpr hash() noexcept = default;
			constexpr hash(const hash&) noexcept = default;
			constexpr hash(hash&&) noexcept = default;

			explicit inline hash(const boost::filesystem::path& a_path) :
				_impl(detail::file_hasher()(a_path))
			{}

			~hash() noexcept = default;

			constexpr hash& operator=(const hash&) noexcept = default;
			constexpr hash& operator=(hash&&) noexcept = default;

			inline hash& operator=(const boost::filesystem::path& a_path)
			{
				_impl = detail::file_hasher()(a_path);
				return *this;
			}

			BSA_NODISCARD constexpr std::uint32_t high() const noexcept { return _impl.high(); }
			BSA_NODISCARD constexpr std::uint32_t low() const noexcept { return _impl.low(); }
			BSA_NODISCARD constexpr std::uint64_t numeric() const noexcept { return _impl.numeric(); }

			constexpr void swap(hash& a_rhs) noexcept
			{
				hash tmp{ std::move(*this) };
				*this = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class file;

			using value_type = detail::hash_t;

			explicit constexpr hash(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit constexpr hash(value_type&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

		private:
			value_type _impl;
		};

		constexpr void swap(hash& a_lhs, hash& a_rhs) noexcept { a_lhs.swap(a_rhs); }

		inline std::ostream& operator<<(std::ostream& a_ostream, const hash& a_hash)
		{
			a_ostream << a_hash.numeric();
			return a_ostream;
		}

		class file final
		{
		public:
			file() noexcept = default;
			file(const file&) noexcept = default;
			file(file&&) noexcept = default;

			inline file(const boost::filesystem::path& a_relativePath, const boost::filesystem::path& a_filePath) :
				_impl(std::make_shared<detail::file_t>(a_relativePath))
			{
				open_and_pack(a_filePath);
			}

			inline file(const boost::filesystem::path& a_relativePath, stl::span<const stl::byte> a_data) :
				_impl(std::make_shared<detail::file_t>(a_relativePath))
			{
				_impl->set_data(std::move(a_data));
			}

			~file() noexcept = default;

			file& operator=(const file&) noexcept = default;
			file& operator=(file&&) noexcept = default;

			friend bool operator==(const file& a_lhs, const file& a_rhs) noexcept;
			friend bool operator<(const file& a_lhs, const file& a_rhs) noexcept;

			BSA_NODISCARD inline explicit operator bool() const noexcept { return exists(); }
			BSA_NODISCARD inline bool exists() const noexcept { return static_cast<bool>(_impl); }

			BSA_NODISCARD inline const char* c_str() const noexcept
			{
				assert(exists());
				return _impl->c_str();
			}

			BSA_NODISCARD inline bool empty() const noexcept
			{
				assert(exists());
				return _impl->empty();
			}

			BSA_NODISCARD inline stl::span<const stl::byte> extract() const
			{
				assert(exists());
				return _impl->get_data();
			}

			inline void extract_to(const boost::filesystem::path& a_root) const
			{
				assert(exists());
				if (!boost::filesystem::exists(a_root)) {
					throw output_error();
				}

				auto path = a_root;
				path /= string();
				boost::filesystem::create_directories(path.parent_path());
				std::ofstream file(path.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
				if (!file.is_open()) {
					throw output_error();
				}

				_impl->extract(file);
			}

			BSA_NODISCARD inline tes3::hash hash() const noexcept
			{
				assert(exists());
				return tes3::hash{ _impl->hash_ref() };
			}

			inline void pack(stl::span<const stl::byte> a_data)
			{
				assert(exists());
				_impl->set_data(std::move(a_data));
			}

			inline void pack(const boost::filesystem::path& a_path)
			{
				assert(exists());
				open_and_pack(a_path);
			}

			BSA_NODISCARD inline std::size_t size() const noexcept
			{
				assert(exists());
				return _impl->size();
			}

			BSA_NODISCARD inline const std::string& string() const noexcept
			{
				assert(exists());
				return _impl->string();
			}

			inline void swap(file& a_rhs) noexcept { std::swap(*this, a_rhs); }

		protected:
			friend class archive;
			friend class file_iterator;

			using value_type = detail::file_ptr;

			explicit inline file(value_type a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

			BSA_NODISCARD constexpr value_type& file_ptr() noexcept { return _impl; }
			BSA_NODISCARD constexpr const value_type& file_ptr() const noexcept { return _impl; }

		private:
			inline void open_and_pack(const boost::filesystem::path& a_path)
			{
				detail::istream_t input{ a_path };
				_impl->set_data(std::move(input));
			}

			value_type _impl;
		};

		BSA_NODISCARD inline bool operator==(const file& a_lhs, const file& a_rhs) noexcept
		{
			if (!a_lhs && !a_rhs) {	 // neither have value
				return true;
			} else if (!a_lhs != !a_rhs) {	// one has value
				return false;
			} else {  // both have value
				return a_lhs._impl->hash_ref() == a_rhs._impl->hash_ref();
			}
		}

		BSA_NODISCARD inline bool operator!=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs == a_rhs); }

		BSA_NODISCARD inline bool operator<(const file& a_lhs, const file& a_rhs) noexcept
		{
			if (!a_lhs && !a_rhs) {	 // neither have value
				return true;
			} else if (!a_lhs != !a_rhs) {	// one has value
				return a_lhs.exists();
			} else {  // both have value
				return a_lhs._impl->hash_ref() < a_rhs._impl->hash_ref();
			}
		}

		BSA_NODISCARD inline bool operator>(const file& a_lhs, const file& a_rhs) noexcept { return a_rhs < a_lhs; }
		BSA_NODISCARD inline bool operator<=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs > a_rhs); }
		BSA_NODISCARD inline bool operator>=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs < a_rhs); }

		inline void swap(file& a_lhs, file& a_rhs) noexcept { a_lhs.swap(a_rhs); }

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

			BSA_NODISCARD inline reference operator*() noexcept { return fetch(); }
			BSA_NODISCARD inline pointer operator->() noexcept { return std::addressof(fetch()); }

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
			friend class archive;

			template <class InputIt>
			explicit inline file_iterator(InputIt a_first, InputIt a_last) :
				_files(nullptr),
				_pos(NPOS)
			{
				if (a_first != a_last) {
					_files = std::make_shared<container_type>();
					_pos = 0;
					do {
						_files->push_back(
							value_type(
								static_cast<const detail::file_ptr&>(*a_first)));
						++a_first;
					} while (a_first != a_last);
				}
			}

		private:
			using container_type = std::vector<value_type>;

			BSA_NODISCARD inline reference fetch() noexcept
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

		class archive final
		{
		public:
			using iterator = file_iterator;
			using const_iterator = iterator;

			archive() noexcept = default;
			archive(const archive&) = default;
			archive(archive&&) noexcept = default;

			inline archive(const boost::filesystem::path& a_path) :
				_files(),
				_header()
			{
				read(a_path);
			}

			~archive() = default;

			archive& operator=(const archive&) = default;
			archive& operator=(archive&&) noexcept = default;

			BSA_NODISCARD inline file front() const noexcept
			{
				assert(!empty());
				return file(_files.front());
			}

			BSA_NODISCARD inline file back() const noexcept
			{
				assert(!empty());
				return file(_files.back());
			}

			BSA_NODISCARD inline iterator begin() const { return iterator(_files.begin(), _files.end()); }
			BSA_NODISCARD inline iterator end() const noexcept { return iterator(); }

			BSA_NODISCARD constexpr std::size_t size() const noexcept { return file_count(); }

			BSA_NODISCARD inline std::size_t size_bytes() const
			{
				std::size_t sz{ 0 };
				sz += detail::header_t::block_size();
				sz += detail::file_t::block_size() * file_count();
				sz += sizeof(std::uint32_t) * file_count();
				sz += _header.hash_offset() - calc_file_size();
				sz += detail::hash_t::block_size() * file_count();
				for (const auto& file : _files) {
					sz += file->size();
				}

				return sz;
			}

			BSA_NODISCARD constexpr bool empty() const noexcept { return size() == 0; }

			inline void clear() noexcept
			{
				_files.clear();
				_header.clear();
			}

			BSA_NODISCARD constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			BSA_NODISCARD constexpr archive_version version() const noexcept { return _header.version(); }

			inline void read(const boost::filesystem::path& a_path)
			{
				detail::istream_t input(a_path);

				clear();

				_header.read(input);
				switch (version()) {
				case v256:
					break;
				default:
					throw version_error();
				}

				read_initial(input);
				read_filenames(input);
				read_hashes(input);
				read_data(input);

				sort();

				assert(sanity_check());
			}

			inline void extract(const boost::filesystem::path& a_path)
			{
				if (!boost::filesystem::exists(a_path)) {
					throw output_error();
				}

				boost::filesystem::path filePath;
				std::ofstream output;
				for (auto& file : _files) {
					output.close();
					filePath = a_path / file->string();
					boost::filesystem::create_directories(filePath.parent_path());
					output.open(filePath.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
					if (!output.is_open()) {
						throw output_error();
					}
					file->extract(output);
				}
			}

			inline void write(const boost::filesystem::path& a_path)
			{
				std::ofstream file{ a_path.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc };
				if (!file.is_open()) {
					throw output_error();
				} else {
					write(file);
				}
			}

			inline void write(std::ostream& a_output)
			{
				detail::ostream_t output(a_output);

				prepare_for_write();

				_header.write(output);
				for (const auto& file : _files) {
					file->write(output);
				}

				std::uint32_t offset = 0;
				for (const auto& file : _files) {
					output << offset;
					offset += detail::zero_extend<std::uint32_t>(file->name_size());
				}

				for (const auto& file : _files) {
					file->write_name(output);
				}

				for (const auto& file : _files) {
					file->write_hash(output);
				}

				for (const auto& file : _files) {
					file->write_data(output);
				}
			}

			inline void insert(const file& a_file)
			{
				if (!can_insert(a_file.file_ptr())) {
					throw size_error{};
				} else if (!a_file || a_file.empty()) {
					throw empty_file{};
				} else if (!contains(a_file)) {
					_files.reserve(size() + 1);
					_files.push_back(a_file.file_ptr());
					sort();
					update_size();
				}
			}

			template <class InputIt>
			inline void insert(InputIt a_first, InputIt a_last)
			{
				container_t toInsert;
				while (a_first != a_last) {
					auto& f = static_cast<const file&>(*a_first);
					if (!f || f.empty()) {
						throw empty_file{};
					} else if (!contains(f)) {
						toInsert.push_back(f.file_ptr());
					}
					++a_first;
				}

				std::sort(toInsert.begin(), toInsert.end(), file_sorter());
				auto newEnd = std::unique(
					toInsert.begin(),
					toInsert.end(),
					[](const value_t& a_lhs, const value_t& a_rhs) noexcept -> bool {
						return a_lhs->hash_ref() == a_rhs->hash_ref();
					});
				toInsert.erase(newEnd, toInsert.end());

				if (!can_insert(toInsert)) {
					throw size_error{};
				}

				_files.reserve(size() + toInsert.size());
				for (auto& elem : toInsert) {
					_files.push_back(elem);
				}

				sort();
				update_size();
			}

			inline void insert(std::initializer_list<file> a_initList)
			{
				insert(a_initList.begin(), a_initList.end());
			}

			inline bool erase(const file& a_file)
			{
				if (!a_file) {
					return false;
				}

				const auto hash = a_file.file_ptr()->hash();
				auto it = binary_find(hash);
				if (it == _files.end()) {
					return false;
				}

				_files.erase(it);
				return true;
			}

			BSA_NODISCARD inline file find(const boost::filesystem::path& a_path)
			{
				const auto hash = detail::file_hasher()(a_path);
				auto it = binary_find(hash);
				return it != _files.end() ? file(*it) : file();
			}

			BSA_NODISCARD inline bool contains(const file& a_file)
			{
				if (!a_file) {
					return false;
				} else {
					const auto hash = a_file.file_ptr()->hash();
					auto it = binary_find(hash);
					return it != _files.end();
				}
			}

		private:
			using value_t = detail::file_ptr;
			using container_t = std::vector<value_t>;
			using iterator_t = typename container_t::iterator;

			class file_sorter final
			{
			public:
				BSA_NODISCARD inline bool operator()(const value_t& a_lhs, const value_t& a_rhs) const noexcept
				{
					return a_lhs->hash_ref() < a_rhs->hash_ref();
				}

				BSA_NODISCARD inline bool operator()(const value_t& a_lhs, const detail::hash_t& a_rhs) const noexcept
				{
					return a_lhs->hash_ref() < a_rhs;
				}
			};

			inline iterator_t binary_find(const detail::hash_t& a_hash)
			{
				auto it = _files.begin();
				const auto itEnd = _files.end();
				it = std::lower_bound(it, itEnd, a_hash, file_sorter());

				return it != _files.end() && (*it)->hash_ref() == a_hash ? it : itEnd;
			}

			BSA_NODISCARD inline std::size_t calc_file_size() const noexcept
			{
				return calc_file_size(_files);
			}

			BSA_NODISCARD inline std::size_t calc_file_size(const container_t& a_files) const noexcept
			{
				return (detail::file_t::block_size() + 0x4) * a_files.size();
			}

			BSA_NODISCARD inline std::size_t calc_hash_offset() const noexcept
			{
				return calc_hash_offset(_files);
			}

			BSA_NODISCARD inline std::size_t calc_hash_offset(const container_t& a_files) const noexcept
			{
				return calc_file_size() + calc_names_size(a_files);
			}

			BSA_NODISCARD inline std::size_t calc_names_size() const noexcept
			{
				return calc_names_size(_files);
			}

			BSA_NODISCARD inline std::size_t calc_names_size(const container_t& a_files) const noexcept
			{
				std::size_t namesSize = 0;
				for (auto& file : a_files) {
					namesSize += file->name_size();
				}
				return namesSize;
			}

			BSA_NODISCARD inline bool can_insert(const value_t& a_file)
			{
				if (size() + 1 > detail::max_int32 ||
					!validate_hash_offsets(a_file) ||
					!validate_name_offsets(a_file) ||
					!validate_data_offsets(a_file)) {
					return false;
				} else {
					return true;
				}
			}

			BSA_NODISCARD inline bool can_insert(const container_t& a_files)
			{
				if (size() + a_files.size() > detail::max_int32) {
					return false;
				}

				container_t merge;
				std::merge(
					_files.begin(),
					_files.end(),
					a_files.begin(),
					a_files.end(),
					std::back_inserter(merge),
					file_sorter());
				if (!validate_hash_offsets(merge) ||
					!validate_name_offsets(merge) ||
					!validate_data_offsets(merge)) {
					return false;
				}

				return true;
			}

			inline void prepare_for_write()
			{
				update_header();
				update_files();
			}

			inline void read_data(detail::istream_t& a_input)
			{
				auto pos = _header.hash_offset();
				pos += detail::header_t::block_size();
				pos += detail::hash_t::block_size() * file_count();
				a_input.seek_beg(pos);

				for (auto& file : _files) {
					file->read_data(a_input);
				}
			}

			inline void read_filenames(detail::istream_t& a_input)
			{
				std::vector<std::uint32_t> offsets(file_count());
				for (auto& offset : offsets) {
					a_input >> offset;
				}

				const auto pos = a_input.tell();
				for (std::size_t i = 0; i < file_count(); ++i) {
					a_input.seek_abs(pos + offsets[i]);
					_files[i]->read_name(a_input);
				}
			}

			inline void read_hashes(detail::istream_t& a_input)
			{
				auto pos = _header.hash_offset();
				pos += detail::header_t::block_size();
				a_input.seek_beg(pos);

				for (auto& file : _files) {
					file->read_hash(a_input);
				}
			}

			inline void read_initial(detail::istream_t& a_input)
			{
				_files.reserve(file_count());
				for (std::size_t i = 0; i < file_count(); ++i) {
					auto file = std::make_shared<detail::file_t>();
					file->read(a_input);
					_files.push_back(std::move(file));
				}
			}

			inline bool sanity_check()
			{
				detail::hash_t hash;
				for (auto& file : _files) {
					hash = detail::file_hasher()({ file->string() });
					if (hash != file->hash_ref()) {
						return false;
					}
				}

				return true;
			}

			inline void sort() { std::sort(_files.begin(), _files.end(), file_sorter()); }

			inline void update_files()
			{
				std::size_t offset = 0;
				for (auto& file : _files) {
					file->set_offset(offset);
					offset += file->size();
				}
			}

			inline void update_header()
			{
				const auto hashOffset = calc_hash_offset();
				_header.hash_offset(hashOffset);
				update_size();
			}

			inline void update_size()
			{
				_header.file_count(_files.size());
			}

			BSA_NODISCARD inline bool validate_data_offsets(const value_t& a_file)
			{
				return validate_offsets(_files, a_file, file_sorter(), [](const value_t& a_val) noexcept -> std::size_t {
					return a_val->size();
				});
			}

			BSA_NODISCARD inline bool validate_data_offsets(const container_t& a_files)
			{
				return validate_offsets(a_files, [](const value_t& a_val) noexcept -> std::size_t {
					return a_val->size();
				});
			}

			BSA_NODISCARD inline bool validate_hash_offsets(const value_t& a_file) noexcept
			{
				auto offset = calc_hash_offset();
				offset += detail::file_t::block_size() + 0x4;
				offset += a_file->name_size();
				return offset <= detail::max_int32;
			}

			BSA_NODISCARD inline bool validate_hash_offsets(const container_t& a_files) noexcept
			{
				auto offset = calc_hash_offset(a_files);
				if (offset > detail::max_int32) {
					return false;
				}

				for (auto& file : a_files) {
					offset += file->name_size();
					if (offset > detail::max_int32) {
						return false;
					}
				}

				return true;
			}

			BSA_NODISCARD inline bool validate_name_offsets(const value_t& a_file)
			{
				return validate_offsets(_files, a_file, file_sorter(), [](const value_t& a_val) noexcept -> std::size_t {
					return a_val->name_size();
				});
			}

			BSA_NODISCARD inline bool validate_name_offsets(const container_t& a_files)
			{
				return validate_offsets(a_files, [](const value_t& a_val) noexcept -> std::size_t {
					return a_val->name_size();
				});
			}

			// std::function<std::size_t(const value_t&)>
			template <
				class Sorter,
				class UnaryFunctor,
				stl::enable_if_t<
					stl::is_invocable_r_v<
						std::size_t,
						UnaryFunctor,
						const value_t&>,
					int> = 0>
			BSA_NODISCARD inline bool validate_offsets(const container_t& a_files, const value_t& a_file, Sorter a_sorter, UnaryFunctor a_func)
			{
				auto lower = std::lower_bound(a_files.begin(), a_files.end(), a_file, a_sorter);
				std::size_t offset = 0;

				for (auto it = a_files.begin(); it != lower; ++it) {
					offset += a_func(*it);
				}

				if (lower != a_files.end()) {
					offset += a_func(a_file);
					if (offset > detail::max_int32) {
						return false;
					}

					for (auto it = lower; it != a_files.end(); ++it) {
						offset += a_func(*it);
						if (offset > detail::max_int32) {
							return false;
						}
					}
				}

				return true;
			}

			// std::function<std::size_t(const value_t&)>
			template <
				class UnaryFunctor,
				stl::enable_if_t<
					stl::is_invocable_r_v<
						std::size_t,
						UnaryFunctor,
						const value_t&>,
					int> = 0>
			BSA_NODISCARD inline bool validate_offsets(const container_t& a_files, UnaryFunctor a_func)
			{
				if (a_files.size() > 0) {
					std::size_t offset = 0;
					auto last = a_files.end();
					--last;
					for (auto it = a_files.begin(); it != last; ++it) {
						offset += a_func(*it);
						if (offset > detail::max_int32) {
							return false;
						}
					}
				}

				return true;
			}

			container_t _files;
			detail::header_t _header;
		};

		inline archive& operator<<(archive& a_archive, const boost::filesystem::path& a_path)
		{
			a_archive.read(a_path);
			return a_archive;
		}

		inline archive& operator<<(archive& a_archive, const file& a_file)
		{
			a_archive.insert(a_file);
			return a_archive;
		}

		inline archive& operator>>(archive& a_archive, const boost::filesystem::path& a_path)
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
