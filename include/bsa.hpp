#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <istream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>


namespace bsa
{
	class exception : public std::exception
	{
	public:
		inline exception() noexcept : exception("base archive exception") {}
		inline exception(const exception&) noexcept = default;
		inline exception(const char* a_what) noexcept : std::exception(), _what(a_what) {}
		virtual ~exception() noexcept = default;

		exception& operator=(const exception&) noexcept = default;

		virtual const char* what() const noexcept override { return _what; }

	private:
		const char* _what;
	};


	// typically thrown when converting from std::size_t to std::uint32_t/std::int32_t for writes
	class size_error : public exception
	{
	public:
		inline size_error() noexcept : size_error("an integer was larger than what a field could hold") {}
		inline size_error(const size_error&) noexcept = default;
		inline size_error(const char* a_what) noexcept : exception(a_what) {}
		virtual ~size_error() noexcept = default;

		size_error& operator=(const size_error&) noexcept = default;
	};


	class hash_error : public exception
	{
	public:
		inline hash_error() noexcept : hash_error("encountered an error during hash generation") {}
		inline hash_error(const hash_error&) noexcept = default;
		inline hash_error(const char* a_what) noexcept : exception(a_what) {}
		virtual ~hash_error() noexcept = default;

		hash_error& operator=(const hash_error&) noexcept = default;
	};


	// non-ascii characters have negative values, and beth doesn't cast them to their unsigned
	// counterparts while remapping them, so you get something like "remaptable[-17]" which
	// is possibly the most bethesda thing they could do
	class hash_non_ascii : public hash_error
	{
	public:
		inline hash_non_ascii() noexcept : hash_non_ascii("encountered a non ascii character during hash generation") {}
		inline hash_non_ascii(const hash_non_ascii&) noexcept = default;
		inline hash_non_ascii(const char* a_what) noexcept : hash_error(a_what) {}
		virtual ~hash_non_ascii() noexcept = default;

		hash_non_ascii& operator=(const hash_non_ascii&) noexcept = default;
	};


	class hash_empty : public hash_error
	{
	public:
		inline hash_empty() noexcept : hash_empty("the given path was empty") {}
		inline hash_empty(const hash_empty&) noexcept = default;
		inline hash_empty(const char* a_what) noexcept : hash_error(a_what) {}
		virtual ~hash_empty() noexcept = default;

		hash_empty& operator=(const hash_empty&) noexcept = default;
	};


	class io_error : public exception
	{
	public:
		inline io_error() noexcept : io_error("failure while performing i/o with archive") {}
		inline io_error(const io_error&) noexcept = default;
		inline io_error(const char* a_what) noexcept : exception(a_what) {}
		virtual ~io_error() noexcept = default;
	};


	class input_error : public io_error
	{
	public:
		inline input_error() noexcept : input_error("failure while performing input") {}
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


	class empty_file : public input_error
	{
	public:
		inline empty_file() noexcept : empty_file("file was empty") {}
		inline empty_file(const empty_file&) noexcept = default;
		inline empty_file(const char* a_what) noexcept : input_error(a_what) {}
		virtual ~empty_file() noexcept = default;
	};


	class output_error : public io_error
	{
	public:
		inline output_error() noexcept : output_error("failure while performing output") {}
		inline output_error(const output_error&) noexcept = default;
		inline output_error(const char* a_what) noexcept : io_error(a_what) {}
		virtual ~output_error() noexcept = default;
	};


	namespace detail
	{
		static constexpr auto max_int32 = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max());
		static constexpr auto max_uint32 = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());


		template <class T, typename std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
		[[nodiscard]] constexpr T rotl(T a_val, int a_pos) noexcept;
		template <class T, typename std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
		[[nodiscard]] constexpr T rotr(T a_val, int a_pos) noexcept;


		template <class T, typename std::enable_if_t<std::is_unsigned_v<T>, int>>
		[[nodiscard]] constexpr T rotl(T a_val, int a_pos) noexcept
		{
			constexpr auto N = std::numeric_limits<T>::digits;
			auto rot = a_pos % N;
			if (rot == 0) {
				return a_val;
			} else if (rot < 0) {
				return rotr(a_val, -rot);
			} else {
				return (a_val << rot) | (a_val >> (N - rot));
			}
		}


		template <class T, typename std::enable_if_t<std::is_unsigned_v<T>, int>>
		[[nodiscard]] constexpr T rotr(T a_val, int a_pos) noexcept
		{
			constexpr auto N = std::numeric_limits<T>::digits;
			auto rot = a_pos % N;
			if (rot == 0) {
				return a_val;
			} else if (rot < 0) {
				return rotl(a_val, -rot);
			} else {
				return (a_val >> rot) | (a_val << (N - rot));
			}
		}


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


		class path_t
		{
		public:
			using value_type = std::string;
			using iterator = typename value_type::iterator;
			using const_iterator = typename value_type::const_iterator;
			using reverse_iterator = typename value_type::reverse_iterator;
			using const_reverse_iterator = typename value_type::const_reverse_iterator;

			inline path_t() noexcept :
				_impl()
			{}

			inline path_t(const path_t& a_rhs) :
				_impl(a_rhs._impl)
			{}

			inline path_t(path_t&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			inline path_t(std::string_view a_path) :
				_impl()
			{
				normalize(a_path);
			}

			inline path_t& operator=(const path_t& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					_impl = a_rhs._impl;
				}
				return *this;
			}

			inline path_t& operator=(path_t&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_impl = std::move(a_rhs._impl);
				}
				return *this;
			}

			inline path_t& operator=(std::string_view a_path)
			{
				normalize(a_path);
				return *this;
			}

			[[nodiscard]] inline iterator begin() noexcept { return _impl.begin(); }
			[[nodiscard]] inline const_iterator begin() const noexcept { return _impl.begin(); }
			[[nodiscard]] inline const_iterator cbegin() const noexcept { return _impl.cbegin(); }

			[[nodiscard]] inline iterator end() noexcept { return _impl.end(); }
			[[nodiscard]] inline const_iterator end() const noexcept { return _impl.end(); }
			[[nodiscard]] inline const_iterator cend() const noexcept { return _impl.cend(); }

			[[nodiscard]] inline reverse_iterator rbegin() noexcept { return _impl.rbegin(); }
			[[nodiscard]] inline const_reverse_iterator rbegin() const noexcept { return _impl.rbegin(); }
			[[nodiscard]] inline const_reverse_iterator crbegin() const noexcept { return _impl.crbegin(); }

			[[nodiscard]] inline reverse_iterator rend() noexcept { return _impl.rend(); }
			[[nodiscard]] inline const_reverse_iterator rend() const noexcept { return _impl.rend(); }
			[[nodiscard]] inline const_reverse_iterator crend() const noexcept { return _impl.crend(); }

			[[nodiscard]] inline bool empty() const noexcept { return _impl.empty(); }

			[[nodiscard]] inline const char* c_str() const noexcept { return _impl.c_str(); }
			[[nodiscard]] inline std::string string() const { return _impl; }
			[[nodiscard]] inline std::string_view string_view() const noexcept { return static_cast<std::string_view>(_impl); }

		private:
			inline void normalize(std::string_view a_path)
			{
				std::filesystem::path p(a_path);
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

			istream_t() = delete;
			istream_t(const istream_t&) = delete;

			inline istream_t(istream_t&& a_rhs) noexcept :
				_stream(std::move(a_rhs._stream)),
				_beg(std::move(a_rhs._beg))
			{}

			istream_t(const std::unique_ptr<stream_type>&) = delete;

			inline istream_t(std::unique_ptr<stream_type>&& a_stream) :
				_stream(std::move(a_stream)),
				_beg(0)
			{
				if (!_stream || !*_stream) {
					throw input_error();
				} else {
					_beg = _stream->tellg();
					_stream->exceptions(std::ios_base::badbit | std::ios_base::failbit);
				}
			}

			istream_t& operator=(const istream_t&) = delete;

			inline istream_t& operator=(istream_t&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_stream = std::move(a_rhs._stream);
					_beg = std::move(a_rhs._beg);
				}
				return *this;
			}

			[[nodiscard]] inline bool operator!() const { return !*_stream; }
			[[nodiscard]] explicit inline operator bool() const { return static_cast<bool>(*_stream); }

			inline istream_t& get(char_type& a_ch) { _stream->get(a_ch); return *this; }

			inline istream_t& read(char_type* a_str, std::streamsize a_count) { _stream->read(a_str, a_count); return *this; }

			[[nodiscard]] inline pos_type tell() { return _stream->tellg(); }

			inline istream_t& seek_abs(pos_type a_pos) { _stream->seekg(a_pos); return *this; }	// seek absolute position
			inline istream_t& seek_beg() { _stream->seekg(_beg); return *this; }	// seek to beginning
			inline istream_t& seek_beg(pos_type a_pos) { _stream->seekg(_beg + a_pos); return *this; }	// seek from beginning
			inline istream_t& seek_rel(off_type a_off) { _stream->seekg(a_off, std::ios_base::cur); return *this; }	// seek relative to current position

