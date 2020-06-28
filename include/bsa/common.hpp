#pragma once

#include "bsa/stl.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/iostreams/device/mapped_file.hpp>

// TODO
#pragma warning(disable : 4820)	 // 'bytes' bytes padding added after construct 'member_name'

static_assert(std::numeric_limits<std::int8_t>::digits + 1 == 8);
static_assert(std::numeric_limits<std::uint8_t>::digits == 8);
static_assert(std::numeric_limits<std::int16_t>::digits + 1 == 16);
static_assert(std::numeric_limits<std::uint16_t>::digits == 16);
static_assert(std::numeric_limits<std::int32_t>::digits + 1 == 32);
static_assert(std::numeric_limits<std::uint32_t>::digits == 32);
static_assert(std::numeric_limits<std::int64_t>::digits + 1 == 64);
static_assert(std::numeric_limits<std::uint64_t>::digits == 64);

namespace bsa
{
	class exception : public std::exception
	{
	private:
		using super = std::exception;

	public:
		inline exception() noexcept :
			exception("base archive exception")
		{}

		inline exception(const exception&) = default;
		inline exception(exception&&) = default;

		inline exception(const char* a_what) noexcept :
			super(),
			_what(a_what)
		{}

		~exception() = default;

		exception& operator=(const exception&) = default;
		exception& operator=(exception&&) = default;

		BSA_NODISCARD const char* what() const noexcept override { return _what; }

	private:
		const char* _what;
	};

	// typically thrown when converting from std::size_t to uint32_t/int32_t for writes
	class size_error : public exception
	{
	private:
		using super = exception;

	public:
		inline size_error() noexcept :
			size_error("an integer was larger than what a field could hold")
		{}

		inline size_error(const size_error&) = default;
		inline size_error(size_error&&) = default;

		inline size_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~size_error() = default;

		size_error& operator=(const size_error&) = default;
		size_error& operator=(size_error&&) = default;
	};

	class hash_error : public exception
	{
	private:
		using super = exception;

	public:
		inline hash_error() noexcept :
			hash_error("encountered an error during hash generation")
		{}

		inline hash_error(const hash_error&) = default;
		inline hash_error(hash_error&&) = default;

		inline hash_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~hash_error() = default;

		hash_error& operator=(const hash_error&) = default;
		hash_error& operator=(hash_error&&) = default;
	};

	// non-ascii characters have negative values, and beth doesn't cast them to their unsigned
	// counterparts while remapping them, so you get something like "remaptable[-17]" which
	// is possibly the most bethesda thing they could do
	class hash_non_ascii : public hash_error
	{
	private:
		using super = hash_error;

	public:
		inline hash_non_ascii() noexcept :
			hash_non_ascii("encountered a non ascii character during hash generation")
		{}

		inline hash_non_ascii(const hash_non_ascii&) = default;
		inline hash_non_ascii(hash_non_ascii&&) = default;

		inline hash_non_ascii(const char* a_what) noexcept :
			super(a_what)
		{}

		~hash_non_ascii() = default;

		hash_non_ascii& operator=(const hash_non_ascii&) = default;
		hash_non_ascii& operator=(hash_non_ascii&&) = default;
	};

	class hash_empty : public hash_error
	{
	private:
		using super = hash_error;

	public:
		inline hash_empty() noexcept :
			hash_empty("the given path was empty")
		{}

		inline hash_empty(const hash_empty&) = default;
		inline hash_empty(hash_empty&&) = default;

		inline hash_empty(const char* a_what) noexcept :
			super(a_what)
		{}

		~hash_empty() = default;

		hash_empty& operator=(const hash_empty&) = default;
		hash_empty& operator=(hash_empty&&) = default;
	};

	class io_error : public exception
	{
	private:
		using super = exception;

	public:
		inline io_error() noexcept :
			io_error("failure while performing i/o with the archive")
		{}

		inline io_error(const io_error&) = default;
		inline io_error(io_error&&) = default;

		inline io_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~io_error() = default;

		io_error& operator=(const io_error&) = default;
		io_error& operator=(io_error&&) = default;
	};

	class input_error : public io_error
	{
	private:
		using super = io_error;

	public:
		inline input_error() noexcept :
			input_error("failure while performing input")
		{}

		inline input_error(const input_error&) = default;
		inline input_error(input_error&&) = default;

		inline input_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~input_error() = default;

		input_error& operator=(const input_error&) = default;
		input_error& operator=(input_error&&) = default;
	};

	class version_error : public input_error
	{
	private:
		using super = input_error;

	public:
		inline version_error() noexcept :
			version_error("encountered unhandled version")
		{}

		inline version_error(const version_error&) = default;
		inline version_error(version_error&&) = default;

		inline version_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~version_error() = default;

		version_error& operator=(const version_error&) = default;
		version_error& operator=(version_error&&) = default;
	};

	class empty_file : public input_error
	{
	private:
		using super = input_error;

	public:
		inline empty_file() noexcept :
			empty_file("file was empty")
		{}

		inline empty_file(const empty_file&) = default;
		inline empty_file(empty_file&&) = default;

		inline empty_file(const char* a_what) noexcept :
			super(a_what)
		{}

		~empty_file() = default;

		empty_file& operator=(const empty_file&) = default;
		empty_file& operator=(empty_file&&) = default;
	};

	class output_error : public io_error
	{
	private:
		using super = io_error;

	public:
		inline output_error() noexcept :
			output_error("failure while performing output")
		{}

		inline output_error(const output_error&) = default;
		inline output_error(output_error&&) = default;

		inline output_error(const char* a_what) noexcept :
			super(a_what)
		{}

		~output_error() = default;

		output_error& operator=(const output_error&) = default;
		output_error& operator=(output_error&&) = default;
	};

	template <
		class T,
		class =
			stl::enable_if_t<
				stl::is_pointer_v<T>>>
	using observer = T;	 // non-owning raw pointer

	template <
		class T,
		class =
			stl::enable_if_t<
				stl::is_pointer_v<T>>>
	using owner = T;  // owning raw pointer

	namespace detail
	{
		// sign extending cast
		template <
			class To,
			class From,
			stl::enable_if_t<
				stl::conjunction_v<
					stl::disjunction<
						std::is_integral<To>,
						std::is_enum<To>>,
					stl::disjunction<
						std::is_integral<From>,
						std::is_enum<From>>,
					stl::negation<
						std::is_same<To, bool>>,
					stl::negation<
						std::is_same<From, bool>>>,
				int> = 0>
		BSA_NODISCARD constexpr To sign_extend(From a_from) noexcept
		{
			const auto to = static_cast<To>(a_from);
			assert(static_cast<From>(static_cast<stl::make_unsigned_t<To>>(to)) == a_from);
			return to;
		}

		// zero extending cast
		template <
			class To,
			class From,
			stl::enable_if_t<
				stl::conjunction_v<
					stl::disjunction<
						std::is_integral<To>,
						std::is_enum<To>>,
					stl::disjunction<
						std::is_integral<From>,
						std::is_enum<From>>,
					stl::negation<
						std::is_same<To, bool>>,
					stl::negation<
						std::is_same<From, bool>>>,
				int> = 0>
		BSA_NODISCARD constexpr To zero_extend(From a_from) noexcept
		{
			const auto to =
				static_cast<To>(
					static_cast<stl::make_unsigned_t<From>>(a_from));
			assert(static_cast<From>(static_cast<stl::make_unsigned_t<To>>(to)) == a_from);
			return to;
		}

		template <class T, std::size_t N>
		BSA_NODISCARD constexpr decltype(auto) at(T (&a_array)[N], std::size_t a_idx) noexcept
		{
			assert(a_idx < N);
			return a_array[a_idx];
		}