		private:
			std::unique_ptr<stream_type> _stream;
			pos_type _beg;
		};


		// a wrapper that allows files to extract data outside of the initial read in a thread safe manner
		class mtistream_t
		{
		public:
			class sentry
			{
			public:
				sentry() = delete;
				sentry(const sentry&) = delete;
				sentry(sentry&&) = delete;

				sentry(mtistream_t& a_stream) :
					_impl(a_stream),
					_locker(a_stream._lock)
				{}

				~sentry() = default;

				sentry& operator=(const sentry&) = delete;
				sentry& operator=(sentry&&) = delete;

				[[nodiscard]] constexpr istream_t* get() noexcept { return std::addressof(_impl._stream); }
				[[nodiscard]] constexpr const istream_t* get() const noexcept { return std::addressof(_impl._stream); }

				[[nodiscard]] constexpr istream_t& operator*() noexcept { return *get(); }
				[[nodiscard]] constexpr const istream_t& operator*() const noexcept { return *get(); }

				[[nodiscard]] constexpr istream_t* operator->() noexcept { return get(); }
				[[nodiscard]] constexpr const istream_t* operator->() const noexcept { return get(); }

			private:
				mtistream_t& _impl;
				std::unique_lock<std::mutex> _locker;
			};

			mtistream_t() = delete;
			mtistream_t(const mtistream_t&) = delete;
			mtistream_t(mtistream_t&&) = delete;

			mtistream_t(const istream_t&) = delete;

			inline mtistream_t(istream_t&& a_stream) :
				_stream(std::move(a_stream)),
				_lock()
			{}

			mtistream_t& operator=(const mtistream_t&) = delete;
			mtistream_t& operator=(mtistream_t&&) = delete;

		private:
			friend class sentry;

			istream_t _stream;
			mutable std::mutex _lock;
		};


		class ostream_t
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
				_beg(a_stream.tellp())
			{
				if (!_stream) {
					throw output_error();
				}
			}

			ostream_t& operator=(const ostream_t&) = delete;
			ostream_t& operator=(ostream_t&&) = delete;

			[[nodiscard]] constexpr reference operator*() noexcept { return _stream; }
			[[nodiscard]] constexpr const_reference operator*() const noexcept { return _stream; }

			[[nodiscard]] constexpr pointer operator->() noexcept { return std::addressof(_stream); }
			[[nodiscard]] constexpr const_pointer operator->() const noexcept { return std::addressof(_stream); }

			[[nodiscard]] inline bool operator!() const { return !_stream; }
			[[nodiscard]] explicit inline operator bool() const { return static_cast<bool>(_stream); }

			inline ostream_t& write(const char_type* a_str, std::streamsize a_count) { if (!_stream.write(a_str, a_count)) throw output_error(); return *this; }

			[[nodiscard]] inline pos_type tell() { return _stream.tellp(); }
			[[nodiscard]] inline pos_type tell_rel() { return tell() - _beg; }

			inline ostream_t& seek_abs(pos_type a_pos) { if (!_stream.seekp(a_pos)) throw output_error(); return *this; }	// seek absolute position
			inline ostream_t& seek_beg() { if (!_stream.seekp(_beg)) throw output_error(); return *this; }	// seek to beginning
			inline ostream_t& seek_beg(pos_type a_pos) { if (!_stream.seekp(_beg + a_pos)) throw output_error(); return *this; }	// seek from beginning
			inline ostream_t& seek_rel(off_type a_off) { if (!_stream.seekp(a_off, std::ios_base::cur)) throw output_error(); return *this; }	// seek relative to current position

		private:
			stream_type& _stream;
			pos_type _beg;
		};
	}


	namespace tes3	// The Elder Scrolls III: Morrowind
	{
		using archive_version = std::size_t;
		static constexpr archive_version v256 = 256;


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

				[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

				[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t> (_block.fileCount); }
				[[nodiscard]] constexpr std::size_t hash_offset() const noexcept { return static_cast<std::size_t> (_block.hashOffset); }
				[[nodiscard]] constexpr archive_version version() const noexcept { return static_cast<archive_version>(_block.version); }

				constexpr void set_file_count(std::size_t a_count)
				{
					if (a_count > max_int32) {
						throw size_error();
					} else {
						_block.fileCount = static_cast<std::uint32_t>(a_count);
					}
				}

				constexpr void set_hash_offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.hashOffset = static_cast<std::uint32_t>(a_offset);
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
				struct block_t
				{
					constexpr block_t() noexcept :
						version(v256),
						hashOffset(0),
						fileCount(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						version(a_rhs.version),
						hashOffset(a_rhs.hashOffset),
						fileCount(a_rhs.fileCount)
					{}

					constexpr block_t(block_t&& a_rhs) noexcept :
						version(std::move(a_rhs.version)),
						hashOffset(std::move(a_rhs.hashOffset)),
						fileCount(std::move(a_rhs.fileCount))
					{}

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							version = a_rhs.version;
							hashOffset = a_rhs.hashOffset;
							fileCount = a_rhs.fileCount;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							version = std::move(a_rhs.version);
							hashOffset = std::move(a_rhs.hashOffset);
							fileCount = std::move(a_rhs.fileCount);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
					}

					inline void write(ostream_t& a_output) const
					{
						a_output.write(reinterpret_cast<const char*>(this), sizeof(block_t));
					}

					std::uint32_t version;
					std::uint32_t hashOffset;
					std::uint32_t fileCount;
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
					if (this != std::addressof(a_rhs)) {
						_impl = a_rhs._impl;
					}
					return *this;
				}

				constexpr hash_t& operator=(hash_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_impl = std::move(a_rhs._impl);
					}
					return *this;
				}

				[[nodiscard]] friend constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_lhs.numeric() == a_rhs.numeric(); }
				[[nodiscard]] friend constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

				[[nodiscard]] friend constexpr bool operator<(const hash_t& a_lhs, const hash_t& a_rhs) noexcept
				{
					if (a_lhs.low() != a_rhs.low()) {
						return a_lhs.low() < a_rhs.low();
					} else {
						return a_lhs.high() < a_rhs.high();
					}
				}

				[[nodiscard]] friend constexpr bool operator>(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return a_rhs < a_lhs; }
				[[nodiscard]] friend constexpr bool operator<=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
				[[nodiscard]] friend constexpr bool operator>=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

				[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

				[[nodiscard]] constexpr std::uint32_t high() const noexcept { return _impl.block.hi; }
				[[nodiscard]] constexpr std::uint32_t low() const noexcept { return _impl.block.lo; }
				[[nodiscard]] constexpr std::uint64_t numeric() const noexcept { return _impl.numeric; }

				inline void read(istream_t& a_input)
				{
					_impl.read(a_input);
				}

				inline void write(ostream_t& a_output) const
				{
					_impl.write(a_output);
				}

			protected:
				friend class file_hasher;

				struct block_t
				{
					std::uint32_t lo;
					std::uint32_t hi;
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
						if (this != std::addressof(a_rhs)) {
							numeric = a_rhs.numeric;
						}
						return *this;
					}

					constexpr NumericBlock& operator=(NumericBlock&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							numeric = std::move(a_rhs.numeric);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(NumericBlock));
					}

					inline void write(ostream_t& a_output) const
					{
						a_output.write(reinterpret_cast<const char*>(this), sizeof(NumericBlock));
					}

					std::uint64_t numeric;
					block_t block;
				};

				NumericBlock _impl;
			};
			using hash_ref = std::reference_wrapper<hash_t>;


			class file_hasher
			{
			public:
				file_hasher() noexcept = default;
				file_hasher(const file_hasher&) noexcept = default;
				file_hasher(file_hasher&&) noexcept = default;

				~file_hasher() noexcept = default;

				file_hasher& operator=(const file_hasher&) noexcept = default;
				file_hasher& operator=(file_hasher&&) noexcept = default;

				[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
				{
					path_t path(a_path);
					auto view = path.string_view();
					verify(view);
					return hash(view);
				}

				[[nodiscard]] inline hash_t operator()(const path_t& a_path) const
				{
					auto view = a_path.string_view();
					verify(view);
					return hash(view);
				}

			private:
				[[nodiscard]] constexpr hash_t hash(std::string_view a_fullPath) const
				{
					hash_t hash;
					auto& block = hash.block_ref();

					std::size_t midPoint = a_fullPath.size() >> 1;
					std::size_t i = 0;
					while (i < midPoint) {
						// rotate between first 4 bytes
						block.lo ^= static_cast<std::uint32_t>(a_fullPath[i]) << ((i % 4) * 8);
						++i;
					}

					while (i < a_fullPath.length()) {
						// rotate between first 4 bytes
						auto rot = static_cast<std::uint32_t>(a_fullPath[i]) << (((i - midPoint) % 4) * 8);
						block.hi = rotr(block.hi ^ rot, static_cast<int>(rot));
						++i;
					}

					return hash;
				}

				inline void verify(std::string_view a_path) const
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


			class file_t
			{
			public:
				inline file_t() noexcept :
					_hash(),
					_block(),
					_name(),
					_data(std::nullopt),
					_mtinput(nullptr)
				{}

				inline file_t(const file_t& a_rhs) :
					_hash(a_rhs._hash),
					_block(a_rhs._block),
					_name(a_rhs._name),
					_data(a_rhs._data),
					_mtinput(a_rhs._mtinput)
				{}

				inline file_t(file_t&& a_rhs) noexcept :
					_hash(std::move(a_rhs._hash)),
					_block(std::move(a_rhs._block)),
					_name(std::move(a_rhs._name)),
					_data(std::move(a_rhs._data)),
					_mtinput(std::move(a_rhs._mtinput))
				{}

				inline file_t(const std::shared_ptr<mtistream_t>& a_mtinput) noexcept :
					_hash(),
					_block(),
					_name(),
					_data(std::nullopt),
					_mtinput(a_mtinput)
				{}

				inline file_t(std::shared_ptr<mtistream_t>&& a_mtinput) noexcept :
					_hash(),
					_block(),
					_name(),
					_data(std::nullopt),
					_mtinput(std::move(a_mtinput))
				{}

				inline file_t(std::string_view a_relativePath) :
					_hash(),
					_block(),
					_name(),
					_data(std::nullopt),
					_mtinput(nullptr)
				{
					path_t path(a_relativePath);
					_hash = file_hasher()(path);
					_name = path.string();
				}

				inline file_t& operator=(const file_t& a_rhs)
				{
					if (this != std::addressof(a_rhs)) {
						_hash = a_rhs._hash;
						_block = a_rhs._block;
						_name = a_rhs._name;
						_data = a_rhs._data;
						_mtinput = a_rhs._mtinput;
					}
					return *this;
				}

				inline file_t& operator=(file_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
						_hash = std::move(a_rhs._hash);
						_block = std::move(a_rhs._block);
						_name = std::move(a_rhs._name);
						_data = std::move(a_rhs._data);
						_mtinput = std::move(a_rhs._mtinput);
					}
					return *this;
				}

				[[nodiscard]] friend constexpr bool operator==(const file_t& a_lhs, const file_t& a_rhs) { return a_lhs.hash_ref() == a_rhs.hash_ref(); }
				[[nodiscard]] friend constexpr bool operator!=(const file_t& a_lhs, const file_t& a_rhs) { return !(a_lhs == a_rhs); }

				[[nodiscard]] friend constexpr bool operator<(const file_t& a_lhs, const file_t& a_rhs) noexcept { return a_lhs.hash_ref() < a_rhs.hash_ref(); }
				[[nodiscard]] friend constexpr bool operator>(const file_t& a_lhs, const file_t& a_rhs) noexcept { return a_rhs < a_lhs; }
				[[nodiscard]] friend constexpr bool operator<=(const file_t& a_lhs, const file_t& a_rhs) noexcept { return !(a_lhs > a_rhs); }
				[[nodiscard]] friend constexpr bool operator>=(const file_t& a_lhs, const file_t& a_rhs) noexcept { return !(a_lhs < a_rhs); }

				[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

				[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

				[[nodiscard]] inline bool empty() const noexcept { return !_data && !_mtinput; }

				[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
				[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
				[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				[[nodiscard]] inline std::size_t name_size() const noexcept { return _name.size() + 1; }

				[[nodiscard]] constexpr std::size_t offset() const noexcept { return static_cast<std::size_t>(_block.offset); }

				[[nodiscard]] constexpr std::size_t size() const noexcept { return static_cast<std::size_t>(_block.size); }

				[[nodiscard]] inline std::string str() const { return _name; }
				[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

				[[nodiscard]] inline std::vector<char> get_data() { return _data ? *_data : load_data(); }

				inline void set_data(const char* a_data, std::size_t a_size)
				{
					if (a_size > max_int32) {
						throw size_error();
					} else {
						_block.size = static_cast<std::uint32_t>(a_size);
						_data.emplace(a_data, a_data + a_size);
					}
				}

				inline void set_data(std::istream& a_input)
				{
					_data.emplace();
					while (a_input) {
						_data->push_back(static_cast<char>(a_input.get()));
					}
					if (!_data->empty()) {
						_data->pop_back();	// discard eof
					}

					if (_data->size() == 0) {
						_data.reset();
						throw size_error();
					} else if (_data->size() > max_int32) {
						_data.reset();
						throw size_error();
					} else {
						_block.size = static_cast<std::uint32_t>(_data->size());
					}
				}

				constexpr void set_offset(std::size_t a_offset)
				{
					if (a_offset > max_int32) {
						throw size_error();
					} else {
						_block.offset = static_cast<std::uint32_t>(a_offset);
					}
				}

				inline void read(istream_t& a_input)
				{
					_block.read(a_input);
				}

				inline void read_hash(istream_t& a_input)
				{
					_hash.read(a_input);
				}

				inline void read_name(istream_t& a_input)
				{
					char ch;
					do {
						a_input.get(ch);
						_name.push_back(ch);
					} while (ch != '\0' && a_input);
					_name.pop_back();

					if (!a_input) {
						throw input_error();
					}
				}

				inline void extract(std::ofstream& a_file)
				{
					auto ssize = static_cast<std::streamsize>(size());
					if (_data) {
						a_file.write(_data->data(), ssize);
					} else {
						auto data = load_data();
						a_file.write(data.data(), ssize);
					}

					if (!a_file) {
						throw output_error();
					}
				}

				inline void write(ostream_t& a_output) const
				{
					_block.write(a_output);
				}

				inline void write_data(ostream_t& a_output)
				{
					auto ssize = static_cast<std::streamsize>(size());
					if (_data) {
						a_output.write(_data->data(), ssize);
					} else {
						auto data = load_data();
						a_output.write(data.data(), ssize);
					}
				}

				inline void write_hash(ostream_t& a_output) const
				{
					_hash.write(a_output);
				}

				inline void write_name(ostream_t& a_output) const
				{
					a_output.write(_name.data(), static_cast<std::streamsize>(name_size()));
				}

			private:
				using sentry_t = typename mtistream_t::sentry;

				struct block_t
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

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
					}

					inline void write(ostream_t& a_output) const
					{
						a_output.write(reinterpret_cast<const char*>(this), sizeof(block_t));
					}

					std::uint32_t size;
					std::uint32_t offset;
				};

				inline std::vector<char> load_data()
				{
					if (!_mtinput) {
						throw input_error();
					}
					sentry_t sentry(*_mtinput);

					auto pos = sentry->tell();
					sentry->seek_rel(static_cast<std::streamoff>(offset()));

					std::vector<char> data(size(), '\0');
					sentry->read(data.data(), static_cast<std::streamsize>(size()));

					sentry->seek_abs(pos);
					return data;
				}

				hash_t _hash;
				block_t _block;
				std::string _name;
				std::optional<std::vector<char>> _data;
				std::shared_ptr<mtistream_t> _mtinput;
			};
			using file_ptr = std::shared_ptr<file_t>;
		}


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

			[[nodiscard]] inline std::uint32_t high_hash() const noexcept { return _impl.get().high(); }
			[[nodiscard]] inline std::uint32_t low_hash() const noexcept { return _impl.get().low(); }
			[[nodiscard]] inline std::uint64_t numeric_hash() const noexcept { return _impl.get().numeric(); }

		protected:
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
			inline file() noexcept :
				_impl(nullptr)
			{}

			inline file(const file& a_rhs) noexcept :
				_impl(a_rhs._impl)
			{}

			inline file(file&& a_rhs) noexcept :
				_impl(std::move(a_rhs._impl))
			{}

			inline file(std::string_view a_relativePath, std::filesystem::path a_filePath) :
				_impl(nullptr)
			{
				_impl.reset(new detail::file_t(a_relativePath));
				open_and_pack(a_filePath);
			}

			inline file(std::string_view a_relativePath, std::istream& a_stream) :
				_impl(new detail::file_t(a_relativePath))
			{
				_impl->set_data(a_stream);
			}

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

			[[nodiscard]] friend inline bool operator==(const file& a_lhs, const file& a_rhs)
			{
				if (!a_lhs && !a_rhs) {	// neither have value
					return true;
				} else if (!a_lhs != !a_rhs) {	// one has value
					return false;
				} else {	// both have value
					return a_lhs._impl->hash_ref() == a_rhs._impl->hash_ref();
				}
			}

			[[nodiscard]] friend inline bool operator!=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] friend inline bool operator<(const file& a_lhs, const file& a_rhs) noexcept
			{
				if (!a_lhs && !a_rhs) {	// neither have value
					return true;
				} else if (!a_lhs != !a_rhs) {	// one has value
					return a_lhs.exists();
				} else {	// both have value
					return a_lhs._impl->hash_ref() < a_rhs._impl->hash_ref();
				}
			}

			[[nodiscard]] friend inline bool operator>(const file& a_lhs, const file& a_rhs) noexcept { return a_rhs < a_lhs; }
			[[nodiscard]] friend inline bool operator<=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs > a_rhs); }
			[[nodiscard]] friend inline bool operator>=(const file& a_lhs, const file& a_rhs) noexcept { return !(a_lhs < a_rhs); }

			[[nodiscard]] inline explicit operator bool() const noexcept { return exists(); }
			[[nodiscard]] inline bool exists() const noexcept { return static_cast<bool>(_impl); }

			[[nodiscard]] inline const char* c_str() const noexcept { assert(exists()); return _impl->c_str(); }

			[[nodiscard]] inline bool empty() const { assert(exists()); return _impl->empty(); }

			[[nodiscard]] inline std::vector<char> extract() const { assert(exists()); return _impl->get_data(); }

			inline void extract_to(const std::filesystem::path& a_root) const
			{
				assert(exists());
				if (!std::filesystem::exists(a_root)) {
					throw output_error();
				}

				auto path = a_root;
				path /= string();
				std::filesystem::create_directories(path.parent_path());
				std::ofstream file(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
				if (!file.is_open()) {
					throw output_error();
				}

				_impl->extract(file);
			}

			[[nodiscard]] inline hash hash_value() const noexcept { assert(exists()); return hash(_impl->hash_ref()); }

			inline void pack(const char* a_data, std::size_t a_size) { assert(exists()); _impl->set_data(a_data, a_size); }
			inline void pack(std::filesystem::path a_path) { assert(exists()); open_and_pack(a_path); }
			inline void pack(std::istream& a_stream) { assert(exists()); _impl->set_data(a_stream); }

			[[nodiscard]] inline std::size_t size() const noexcept { assert(exists()); return _impl->size(); }
			[[nodiscard]] inline const std::string& string() const noexcept { assert(exists()); return _impl->str_ref(); }

		protected:
			friend class archive;
			friend class file_iterator;

			using value_type = detail::file_ptr;

			explicit inline file(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline file(value_type&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

			[[nodiscard]] constexpr value_type& file_ptr() noexcept { return _impl; }
			[[nodiscard]] constexpr const value_type& file_ptr() const noexcept { return _impl; }

			[[nodiscard]] inline value_type&& steal_file() noexcept { return std::move(_impl); }

		private:
			inline void open_and_pack(const std::filesystem::path& a_path)
			{
				std::ifstream input(a_path, std::ios_base::in | std::ios_base::binary);
				input.exceptions(std::ios_base::badbit);
				_impl->set_data(input);
			}

			value_type _impl;
		};


		template <class... Args, typename std::enable_if_t<std::is_constructible_v<file, Args...>, int> = 0>
		file make_file(Args... a_args) noexcept(std::is_nothrow_constructible_v<file, Args...>)
		{
			return file(std::forward<Args>(a_args)...);
		}


		class file_iterator
		{
		public:
			using value_type = file;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::input_iterator_tag;

			inline file_iterator() noexcept :
				_files(nullptr),
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

			[[nodiscard]] friend inline bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return a_lhs._files == a_rhs._files && a_lhs._pos == a_rhs._pos; }
			[[nodiscard]] friend inline bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline reference operator*() { return fetch(); }
			[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

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
			[[nodiscard]] inline file_iterator operator++([[maybe_unused]] int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			friend inline void swap(file_iterator& a_lhs, file_iterator& a_rhs)
			{
				auto tmp = std::move(a_lhs);
				a_lhs = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class archive;

			template <class InputIt>
			explicit inline file_iterator(InputIt a_first, InputIt a_last) :
				_files(nullptr),
				_pos(NPOS)
			{
				if (a_first != a_last) {
					_files.reset(new typename decltype(_files)::element_type());
					_pos = 0;
					do {
						_files->push_back(value_type(static_cast<const detail::file_ptr&>(*a_first)));
						++a_first;
					} while (a_first != a_last);
				}
			}

		private:
			inline reference fetch() { return (*_files)[_pos]; }

			static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

			std::shared_ptr<std::vector<value_type>> _files;
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

			inline archive(const archive& a_rhs) :
				_files(a_rhs._files),
				_header(a_rhs._header)
			{}

			inline archive(archive&& a_rhs) noexcept :
				_files(std::move(a_rhs._files)),
				_header(std::move(a_rhs._header))
			{}

			inline archive(const std::filesystem::path& a_path) :
				_files(),
				_header()
			{
				read(a_path);
			}

			inline archive(std::unique_ptr<std::istream>&& a_stream) :
				_files(),
				_header()
			{
				read(std::move(a_stream));
			}

			inline archive& operator=(const archive& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					_files = a_rhs._files;
					_header = a_rhs._header;
				}
				return *this;
			}

			inline archive& operator=(archive&& a_rhs) noexcept
			{
				if (this != std::addressof(a_rhs)) {
					_files = std::move(a_rhs._files);
					_header = std::move(a_rhs._header);
				}
				return *this;
			}

			friend inline archive& operator>>(archive& a_archive, const std::filesystem::path& a_path)
			{
				a_archive.read(a_path);
				return a_archive;
			}

			friend inline archive& operator>>(archive& a_archive, std::filesystem::path&& a_path)
			{
				a_archive.read(std::move(a_path));
				return a_archive;
			}

			friend inline archive& operator>>(archive& a_archive, std::ifstream&& a_stream)
			{
				a_archive.read(std::move(a_stream));
				return a_archive;
			}

			friend inline archive& operator>>(archive& a_archive, const file& a_file)
			{
				a_archive.insert(a_file);
				return a_archive;
			}

			friend inline archive& operator>>(archive& a_archive, file&& a_file)
			{
				a_archive.insert(std::move(a_file));
				return a_archive;
			}

			friend inline archive& operator>>(archive& a_archive, std::unique_ptr<std::istream>&& a_stream)
			{
				a_archive.read(std::move(a_stream));
				return a_archive;
			}

			friend inline archive& operator<<(archive& a_archive, const std::filesystem::path& a_path)
			{
				a_archive.write(a_path);
				return a_archive;
			}

			friend inline archive& operator<<(archive& a_archive, std::filesystem::path&& a_path)
			{
				a_archive.write(std::move(a_path));
				return a_archive;
			}

			friend inline archive& operator<<(archive& a_archive, std::ostream& a_stream)
			{
				a_archive.write(a_stream);
				return a_archive;
			}

			[[nodiscard]] inline file front() const { assert(!empty()); return file(_files.front()); }
			[[nodiscard]] inline file back() const { assert(!empty()); return file(_files.back()); }

			[[nodiscard]] inline iterator begin() const { return iterator(_files.begin(), _files.end()); }
			[[nodiscard]] inline iterator end() const noexcept { return iterator(); }

			[[nodiscard]] constexpr std::size_t size() const noexcept { return file_count(); }
			[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
			inline void clear() noexcept { _files.clear(); _header.clear(); }

			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			[[nodiscard]] constexpr archive_version version() const noexcept { return _header.version(); }

			inline void read(const std::filesystem::path& a_path)
			{
				auto file = std::make_unique<std::ifstream>(a_path, std::ios_base::in | std::ios_base::binary);
				if (!file->is_open()) {
					throw input_error();
				} else {
					read(std::move(file));
				}
			}

			inline void read(std::ifstream&& a_input)
			{
				read(std::make_unique<std::ifstream>(std::move(a_input)));
			}

			inline void read(std::unique_ptr<std::istream>&& a_input)
			{
				auto mtinput = std::make_shared<detail::mtistream_t>(std::move(a_input));
				detail::mtistream_t::sentry sentry(*mtinput);
				auto& input = *sentry;

				clear();

				_header.read(input);
				switch (version()) {
				case v256:
					break;
				default:
					throw version_error();
				}

				read_initial(mtinput, input);
				read_filenames(input);
				read_hashes(input);
				read_data(input);	// important this happens last

				sort();

				assert(sanity_check());
			}

			inline void extract(const std::filesystem::path& a_path)
			{
				if (!std::filesystem::exists(a_path)) {
					throw output_error();
				}

				std::filesystem::path filePath;
				std::ofstream output;
				for (auto& file : _files) {
					output.close();
					filePath = a_path / file->str_ref();
					std::filesystem::create_directories(filePath.parent_path());
					output.open(filePath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
					if (!output.is_open()) {
						throw output_error();
					}
					file->extract(output);
				}
			}

			inline void write(const std::filesystem::path& a_path)
			{
				std::ofstream file(a_path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
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
				for (auto& file : _files) {
					file->write(output);
				}

				std::uint32_t offset = 0;
				for (auto& file : _files) {
					output.write(reinterpret_cast<const char*>(std::addressof(offset)), sizeof(offset));
					offset += static_cast<std::uint32_t>(file->name_size());
				}

				for (auto& file : _files) {
					file->write_name(output);
				}

				for (auto& file : _files) {
					file->write_hash(output);
				}

				for (auto& file : _filesByName) {
					file->write_data(output);
				}
			}

			inline void insert(const file& a_file)
			{
				if (!can_insert(a_file.file_ptr())) {
					throw size_error();
				} else if (!a_file || a_file.empty()) {
					throw empty_file();
				} else if (!contains(a_file)) {
					reserve(size() + 1);
					push_back(a_file.file_ptr());
					sort();
					update_size();
				}
			}

			inline void insert(file&& a_file)
			{
				if (!can_insert(a_file.file_ptr())) {
					throw size_error();
				} else if (!a_file || a_file.empty()) {
					throw empty_file();
				} else if (!contains(a_file)) {
					reserve(size() + 1);
					push_back(a_file.steal_file());
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
						throw empty_file();
					} else if (!contains(f)) {
						toInsert.push_back(f.file_ptr());
					}
					++a_first;
				}

				std::sort(toInsert.begin(), toInsert.end(), file_sorter());
				auto newEnd = std::unique(toInsert.begin(), toInsert.end(), [](const value_t& a_lhs, const value_t& a_rhs) -> bool
				{
					return a_lhs->hash_ref() == a_rhs->hash_ref();
				});
				toInsert.erase(newEnd, toInsert.end());

				if (!can_insert(toInsert)) {
					throw size_error();
				}

				reserve(size() + toInsert.size());
				for (auto& elem : toInsert) {
					push_back(elem);
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

				auto hash = a_file.file_ptr()->hash();
				auto it = binary_find(hash);
				if (it == _files.end()) {
					return false;
				}

				_files.erase(it);
				return true;
			}

			[[nodiscard]] inline file find(const std::filesystem::path& a_path)
			{
				auto path = a_path.string();
				return find(path);
			}

			[[nodiscard]] inline file find(const std::string& a_path)
			{
				auto view = static_cast<std::string_view>(a_path);
				return find(view);
			}

			[[nodiscard]] inline file find(const std::string_view& a_path)
			{
				auto hash = detail::file_hasher()(a_path);
				auto it = binary_find(hash);
				return it != _files.end() ? file(*it) : file();
			}

			[[nodiscard]] inline file find(const char* a_path)
			{
				std::string_view path(a_path);
				return find(path);
			}

			[[nodiscard]] inline bool contains(const file& a_file)
			{
				if (!a_file) {
					return false;
				} else {
					auto hash = a_file.file_ptr()->hash();
					auto it = binary_find(hash);
					return it != _files.end();
				}
			}

		private:
			using value_t = detail::file_ptr;
			using container_t = std::vector<value_t>;
			using iterator_t = typename container_t::iterator;

			class file_sorter
			{
			public:
				[[nodiscard]] inline bool operator()(const value_t& a_lhs, const value_t& a_rhs) const
				{
					return a_lhs->hash_ref() < a_rhs->hash_ref();
				}

				[[nodiscard]] inline bool operator()(const value_t& a_lhs, const detail::hash_t& a_rhs) const
				{
					return a_lhs->hash_ref() < a_rhs;
				}
			};

			class file_name_sorter
			{
			public:
				[[nodiscard]] inline bool operator()(const value_t& a_lhs, const value_t& a_rhs) const
				{
					return a_lhs->str_ref() < a_rhs->str_ref();
				}
			};

			inline iterator_t binary_find(const detail::hash_t& a_hash)
			{
				auto it = _files.begin();
				auto itEnd = _files.end();
				it = std::lower_bound(it, itEnd, a_hash, file_sorter());

				return it != _files.end() && (*it)->hash_ref() == a_hash ? it : itEnd;
			}

			[[nodiscard]] inline bool can_insert(const value_t& a_file)
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

			[[nodiscard]] inline bool can_insert(const container_t& a_files)
			{
				if (size() + a_files.size() > detail::max_int32) {
					return false;
				}

				container_t merge;
				std::merge(_files.begin(), _files.end(), a_files.begin(), a_files.end(), std::back_inserter(merge), file_sorter());
				if (!validate_hash_offsets(merge) || !validate_name_offsets(merge)) {
					return false;
				}

				std::sort(merge.begin(), merge.end(), file_name_sorter());
				if (!validate_data_offsets(merge)) {
					return false;
				}

				return true;
			}

			// std::function<std::size_t(const value_t&)>
			template <class Sorter, class UnaryFunctor>
			[[nodiscard]] inline bool validate_offsets(const container_t& a_files, const value_t& a_file, Sorter a_sorter, UnaryFunctor a_func)
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
			template <class UnaryFunctor>
			[[nodiscard]] inline bool validate_offsets(const container_t& a_files, UnaryFunctor a_func)
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

			[[nodiscard]] inline bool validate_data_offsets(const value_t& a_file)
			{
				return validate_offsets(_filesByName, a_file, file_name_sorter(), [](const value_t& a_val) -> std::size_t
				{
					return a_val->size();
				});
			}

			[[nodiscard]] inline bool validate_data_offsets(const container_t& a_files)
			{
				return validate_offsets(a_files, [](const value_t& a_val) -> std::size_t
				{
					return a_val->size();
				});
			}

			[[nodiscard]] inline bool validate_hash_offsets(const value_t& a_file)
			{
				auto offset = calc_hash_offset();
				offset += detail::file_t::block_size() + 4;
				offset += a_file->name_size();
				return offset <= detail::max_int32;
			}

			[[nodiscard]] inline bool validate_hash_offsets(const container_t& a_files)
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

			[[nodiscard]] inline bool validate_name_offsets(const value_t& a_file)
			{
				return validate_offsets(_files, a_file, file_sorter(), [](const value_t& a_val) -> std::size_t
				{
					return a_val->name_size();
				});
			}

			[[nodiscard]] inline bool validate_name_offsets(const container_t& a_files)
			{
				return validate_offsets(a_files, [](const value_t& a_val) -> std::size_t
				{
					return a_val->name_size();
				});
			}

			[[nodiscard]] inline std::size_t calc_file_size() const noexcept
			{
				return calc_file_size(_files);
			}

			[[nodiscard]] inline std::size_t calc_file_size(const container_t& a_files) const noexcept
			{
				return (detail::file_t::block_size() + 4) * a_files.size();
			}

			[[nodiscard]] inline std::size_t calc_hash_offset() const noexcept
			{
				return calc_hash_offset(_files);
			}

			[[nodiscard]] inline std::size_t calc_hash_offset(const container_t& a_files) const noexcept
			{
				return calc_file_size() + calc_names_size(a_files);
			}

			[[nodiscard]] inline std::size_t calc_names_size() const noexcept
			{
				return calc_names_size(_files);
			}

			[[nodiscard]] inline std::size_t calc_names_size(const container_t& a_files) const noexcept
			{
				std::size_t namesSize = 0;
				for (auto& file : a_files) {
					namesSize += file->name_size();
				}
				return namesSize;
			}

			[[nodiscard]] inline void prepare_for_write()
			{
				update_header();
				update_files();
			}

			inline void push_back(const value_t& a_val)
			{
				_files.push_back(a_val);
				_filesByName.push_back(a_val);
			}

			inline void push_back(value_t&& a_val)
			{
				_files.push_back(a_val);
				_filesByName.push_back(std::move(a_val));
			}

			inline void read_data(detail::istream_t& a_input)
			{
				std::streampos pos = static_cast<std::streamoff>(_header.hash_offset());
				pos += static_cast<std::streamoff>(detail::header_t::block_size());
				pos += static_cast<std::streamoff>(detail::hash_t::block_size() * file_count());
				a_input.seek_beg(pos);
			}

			inline void read_filenames(detail::istream_t& a_input)
			{
				using integer_t = std::uint32_t;
				std::vector<integer_t> offsets(file_count());
				for (auto& offset : offsets) {
					a_input.read(reinterpret_cast<char*>(std::addressof(offset)), sizeof(integer_t));
				}

				auto pos = a_input.tell();
				for (std::size_t i = 0; i < file_count(); ++i) {
					a_input.seek_abs(pos + static_cast<std::streamoff>(offsets[i]));
					_files[i]->read_name(a_input);
				}
			}

			inline void read_hashes(detail::istream_t& a_input)
			{
				std::streamoff pos = static_cast<std::streamoff>(_header.hash_offset());
				pos += static_cast<std::streamoff>(detail::header_t::block_size());
				a_input.seek_beg(pos);

				for (auto& file : _files) {
					file->read_hash(a_input);
				}
			}

			inline void read_initial(std::shared_ptr<detail::mtistream_t>& a_mtinput, detail::istream_t& a_input)
			{
				reserve(file_count());
				for (std::size_t i = 0; i < file_count(); ++i) {
					auto file = std::make_shared<detail::file_t>(a_mtinput);
					file->read(a_input);
					push_back(std::move(file));
				}
			}

			inline void reserve(std::size_t a_capacity)
			{
				_files.reserve(a_capacity);
				_filesByName.reserve(a_capacity);
			}

			inline void sort()
			{
				std::sort(_files.begin(), _files.end(), file_sorter());
				std::sort(_filesByName.begin(), _filesByName.end(), file_name_sorter());
			}

			inline bool sanity_check()
			{
				for (auto& file : _files) {
					auto hash = detail::file_hasher()(file->str_ref());
					if (hash != file->hash_ref()) {
						assert(false);
					}
				}

				return true;
			}

			[[nodiscard]] inline void update_files()
			{
				std::size_t offset = 0;
				for (auto& file : _filesByName) {
					file->set_offset(offset);
					offset += file->size();
				}
			}

			inline void update_header()
			{
				auto hashOffset = calc_hash_offset();
				_header.set_hash_offset(hashOffset);
				update_size();
			}

			inline void update_size()
			{
				_header.set_file_count(_files.size());
			}

			container_t _files;
			container_t _filesByName;
			detail::header_t _header;
		};
	}


	namespace tes4	// The Elder Scrolls IV: Oblivion
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

		using archive_version = std::size_t;
		static constexpr archive_version v103 = 103;
		static constexpr archive_version v104 = 104;
		static constexpr archive_version v105 = 105;


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

				[[nodiscard]] static constexpr std::size_t block_size() noexcept { return sizeof(block_t); }

				[[nodiscard]] constexpr std::size_t directory_count() const noexcept { return static_cast<std::size_t>(_block.directoryCount); }
				[[nodiscard]] constexpr std::size_t directory_names_length() const noexcept { return static_cast<std::size_t>(_block.directoryNamesLength); }
				[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t>(_block.fileCount); }
				[[nodiscard]] constexpr std::size_t file_names_length() const noexcept { return static_cast<std::size_t>(_block.fileNamesLength); }
				[[nodiscard]] constexpr archive_flag flags() const noexcept { return static_cast<archive_flag>(_block.flags); }
				[[nodiscard]] constexpr std::size_t header_size() const noexcept { return static_cast<std::size_t>(_block.headerSize); }
				[[nodiscard]] constexpr std::string_view tag() const { return std::string_view(_block.tag, std::extent_v<decltype(_block.tag)>); }
				[[nodiscard]] constexpr archive_type types() const noexcept { return static_cast<archive_type>(_block.archiveTypes); }
				[[nodiscard]] constexpr archive_version version() const noexcept { return static_cast<archive_version>(_block.version); }

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
					if (tag() != BSA) {
						throw input_error();
					}
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
						pad(0)
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(tag)>; ++i) {
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
						pad(0)
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(tag)>; ++i) {
							tag[i] = std::move(a_rhs.tag[i]);
						}
					}

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(tag)>; ++i) {
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
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(tag)>; ++i) {
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
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
						pad = 0;
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

				static constexpr auto BSA = std::string_view("BSA\0", std::extent_v<decltype(block_t::tag)>);

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
					if (this != std::addressof(a_rhs)) {
						_impl = a_rhs._impl;
					}
					return *this;
				}

				constexpr hash_t& operator=(hash_t&& a_rhs) noexcept
				{
					if (this != std::addressof(a_rhs)) {
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
						if (this != std::addressof(a_rhs)) {
							numeric = a_rhs.numeric;
						}
						return *this;
					}

					constexpr NumericBlock& operator=(NumericBlock&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
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
						a_input.get(ch);
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

				[[nodiscard]] static constexpr std::size_t block_size(archive_version a_version)
				{
					switch (a_version) {
					case v103:
					case v104:
						return sizeof(block86_t) + hash_t::block_size();
					case v105:
						return sizeof(block64_t) + hash_t::block_size();
					default:
						throw version_error();
					}
				}

				[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

				[[nodiscard]] constexpr std::size_t file_count() const
				{
					switch (_block.index()) {
					case oblivion:
						return static_cast<std::size_t>(std::get<oblivion>(_block).fileCount);
					case sse:
						return static_cast<std::size_t>(std::get<sse>(_block).fileCount);
					default:
						throw std::bad_variant_access();
					}
				}

				[[nodiscard]] constexpr std::uint64_t file_offset() const
				{
					switch (_block.index()) {
					case oblivion:
						return static_cast<std::uint64_t>(std::get<oblivion>(_block).fileOffset);
					case sse:
						return static_cast<std::uint64_t>(std::get<sse>(_block).fileOffset);
					default:
						throw std::bad_variant_access();
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
					case v103:
					case v104:
						_block.emplace<oblivion>();
						std::get<oblivion>(_block).read(a_input, a_header);
						break;
					case v105:
						_block.emplace<sse>();
						std::get<sse>(_block).read(a_input, a_header);
						break;
					default:
						throw version_error();
					}

					if (a_header.directory_strings() || file_count() > 0) {
						read_extra(a_input, a_header);
					}
				}

			private:
				enum : std::size_t { oblivion, sse };

				struct block86_t	// BSDirectoryEntry x86
				{
					constexpr block86_t() noexcept :
						fileCount(0),
						fileOffset(0)
					{}

					constexpr block86_t(const block86_t& a_rhs) noexcept :
						fileCount(a_rhs.fileCount),
						fileOffset(a_rhs.fileOffset)
					{}

					constexpr block86_t(block86_t&& a_rhs) noexcept :
						fileCount(std::move(a_rhs.fileCount)),
						fileOffset(std::move(a_rhs.fileOffset))
					{}

					constexpr block86_t& operator=(const block86_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = a_rhs.fileCount;
							fileOffset = a_rhs.fileOffset;
						}
						return *this;
					}

					constexpr block86_t& operator=(block86_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = std::move(a_rhs.fileCount);
							fileOffset = std::move(a_rhs.fileOffset);
						}
						return *this;
					}

					inline void read(istream_t& a_input, [[maybe_unused]] const header_t& a_header)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block86_t));
					}

					inline void byte_swap()
					{
						swap_endian(fileCount);
						swap_endian(fileOffset);
					}

					std::uint32_t fileCount;
					std::uint32_t fileOffset;
				};

				struct block64_t	// BSDirectoryEntry x64
				{
					constexpr block64_t() noexcept :
						fileCount(0),
						pad(0),
						fileOffset(0)
					{}

					constexpr block64_t(const block64_t& a_rhs) noexcept :
						fileCount(a_rhs.fileCount),
						pad(0),
						fileOffset(a_rhs.fileOffset)
					{}

					constexpr block64_t(block64_t&& a_rhs) noexcept :
						fileCount(std::move(a_rhs.fileCount)),
						pad(0),
						fileOffset(std::move(a_rhs.fileOffset))
					{}

					constexpr block64_t& operator=(const block64_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = a_rhs.fileCount;
							fileOffset = a_rhs.fileOffset;
						}
						return *this;
					}

					constexpr block64_t& operator=(block64_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							fileCount = std::move(a_rhs.fileCount);
							fileOffset = std::move(a_rhs.fileOffset);
						}
						return *this;
					}

					inline void read(istream_t& a_input, [[maybe_unused]] const header_t& a_header)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block64_t));
						pad = 0;
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
					auto pos = a_input.tell();
					a_input.seek_beg(static_cast<std::streamoff>(file_offset() - a_header.file_names_length()));

					if (a_header.directory_strings()) {
						std::uint8_t length;
						a_input.read(reinterpret_cast<char*>(&length), sizeof(length));
						_name.resize(static_cast<std::size_t>(length) - 1);
						a_input.read(_name.data(), length);
					}

					for (std::size_t i = 0; i < file_count(); ++i) {
						auto file = std::make_shared<file_t>();
						file->read(a_input, a_header);
						_files.push_back(std::move(file));
					}

					a_input.seek_abs(pos);
				}

				hash_t _hash;
				std::variant<block86_t, block64_t> _block;
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

				[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
				{
					verify_path(a_path);
					auto fullPath = normalize(std::move(a_path));
					return hash(fullPath);
				}

			protected:
				[[nodiscard]] inline hash_t hash(std::string_view a_fullPath) const
				{
					constexpr auto LEN_MAX = static_cast<std::size_t>(std::numeric_limits<std::int8_t>::max());

					hash_t hash;
					auto& block = hash.block_ref();
					switch (std::min<std::size_t>(a_fullPath.length(), 3)) {
					case 3:
						block.last2 = a_fullPath[a_fullPath.length() - 2];
						[[fallthrough]];
					case 2:
					case 1:
						block.last = a_fullPath.back();
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

				inline constexpr void verify_path(const std::string_view& a_path) const
				{
					for (auto& ch : a_path) {
						if (ch < 0) {
							throw hash_error();
						}
					}
				}

				static constexpr std::uint32_t HASH_CONSTANT = 0x1003F;

			private:
				[[nodiscard]] inline std::string normalize(std::string_view a_path) const
				{
					std::filesystem::path path(a_path);
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

				[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
				{
					verify_path(a_path);
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

				[[nodiscard]] inline std::pair<std::string, std::string> normalize(std::string_view a_path) const
				{
					std::filesystem::path path(a_path);
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

				[[nodiscard]] inline hash_t hash(std::string_view a_stem, std::string_view a_extension) const
				{
					constexpr std::array<extension_t, 6> EXTENSIONS = {
						extension_t("\0\0\0\0"),
						extension_t(".nif"),
						extension_t(".kf\0"),
						extension_t(".dds"),
						extension_t(".wav"),
						extension_t(".adp")
					};

					auto hash = dir_hasher::hash(a_stem);
					auto& block = hash.block_ref();

					std::uint32_t extCRC = 0;
					for (auto& ch : a_extension) {
						extCRC = ch + extCRC * HASH_CONSTANT;
					}
					block.crc += extCRC;

					extension_t ext(a_extension);
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

			[[nodiscard]] inline std::uint32_t crc() const noexcept { return _impl.get().crc(); }
			[[nodiscard]] inline char first_char() const noexcept { return static_cast<char>(_impl.get().first()); }
			[[nodiscard]] inline char last_char() const noexcept { return static_cast<char>(_impl.get().last()); }
			[[nodiscard]] inline std::int8_t length() const noexcept { return _impl.get().length(); }
			[[nodiscard]] inline std::uint64_t numeric_hash() const noexcept { return _impl.get().numeric(); }
			[[nodiscard]] inline char second_to_last_char() const noexcept { return static_cast<char>(_impl.get().last2()); }

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

			[[nodiscard]] inline const char* c_str() const noexcept { return _impl->c_str(); }
			[[nodiscard]] inline hash hash_value() const noexcept { return hash(_impl->hash_ref()); }
			[[nodiscard]] inline std::size_t size() const noexcept { return _impl->size(); }
			[[nodiscard]] inline const std::string& string() const noexcept { return _impl->str_ref(); }

		protected:
			friend class file_iterator;

			using value_type = detail::file_ptr;

			explicit inline file(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline file(value_type&& a_rhs) noexcept :
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

			[[nodiscard]] friend constexpr bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !a_lhs._files && !a_rhs._files; }
			[[nodiscard]] friend constexpr bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline reference operator*() { return fetch(); }
			[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

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
			[[nodiscard]] inline file_iterator operator++([[maybe_unused]] int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			friend inline void swap(file_iterator& a_lhs, file_iterator& a_rhs)
			{
				auto tmp = std::move(a_lhs);
				a_lhs = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class directory;

			explicit inline file_iterator(detail::directory_ptr a_directory) :
				_files(std::in_place_t()),
				_pos(0)
			{
				if (a_directory) {
					for (auto& file : *a_directory) {
						_files->push_back(value_type(file));
					}
				} else {
					_files.reset();
					_pos = NPOS;
				}
			}

		private:
			inline reference fetch() { return _files.value()[_pos]; }

			static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

			std::optional<std::vector<value_type>> _files;
			std::size_t _pos;
		};


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

			[[nodiscard]] inline const char* c_str() const noexcept { return _impl->c_str(); }
			[[nodiscard]] inline std::size_t file_count() const noexcept { return _impl->file_count(); }
			[[nodiscard]] inline hash hash_value() const noexcept { return hash(_impl->hash_ref()); }
			[[nodiscard]] inline const std::string& string() const noexcept { return _impl->str_ref(); }

			[[nodiscard]] inline iterator begin() const { return iterator(_impl); }
			[[nodiscard]] inline iterator end() const noexcept { return iterator(); }

		protected:
			friend class directory_iterator;

			using value_type = detail::directory_ptr;

			explicit inline directory(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline directory(value_type&& a_rhs) noexcept :
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

			inline directory_iterator& operator=(const directory_iterator& a_rhs)
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

			[[nodiscard]] friend constexpr bool operator==(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept { return !a_lhs._dirs && !a_rhs._dirs; }
			[[nodiscard]] friend constexpr bool operator!=(const directory_iterator& a_lhs, const directory_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline reference operator*() { return fetch(); }
			[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

			// prefix
			inline directory_iterator& operator++()
			{
				++_pos;
				if (_pos >= _dirs->size()) {
					_dirs.reset();
					_pos = NPOS;
				}
				return *this;
			}

			// postfix
			[[nodiscard]] inline directory_iterator operator++([[maybe_unused]] int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			friend inline void swap(directory_iterator& a_lhs, directory_iterator& a_rhs)
			{
				auto tmp = std::move(a_lhs);
				a_lhs = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class archive;

			explicit inline directory_iterator(const std::vector<detail::directory_ptr>& a_directories) :
				_dirs(std::in_place_t()),
				_pos(0)
			{
				if (!a_directories.empty()) {
					for (auto& dir : a_directories) {
						_dirs->push_back(value_type(dir));
					}
				} else {
					_dirs.reset();
					_pos = NPOS;
				}
			}

		private:
			inline reference fetch() { return _dirs.value()[_pos]; }

			static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

			std::optional<std::vector<value_type>> _dirs;
			std::size_t _pos;
		};


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

			inline archive(std::unique_ptr<std::istream>&& a_stream) :
				_dirs(),
				_header()
			{
				read(std::move(a_stream));
			}

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

			[[nodiscard]] inline iterator begin() const { return iterator(_dirs); }
			[[nodiscard]] inline iterator end() const noexcept { return iterator(); }

			[[nodiscard]] constexpr std::size_t directory_count() const noexcept { return _header.directory_count(); }
			[[nodiscard]] constexpr std::size_t directory_names_length() const noexcept { return _header.directory_names_length(); }
			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			[[nodiscard]] constexpr std::size_t file_names_length() const noexcept { return _header.file_names_length(); }
			[[nodiscard]] constexpr archive_flag flags() const noexcept { return _header.flags(); }
			[[nodiscard]] constexpr std::size_t header_size() const noexcept { return _header.header_size(); }
			[[nodiscard]] constexpr std::string_view tag() const { return _header.tag(); }
			[[nodiscard]] constexpr archive_type types() const noexcept { return _header.types(); }
			[[nodiscard]] constexpr archive_version version() const noexcept { return _header.version(); }

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

			inline void clear() noexcept { _dirs.clear(); _header.clear(); }

			inline void read(const std::filesystem::path& a_path)
			{
				auto file = std::make_unique<std::ifstream>(a_path, std::ios_base::in | std::ios_base::binary);
				if (!file->is_open()) {
					throw input_error();
				} else {
					read(std::move(file));
				}
			}

			inline void read(std::filesystem::path&& a_path)
			{
				read(a_path);
			}

			inline void read(std::unique_ptr<std::istream>&& a_input)
			{
				detail::istream_t input(std::move(a_input));

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

				input.seek_beg(static_cast<std::streamoff>(_header.header_size()));
				for (std::size_t i = 0; i < _header.directory_count(); ++i) {
					auto dir = std::make_shared<detail::directory_t>();
					dir->read(input, _header);
					_dirs.push_back(std::move(dir));
				}

				auto offset = static_cast<std::streamoff>(_header.directory_names_length()) + static_cast<std::streamoff>(_header.directory_count());	// include prefixed length byte
				offset += static_cast<std::streamoff>(_header.file_count()) * detail::file_t::block_size();
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
				for (const auto& dir : _dirs) {
					auto dHash = detail::dir_hasher()(dir->str_ref());
					if (dHash != dir->hash()) {
						assert(false);
					}

					for (const auto& file : *dir) {
						try {
							auto fHash = detail::file_hasher()(file->str_ref());
							if (fHash != file->hash()) {
								assert(false);
							}
						} catch ([[maybe_unused]] hash_error& e) {
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


	namespace fo3 = tes4;	// Fallout 3
	namespace tes5 = tes4;	// The Elder Scrolls V: Skyrim
	namespace sse = tes4;	// The Elder Scrolls V: Skyrim - Special Edition


	namespace fo4	// Fallout 4
	{
		using archive_version = std::size_t;
		static constexpr archive_version v1 = 1;


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

				[[nodiscard]] constexpr bool directx() const { return format() == DIRECTX; }
				[[nodiscard]] constexpr bool general() const { return format() == GENERAL; }

				[[nodiscard]] constexpr std::size_t file_count() const noexcept { return static_cast<std::size_t>(_block.fileCount); }
				[[nodiscard]] constexpr std::string_view format() const { return std::string_view(_block.contentsFormat, std::extent_v<decltype(_block.contentsFormat)>); }
				[[nodiscard]] constexpr bool has_string_table() const noexcept { return string_table_offset() != 0; }
				[[nodiscard]] constexpr std::string_view magic() const { return std::string_view(_block.magic, std::extent_v<decltype(_block.magic)>); }
				[[nodiscard]] constexpr std::uint64_t string_table_offset() const noexcept { return _block.stringTableOffset; }
				[[nodiscard]] constexpr archive_version version() const noexcept { return static_cast<archive_version>(_block.version); }

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
						magic{ '\0' },
						version(0),
						contentsFormat{ '\0' },
						fileCount(0),
						stringTableOffset(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						magic{ '\0' },
						version(a_rhs.version),
						contentsFormat{ '\0' },
						fileCount(a_rhs.fileCount),
						stringTableOffset(a_rhs.stringTableOffset)
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(magic)>; ++i) {
							magic[i] = a_rhs.magic[i];
						}

						for (std::size_t i = 0; i < std::extent_v<decltype(contentsFormat)>; ++i) {
							contentsFormat[i] = a_rhs.contentsFormat[i];
						}
					}

					constexpr block_t(block_t&& a_rhs) noexcept :
						magic{ '\0' },
						version(std::move(a_rhs.version)),
						contentsFormat{ '\0' },
						fileCount(std::move(a_rhs.fileCount)),
						stringTableOffset(std::move(a_rhs.stringTableOffset))
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(magic)>; ++i) {
							magic[i] = std::move(a_rhs.magic[i]);
						}

						for (std::size_t i = 0; i < std::extent_v<decltype(contentsFormat)>; ++i) {
							contentsFormat[i] = std::move(a_rhs.contentsFormat[i]);
						}
					}

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(magic)>; ++i) {
								magic[i] = a_rhs.magic[i];
							}

							for (std::size_t i = 0; i < std::extent_v<decltype(contentsFormat)>; ++i) {
								contentsFormat[i] = a_rhs.contentsFormat[i];
							}

							version = a_rhs.version;
							fileCount = a_rhs.fileCount;
							stringTableOffset = a_rhs.stringTableOffset;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(magic)>; ++i) {
								magic[i] = std::move(a_rhs.magic[i]);
							}

							for (std::size_t i = 0; i < std::extent_v<decltype(contentsFormat)>; ++i) {
								contentsFormat[i] = std::move(a_rhs.contentsFormat[i]);
							}

							version = std::move(a_rhs.version);
							fileCount = std::move(a_rhs.fileCount);
							stringTableOffset = std::move(a_rhs.stringTableOffset);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
					}

					char magic[4];
					std::uint32_t version;
					char contentsFormat[4];
					std::uint32_t fileCount;
					std::uint64_t stringTableOffset;
				};

				static constexpr auto DIRECTX = std::string_view("DX10", std::extent_v<decltype(block_t::contentsFormat)>);
				static constexpr auto GENERAL = std::string_view("GNRL", std::extent_v<decltype(block_t::contentsFormat)>);
				static constexpr auto MAGIC = std::string_view("BTDX", std::extent_v<decltype(block_t::magic)>);

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

				[[nodiscard]] friend constexpr bool operator==(const hash_t& a_lhs, const hash_t& a_rhs) noexcept
				{
					if (a_lhs.file_hash() != a_rhs.file_hash() ||
						a_lhs.directory_hash() != a_rhs.directory_hash()) {
						return false;
					} else {
						return std::char_traits<char>::compare(a_lhs._block.ext, a_rhs._block.ext, std::extent_v<decltype(_block.ext)>) == 0;
					}
				}

				[[nodiscard]] friend constexpr bool operator!=(const hash_t& a_lhs, const hash_t& a_rhs) noexcept { return !(a_lhs == a_rhs); }

				[[nodiscard]] constexpr std::uint32_t directory_hash() const noexcept { return _block.dir; }
				[[nodiscard]] constexpr std::string_view extension() const { return std::string_view(_block.ext, std::extent_v<decltype(_block.ext)>); }
				[[nodiscard]] constexpr std::uint32_t file_hash() const noexcept { return _block.file; }

				inline void read(istream_t& a_input) { _block.read(a_input); }

			protected:
				friend class file_hasher;

				struct block_t	// BSResource::ID
				{
					constexpr block_t() noexcept :
						file(0),
						ext{ '\0' },
						dir(0)
					{}

					constexpr block_t(const block_t& a_rhs) noexcept :
						file(a_rhs.file),
						ext{ '\0' },
						dir(a_rhs.dir)
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(ext)>; ++i) {
							ext[i] = a_rhs.ext[i];
						}
					}

					constexpr block_t(block_t&& a_rhs) noexcept :
						file(std::move(a_rhs.file)),
						ext{ '\0' },
						dir(std::move(a_rhs.dir))
					{
						for (std::size_t i = 0; i < std::extent_v<decltype(ext)>; ++i) {
							ext[i] = std::move(a_rhs.ext[i]);
						}
					}

					constexpr block_t& operator=(const block_t& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(ext)>; ++i) {
								ext[i] = a_rhs.ext[i];
							}

							file = a_rhs.file;
							dir = a_rhs.dir;
						}
						return *this;
					}

					constexpr block_t& operator=(block_t&& a_rhs) noexcept
					{
						if (this != std::addressof(a_rhs)) {
							for (std::size_t i = 0; i < std::extent_v<decltype(ext)>; ++i) {
								ext[i] = std::move(a_rhs.ext[i]);
							}

							file = std::move(a_rhs.file);
							dir = std::move(a_rhs.dir);
						}
						return *this;
					}

					inline void read(istream_t& a_input)
					{
						a_input.read(reinterpret_cast<char*>(this), sizeof(block_t));
					}

					std::uint32_t file;
					char ext[4];
					std::uint32_t dir;
				};

				[[nodiscard]] constexpr block_t& block_ref() noexcept { return _block; }
				[[nodiscard]] constexpr const block_t& block_ref() const noexcept { return _block; }

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

				[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

				[[nodiscard]] constexpr std::ptrdiff_t chunk_count() const noexcept { return static_cast<std::ptrdiff_t>(_header.chunkCount); }
				[[nodiscard]] constexpr std::size_t chunk_offset() const noexcept { return static_cast<std::size_t>(_header.chunkOffsetOrType); }

				[[nodiscard]] constexpr std::ptrdiff_t data_file_index() const noexcept { return static_cast<std::ptrdiff_t>(_header.dataFileIndex); }

				[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
				[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
				[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				[[nodiscard]] inline std::string str() const { return _name; }
				[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

				inline void read(istream_t& a_input)
				{
					_hash.read(a_input);
					_header.read(a_input);
					if (chunk_count() > 0) {
						_chunks.resize(static_cast<std::size_t>(chunk_count()));
						for (auto& chunk : _chunks) {
							chunk.read(a_input);
						}
					}
				}

				inline void read_name(istream_t& a_input)
				{
					std::uint16_t length;
					a_input.read(reinterpret_cast<char*>(&length), sizeof(length));
					_name.resize(length);
					a_input.read(_name.data(), static_cast<std::streamsize>(_name.length()));
				}

			private:
				struct header_t	// BSResource::Archive2::Index::EntryHeader
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
						a_input.read(reinterpret_cast<char*>(this), sizeof(header_t));
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
						a_input.read(reinterpret_cast<char*>(this), sizeof(chunk_t));

						std::uint32_t sentinel;
						a_input.read(reinterpret_cast<char*>(&sentinel), sizeof(sentinel));
						if (sentinel != BAADFOOD) {
							throw input_error();
						}
					}

					static constexpr std::uint32_t BAADFOOD = 0xBAADF00D;

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

				[[nodiscard]] constexpr std::ptrdiff_t chunk_count() const noexcept { return static_cast<std::ptrdiff_t>(_header.chunkCount); }
				[[nodiscard]] constexpr std::size_t chunk_offset() const noexcept { return static_cast<std::size_t>(_header.chunkOffset); }

				[[nodiscard]] inline const char* c_str() const noexcept { return _name.c_str(); }

				[[nodiscard]] constexpr std::ptrdiff_t data_file_index() const noexcept { return static_cast<std::ptrdiff_t>(_header.dataFileIndex); }

				[[nodiscard]] constexpr std::ptrdiff_t flags() const noexcept { return static_cast<std::ptrdiff_t>(_header.flags); }

				[[nodiscard]] constexpr std::ptrdiff_t format() const noexcept { return static_cast<std::ptrdiff_t>(_header.format); }

				[[nodiscard]] constexpr std::size_t height() const noexcept { return static_cast<std::size_t>(_header.height); }

				[[nodiscard]] constexpr hash_t hash() const noexcept { return _hash; }
				[[nodiscard]] constexpr hash_t& hash_ref() noexcept { return _hash; }
				[[nodiscard]] constexpr const hash_t& hash_ref() const noexcept { return _hash; }

				[[nodiscard]] constexpr std::ptrdiff_t mip_count() const noexcept { return static_cast<std::ptrdiff_t>(_header.mipCount); }

				[[nodiscard]] inline std::string str() const { return _name; }
				[[nodiscard]] constexpr std::string& str_ref() noexcept { return _name; }
				[[nodiscard]] constexpr const std::string& str_ref() const noexcept { return _name; }

				[[nodiscard]] constexpr std::ptrdiff_t tile_mode() const noexcept { return static_cast<std::ptrdiff_t>(_header.tilemode); }

				[[nodiscard]] constexpr std::size_t width() const noexcept { return static_cast<std::size_t>(_header.width); }

				inline void read(istream_t& a_input)
				{
					_hash.read(a_input);
					_header.read(a_input);
					if (chunk_count() > 0) {
						_chunks.resize(static_cast<std::size_t>(chunk_count()));
						for (auto& chunk : _chunks) {
							chunk.read(a_input);
						}
					}
				}

				inline void read_name(istream_t& a_input)
				{
					std::uint16_t length;
					a_input.read(reinterpret_cast<char*>(&length), sizeof(length));
					_name.resize(length);
					a_input.read(_name.data(), static_cast<std::streamsize>(_name.length()));
				}

			private:
				struct header_t	// BSTextureStreamer::NativeDesc<BSGraphics::TextureHeader>
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
						a_input.read(reinterpret_cast<char*>(this), sizeof(header_t));
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
						a_input.read(reinterpret_cast<char*>(this), sizeof(chunk_t));
						if (sentinel != BAADFOOD) {
							throw input_error();
						}
					}

					static constexpr std::uint32_t BAADFOOD = 0xBAADF00D;

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

				[[nodiscard]] inline hash_t operator()(std::string_view a_path) const
				{
					for (auto& ch : a_path) {
						if (ch < 0) {
							throw hash_error();
						}
					}

					auto [file, extension, directory] = normalize(std::move(a_path));
					hash_t hash;
					auto& block = hash.block_ref();

					block.file = hash_string(file);
					block.dir = hash_string(directory);
					for (std::size_t i = 0; i < std::min(extension.size(), std::extent_v<decltype(block.ext)>); ++i) {
						block.ext[i] = extension[i];
					}

					return hash;
				}

			private:
				static constexpr std::array<std::uint32_t, 256> CRCTABLE =
				{
					{
						0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
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
						0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
					}
				};

				[[nodiscard]] constexpr std::uint32_t hash_string(std::string_view a_string) const
				{
					std::uint32_t hash = 0;
					for (auto& ch : a_string) {
						hash = (hash >> 8) ^ CRCTABLE[(hash ^ static_cast<std::uint8_t>(ch)) & 0xFF];
					}
					return hash;
				}

				[[nodiscard]] inline std::tuple<std::string, std::string, std::string> normalize(std::string_view a_path) const
				{
					std::filesystem::path path(a_path);
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

			[[nodiscard]] inline std::uint32_t directory_hash() const noexcept { return _impl.get().directory_hash(); }
			[[nodiscard]] inline std::string_view extension() const { return _impl.get().extension(); }
			[[nodiscard]] inline std::uint32_t file_hash() const noexcept { return _impl.get().file_hash(); }

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

			[[nodiscard]] inline std::ptrdiff_t chunk_count() const noexcept { return _impl->chunk_count(); }
			[[nodiscard]] inline const char* c_str() const noexcept { return _impl->c_str(); }
			[[nodiscard]] inline hash hash_value() const noexcept { return hash(_impl->hash_ref()); }
			[[nodiscard]] inline const std::string& string() const noexcept { return _impl->str_ref(); }

		protected:
			friend class file_iterator;

			using value_type = detail::general_ptr;

			explicit inline general_file(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline general_file(value_type&& a_rhs) noexcept :
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

			[[nodiscard]] inline std::ptrdiff_t chunk_count() const noexcept { return _impl->chunk_count(); }
			[[nodiscard]] inline const char* c_str() const noexcept { return _impl->c_str(); }
			[[nodiscard]] inline std::ptrdiff_t flags() const noexcept { return _impl->flags(); }
			[[nodiscard]] inline std::ptrdiff_t format() const noexcept { return _impl->format(); }
			[[nodiscard]] inline hash hash_value() const noexcept { return hash(_impl->hash_ref()); }
			[[nodiscard]] inline std::size_t height() const noexcept { return _impl->height(); }
			[[nodiscard]] inline std::ptrdiff_t mip_count() const noexcept { return _impl->mip_count(); }
			[[nodiscard]] inline const std::string& string() const noexcept { return _impl->str_ref(); }
			[[nodiscard]] inline std::ptrdiff_t tile_mode() const noexcept { return _impl->tile_mode(); }
			[[nodiscard]] inline std::size_t width() const noexcept { return _impl->width(); }

		protected:
			friend class file_iterator;

			using value_type = detail::texture_ptr;

			explicit inline texture_file(const value_type& a_rhs) noexcept :
				_impl(a_rhs)
			{}

			explicit inline texture_file(value_type&& a_rhs) noexcept :
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

			explicit inline file_entry(const general_file& a_rhs) :
				_impl(a_rhs)
			{}

			explicit inline file_entry(general_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

			explicit inline file_entry(const texture_file& a_rhs) :
				_impl(a_rhs)
			{}

			explicit inline file_entry(texture_file&& a_rhs) noexcept :
				_impl(std::move(a_rhs))
			{}

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

			[[nodiscard]] constexpr bool is_general_file() const noexcept { return _impl.index() == igeneral; }
			[[nodiscard]] constexpr bool is_texture_file() const noexcept { return _impl.index() == itexture; }

			[[nodiscard]] constexpr const general_file& general_file() const { return std::get<igeneral>(_impl); }
			[[nodiscard]] constexpr const texture_file& texture_file() const { return std::get<itexture>(_impl); }

			[[nodiscard]] const char* c_str() const noexcept
			{
				switch (_impl.index()) {
				case igeneral:
					return std::get<igeneral>(_impl).c_str();
				case itexture:
					return std::get<itexture>(_impl).c_str();
				default:
					return "";
				}
			}

			[[nodiscard]] const std::string& string() const noexcept
			{
				static std::string defaultStr;

				switch (_impl.index()) {
				case igeneral:
					return std::get<igeneral>(_impl).string();
				case itexture:
					return std::get<itexture>(_impl).string();
				default:
					return defaultStr;
				}
			}

		private:
			enum : std::size_t { imono, igeneral, itexture };

			std::variant<std::monostate, general_file_t, texture_file_t> _impl;
		};


		class file_iterator
		{
		public:
			using value_type = file_entry;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::input_iterator_tag;

			constexpr file_iterator() noexcept :
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

			[[nodiscard]] friend constexpr bool operator==(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !a_lhs._files && !a_rhs._files; }
			[[nodiscard]] friend constexpr bool operator!=(const file_iterator& a_lhs, const file_iterator& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline reference operator*() { return fetch(); }
			[[nodiscard]] inline pointer operator->() { return std::addressof(fetch()); }

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
			[[nodiscard]] inline file_iterator operator++([[maybe_unused]] int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			friend inline void swap(file_iterator& a_lhs, file_iterator& a_rhs)
			{
				auto tmp = std::move(a_lhs);
				a_lhs = std::move(a_rhs);
				a_rhs = std::move(tmp);
			}

		protected:
			friend class archive;

			explicit inline file_iterator(const std::vector<detail::general_ptr>& a_general) :
				_files(std::in_place_t()),
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
				_files(std::in_place_t()),
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

			static constexpr auto NPOS = std::numeric_limits<std::size_t>::max();

			std::optional<std::vector<value_type>> _files;
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

			inline archive(const std::filesystem::path& a_path) :
				_files(),
				_header()
			{
				read(a_path);
			}

			inline archive(std::filesystem::path&& a_path) :
				_files(),
				_header()
			{
				read(std::move(a_path));
			}

			inline archive(std::unique_ptr<std::istream>&& a_stream) :
				_files(),
				_header()
			{
				read(std::move(a_stream));
			}

			inline archive& operator=(const archive& a_rhs)
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

			[[nodiscard]] inline iterator begin() const
			{
				switch (_files.index()) {
				case igeneral:
					return iterator(std::get<igeneral>(_files));
				case itexture:
					return iterator(std::get<itexture>(_files));
				default:
					return end();
				}
			}

			[[nodiscard]] inline iterator end() const noexcept { return iterator(); }

			[[nodiscard]] constexpr std::size_t file_count() const noexcept { return _header.file_count(); }
			[[nodiscard]] constexpr std::string_view format() const { return _header.format(); }
			[[nodiscard]] constexpr std::string_view tag() const { return _header.magic(); }
			[[nodiscard]] constexpr archive_version version() const noexcept { return _header.version(); }

			[[nodiscard]] constexpr bool file_strings() const noexcept { return _header.has_string_table(); }

			[[nodiscard]] constexpr bool directx() const { return _header.directx(); }
			[[nodiscard]] constexpr bool general() const { return _header.general(); }

			inline void clear() noexcept	// !!!
			{
				if (_files.valueless_by_exception()) {
					_files.emplace<0>();
				} else {
					std::visit([](auto&& a_arg) { a_arg.clear(); }, _files);
				}

				_header.clear();
			}

			inline void read(const std::filesystem::path& a_path)
			{
				auto file = std::make_unique<std::ifstream>(a_path, std::ios_base::in | std::ios_base::binary);
				if (!file->is_open()) {
					throw input_error();
				} else {
					read(std::move(file));
				}
			}

			inline void read(std::filesystem::path&& a_path)
			{
				read(a_path);
			}

			inline void read(std::unique_ptr<std::istream>&& a_input)
			{
				detail::istream_t input(std::move(a_input));

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
					for (auto& file : std::get<cgeneral>(_files)) {
						file = std::make_shared<detail::general_t>();
						file->read(input);
					}
				} else if (_header.directx()) {
					_files.emplace<ctexture>(_header.file_count());
					for (auto& file : std::get<ctexture>(_files)) {
						file = std::make_shared<detail::texture_t>();
						file->read(input);
					}
				} else {
					throw input_error();
				}

				if (_header.has_string_table()) {
					input.seek_beg(static_cast<std::streamoff>(_header.string_table_offset()));

					switch (_files.index()) {
					case igeneral:
						for (auto& file : std::get<igeneral>(_files)) {
							file->read_name(input);
						}
						break;
					case itexture:
						for (auto& file : std::get<itexture>(_files)) {
							file->read_name(input);
						}
						break;
					default:
						throw std::bad_variant_access();
					}
				}

				assert(sanity_check());
			}

		private:
			using cgeneral = std::vector<detail::general_ptr>;
			using ctexture = std::vector<detail::texture_ptr>;

			enum : std::size_t { igeneral, itexture };

			inline bool sanity_check()
			{
				switch (_files.index()) {
				case igeneral:
				case itexture:
					std::visit([](auto&& a_files)
					{
						for (const auto& file : a_files) {
							try {
								auto hash = detail::file_hasher()(file->str_ref());
								if (hash != file->hash_ref()) {
									assert(false);
								}
							} catch ([[maybe_unused]] hash_error&) {
								continue;
							}
						}
					}, _files);
					break;
				default:
					assert(false);
					break;
				}

				return true;
			}

			std::variant<cgeneral, ctexture> _files;
			detail::header_t _header;
		};
	}
}