		// Bethesda uses std::tolower to convert chars to lowercase, however
		// they use the default C locale to convert the characters,
		// so I've emulated this functionality, which enables a constexpr
		// mapping, and allows users to set the locale without encountering
		// unexpected hashing behavior
		BSA_NODISCARD constexpr char mapchar(char a_ch) noexcept
		{
			constexpr auto MAP = []() noexcept {
				constexpr auto MAX =
					zero_extend<std::size_t>((std::numeric_limits<unsigned char>::max)()) + 1;

				std::array<char, MAX> map{};
				std::size_t i = 0;
				for (auto& ch : map) {
					ch = zero_extend<char>(i++);
				}

				map[zero_extend<std::size_t>('/')] = '\\';

				i = zero_extend<std::size_t>('A');
				char j = 'a';
				while (i <= zero_extend<std::size_t>('Z')) {
					map[i++] = j++;
				}

				return map;
			}();

			return MAP[zero_extend<std::size_t>(a_ch)];
		}

		BSA_CXX17_INLINE constexpr auto byte_v{
			zero_extend<std::size_t>(
				std::numeric_limits<std::uint8_t>::digits)
		};

		BSA_CXX17_INLINE constexpr auto max_int32{
			zero_extend<std::size_t>(
				(std::numeric_limits<std::int32_t>::max)())
		};

		BSA_CXX17_INLINE constexpr auto max_uint32{
			zero_extend<std::size_t>(
				(std::numeric_limits<std::uint32_t>::max)())
		};

		enum class endian
		{
			little,
			big
		};

		class path_t final
		{
		public:
			using value_type = std::string;
			using iterator = typename value_type::iterator;
			using const_iterator = typename value_type::const_iterator;
			using reverse_iterator = typename value_type::reverse_iterator;
			using const_reverse_iterator = typename value_type::const_reverse_iterator;

			path_t() noexcept = default;
			path_t(const path_t&) = default;
			path_t(path_t&&) noexcept = default;

			inline path_t(stl::string_view a_path) :
				_impl()
			{
				normalize(a_path);
			}

			~path_t() = default;

			path_t& operator=(const path_t&) = default;
			path_t& operator=(path_t&&) noexcept = default;

			inline path_t& operator=(stl::string_view a_path)
			{
				normalize(a_path);
				return *this;
			}

			BSA_NODISCARD inline iterator begin() noexcept { return _impl.begin(); }
			BSA_NODISCARD inline const_iterator begin() const noexcept { return _impl.begin(); }
			BSA_NODISCARD inline const_iterator cbegin() const noexcept { return _impl.cbegin(); }

			BSA_NODISCARD inline iterator end() noexcept { return _impl.end(); }
			BSA_NODISCARD inline const_iterator end() const noexcept { return _impl.end(); }
			BSA_NODISCARD inline const_iterator cend() const noexcept { return _impl.cend(); }

			BSA_NODISCARD inline reverse_iterator rbegin() noexcept { return _impl.rbegin(); }
			BSA_NODISCARD inline const_reverse_iterator rbegin() const noexcept { return _impl.rbegin(); }
			BSA_NODISCARD inline const_reverse_iterator crbegin() const noexcept { return _impl.crbegin(); }

			BSA_NODISCARD inline reverse_iterator rend() noexcept { return _impl.rend(); }
			BSA_NODISCARD inline const_reverse_iterator rend() const noexcept { return _impl.rend(); }
			BSA_NODISCARD inline const_reverse_iterator crend() const noexcept { return _impl.crend(); }

			BSA_NODISCARD inline bool empty() const noexcept { return _impl.empty(); }

			BSA_NODISCARD inline const char* c_str() const noexcept { return _impl.c_str(); }
			BSA_NODISCARD inline std::string string() const { return _impl; }
			BSA_NODISCARD inline stl::string_view string_view() const { return _impl; }

		private:
			inline void normalize(stl::string_view a_path)
			{
				stl::filesystem::path p(a_path.begin(), a_path.end());
				_impl = p.lexically_normal().string();

				for (auto& ch : _impl) {
					ch = mapchar(ch);
				}

				if (!_impl.empty() && _impl.back() == '\\') {
					_impl.pop_back();
				}

				if (!_impl.empty() && _impl.front() == '\\') {
					_impl = _impl.substr(1);
				}
			}

			value_type _impl;
		};

		class istream_t final
		{
		public:
			using stream_type = boost::iostreams::mapped_file_source;

			using value_type = const stl::byte;
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using const_pointer = stl::add_const_t<value_type>*;
			using reference = value_type&;
			using const_reference = stl::add_const_t<value_type>&;

			inline istream_t() noexcept :
				_stream(),
				_pos(0),
				_endian(endian::little)
			{}

			inline istream_t(const istream_t& a_rhs) noexcept :
				_stream(a_rhs._stream),
				_pos(a_rhs._pos),
				_endian(a_rhs._endian)
			{}

			inline istream_t(istream_t&& a_rhs) noexcept :
				_stream(std::move(a_rhs._stream)),
				_pos(std::move(a_rhs._pos)),
				_endian(std::move(a_rhs._endian))
			{}

			inline istream_t(stream_type a_stream) :
				_stream(std::move(a_stream)),
				_pos(0),
				_endian(endian::little)
			{}

			inline istream_t(const stl::filesystem::path& a_path) :
				_stream(),
				_pos(0),
				_endian(endian::little)
			{
				open(a_path);
			}

			~istream_t() noexcept = default;

			inline istream_t& operator=(const istream_t& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_stream = a_rhs._stream;
					_pos = a_rhs._pos;
					_endian = a_rhs._endian;
				}
				return *this;
			}

			inline istream_t& operator=(istream_t&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_stream = std::move(a_rhs._stream);
					_pos = std::move(a_rhs._pos);
					_endian = std::move(a_rhs._endian);
				}
				return *this;
			}

			template <
				class T,
				stl::enable_if_t<
					stl::disjunction_v<
						std::is_integral<T>,
						std::is_enum<T>>,
					int> = 0>
			inline istream_t& operator>>(T& a_value)
			{
				using integer_t =
					std::conditional_t<
						(sizeof(T) > sizeof(std::size_t)),
						stl::make_unsigned_t<T>,
						std::size_t>;
				integer_t tmp{ 0 };

				switch (_endian) {
				case endian::little:
					for (std::size_t i = 0; i < sizeof(T); ++i) {
						tmp |= ref<integer_t>(_pos++) << i * byte_v;
					}
					break;
				case endian::big:
					for (auto i = zero_extend<std::ptrdiff_t>(sizeof(T)); i >= 0; --i) {
						tmp |= ref<integer_t>(_pos++) << i * byte_v;
					}
					break;
				default:
					throw input_error();
				}

				a_value = zero_extend<T>(tmp);
				return *this;
			}

			template <std::size_t N>
			inline istream_t& operator>>(std::array<char, N>& a_value)
			{
				read(a_value.begin(), a_value.size());
				return *this;
			}

			constexpr istream_t& operator>>(endian a_endian) noexcept
			{
				_endian = a_endian;
				return *this;
			}

			BSA_NODISCARD inline bool operator!() const { return !is_open(); }
			BSA_NODISCARD explicit inline operator bool() const { return is_open(); }

			inline void get(char& a_ch)
			{
				a_ch = static_cast<char>(ref(_pos++));
			}

			template <class OutputIt>
			inline void read(OutputIt a_dst, size_type a_count)
			{
				assert(_pos + a_count <= size());
				std::copy_n(reinterpret_cast<const char*>(ptr(_pos)), a_count, a_dst);
				_pos += a_count;
			}

			BSA_NODISCARD constexpr size_type tell() const noexcept { return _pos; }

			// seek absolute position
			inline void seek_abs(size_type a_pos) noexcept
			{
				assert(a_pos < size());
				_pos = a_pos;
			}

			constexpr void seek_beg() noexcept { _pos = 0; }					 // seek to beginning
			inline void seek_beg(size_type a_pos) noexcept { seek_abs(a_pos); }	 // seek from beginning

			// seek relative to current position
			template <
				class T,
				stl::enable_if_t<
					stl::disjunction_v<
						std::is_integral<T>,
						std::is_enum<T>>,
					int> = 0>
			inline void seek_rel(T a_off) noexcept
			{
				assert(_pos + a_off < size());
				_pos += a_off;
			}

			BSA_NODISCARD inline bool is_open() const { return _stream.is_open(); }

			inline void open(const stl::filesystem::path& a_path)
			{
				auto fail = false;
				try {
#if __cplusplus >= 201703L
					auto path = a_path.string();
#else
					auto& path = a_path;
#endif
					_stream.open(path);
				} catch (...) {
					fail = true;
				}

				if (fail || !is_open()) {
					throw input_error();
				}
			}

			inline void close() { _stream.close(); }

			BSA_NODISCARD inline size_type size() const noexcept
			{
				try {
					return _stream.size();
				} catch (...) {
					return 0;
				}
			}

			inline stl::span<value_type> subspan() const { return subspan(size()); }
			inline stl::span<value_type> subspan(size_type a_count) const { return subspan(_pos, a_count); }

			inline stl::span<value_type> subspan(size_type a_offset, size_type a_count) const
			{
				assert(a_offset + a_count <= size());
				return { ptr(a_offset), a_count };
			}

		private:
			BSA_NODISCARD inline observer<pointer> data() const
			{
				assert(is_open());
				return reinterpret_cast<pointer>(_stream.data());
			}

			BSA_NODISCARD inline observer<pointer> fetch(size_type a_pos) const
			{
				assert(a_pos < size());
				return data() + a_pos;
			}

			BSA_NODISCARD inline observer<pointer> ptr(size_type a_pos) const { return fetch(a_pos); }
			BSA_NODISCARD inline reference ref(size_type a_pos) const { return *fetch(a_pos); }

			template <
				class T,
				stl::enable_if_t<
					stl::disjunction_v<
						std::is_integral<T>,
						std::is_enum<T>>,
					int> = 0>
			BSA_NODISCARD inline T ref(size_type a_pos) const
			{
				return zero_extend<T>(ref(a_pos));
			}

			stream_type _stream;
			size_type _pos;
			endian _endian;
		};

		class ostream_t final
		{
		public:
			using stream_type = std::ostream;
			using reference = stream_type&;
			using const_reference = const stream_type&;
			using pointer = stream_type*;
			using const_pointer = const stream_type*;
			using char_type = typename stream_type::char_type;
			using pos_type = typename stream_type::pos_type;
			using off_type = typename stream_type::off_type;

			inline ostream_t() = delete;
			inline ostream_t(const ostream_t&) = delete;
			inline ostream_t(ostream_t&&) = delete;

			inline ostream_t(stream_type& a_stream) :
				_stream(a_stream),
				_beg(a_stream.tellp()),
				_endian(endian::little)
			{
				if (!_stream) {
					throw output_error();
				} else {
					_stream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
					_stream.flags(std::ios_base::dec);
				}
			}

			ostream_t& operator=(const ostream_t&) = delete;
			ostream_t& operator=(ostream_t&&) = delete;

			template <
				class T,
				stl::enable_if_t<
					stl::disjunction_v<
						std::is_integral<T>,
						std::is_enum<T>>,
					int> = 0>
			inline ostream_t& operator<<(T a_value)
			{
				using integer_t =
					std::conditional_t<
						(sizeof(T) > sizeof(unsigned int)),
						stl::make_unsigned_t<T>,
						unsigned int>;
				constexpr auto MASK =
					zero_extend<std::size_t>(
						(std::numeric_limits<std::uint8_t>::max)());

				const auto value = zero_extend<integer_t>(a_value);
				std::array<stl::byte, sizeof(T)> buf;
				auto it = buf.begin();

				switch (_endian) {
				case endian::little:
					for (std::size_t i = 0; i < sizeof(T); ++i) {
						*it++ = zero_extend<stl::byte>((value >> i * byte_v) & MASK);
					}
					break;
				case endian::big:
					for (auto i = zero_extend<std::ptrdiff_t>(sizeof(T)); i >= 0; --i) {
						*it++ = zero_extend<stl::byte>((value >> i * byte_v) & MASK);
					}
					break;
				default:
					throw output_error();
				}

				return *this << buf;
			}

			inline ostream_t& operator<<(char a_value)
			{
				return write(std::addressof(a_value), 1);
			}

			template <std::size_t N>
			inline ostream_t& operator<<(const std::array<stl::byte, N>& a_value)
			{
				return write(
					reinterpret_cast<const char*>(a_value.data()),
					zero_extend<std::streamsize>(a_value.size()));
			}

			template <std::size_t N>
			inline ostream_t& operator<<(const std::array<char_type, N>& a_value)
			{
				return write(a_value.data(), zero_extend<std::streamsize>(a_value.size()));
			}

			inline ostream_t& operator<<(stl::span<stl::byte> a_value)
			{
				return write(
					reinterpret_cast<const char*>(a_value.data()),
					zero_extend<std::streamsize>(a_value.size()));
			}

			inline ostream_t& operator<<(stl::span<const stl::byte> a_value)
			{
				return write(
					reinterpret_cast<const char*>(a_value.data()),
					zero_extend<std::streamsize>(a_value.size()));
			}

			inline ostream_t& operator<<(stl::span<char_type> a_value)
			{
				return write(a_value.data(), zero_extend<std::streamsize>(a_value.size()));
			}

			inline ostream_t& operator<<(stl::span<const char_type> a_value)
			{
				return write(a_value.data(), zero_extend<std::streamsize>(a_value.size()));
			}

			inline ostream_t& operator<<(stl::basic_string_view<char_type> a_value)
			{
				return write(a_value.data(), zero_extend<std::streamsize>(a_value.size()));
			}

			constexpr ostream_t& operator<<(endian a_endian) noexcept
			{
				_endian = a_endian;
				return *this;
			}

			BSA_NODISCARD inline bool operator!() const { return !_stream; }
			BSA_NODISCARD explicit inline operator bool() const { return static_cast<bool>(_stream); }

			inline ostream_t& write(observer<const char_type*> a_str, std::streamsize a_count)
			{
				_stream.write(a_str, a_count);
				return *this;
			}

			BSA_NODISCARD inline pos_type tell() { return _stream.tellp(); }
			BSA_NODISCARD inline pos_type tell_rel() { return tell() - _beg; }

			// seek absolute position
			inline ostream_t& seek_abs(pos_type a_pos)
			{
				_stream.seekp(a_pos);
				return *this;
			}

			// seek to beginning
			inline ostream_t& seek_beg()
			{
				_stream.seekp(_beg);
				return *this;
			}

			// seek from beginning
			inline ostream_t& seek_beg(pos_type a_pos)
			{
				_stream.seekp(_beg + a_pos);
				return *this;
			}

			// seek relative to current position
			inline ostream_t& seek_rel(off_type a_off)
			{
				_stream.seekp(a_off, std::ios_base::cur);
				return *this;
			}

		private:
			stream_type& _stream;
			pos_type _beg;
			endian _endian;
		};

		class BSA_MAYBE_UNUSED restore_point final
		{
		public:
			using stream_type = istream_t;

			restore_point() = delete;
			restore_point(const restore_point&) = delete;

			inline restore_point(restore_point&& a_rhs) noexcept :
				_stream(std::move(a_rhs._stream)),
				_pos(std::move(a_rhs._pos))
			{
				a_rhs._stream.reset();
				a_rhs._pos = 0;
			}

			BSA_CXX17_CONSTEXPR restore_point(stream_type& a_stream) :
				_stream(stl::in_place, a_stream),
				_pos(a_stream.tell())
			{}

			inline ~restore_point()
			{
				if (_stream) {
					_stream->get().seek_abs(_pos);
				}
			}

			restore_point& operator=(const restore_point&) = delete;

			constexpr restore_point& operator=(restore_point&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_stream = std::move(a_rhs._stream);
					a_rhs._stream.reset();

					_pos = std::move(a_rhs._pos);
					a_rhs._pos = 0;
				}
				return *this;
			}

		private:
			stl::optional<std::reference_wrapper<stream_type>> _stream;
			std::size_t _pos;
		};
	}
}
