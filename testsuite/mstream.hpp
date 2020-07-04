#pragma once

#include <algorithm>
#include <cassert>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <nonstd/span.hpp>

template <
	class CharT,
	class Traits = std::char_traits<CharT>>
class basic_memorybuf :
	public std::basic_streambuf<CharT, Traits>
{
private:
	using super = std::basic_streambuf<CharT, Traits>;

public:
	using char_type = typename super::char_type;
	using traits_type = typename super::traits_type;
	using int_type = typename super::int_type;
	using pos_type = typename super::pos_type;
	using off_type = typename super::off_type;

	basic_memorybuf() :
		basic_memorybuf(nonstd::span<char_type>{})
	{}

	basic_memorybuf(const basic_memorybuf&) = delete;

	basic_memorybuf(basic_memorybuf&& a_rhs) :
		super(std::move(a_rhs)),
		_memory()
	{
		memory(a_rhs._memory);
		a_rhs._memory(nonstd::span<char_type>{});
	}

	basic_memorybuf(nonstd::span<char_type> a_memory) :
		super(),
		_memory()
	{
		memory(a_memory);
	}

	~basic_memorybuf() = default;

	basic_memorybuf& operator=(const basic_memorybuf&) = delete;

	basic_memorybuf& operator=(basic_memorybuf&& a_rhs)
	{
		if (this != std::addressof(a_rhs)) {
			super::operator=(std::move(a_rhs));
			memory(a_rhs._memory);
			a_rhs._memory(nonstd::span<char_type>{});
		}
		return *this;
	}

	void swap(basic_memorybuf& a_rhs)
	{
		using std::swap;
		swap(_memory, a_rhs._memory);
		super::swap(a_rhs);
	}

	[[nodiscard]] nonstd::span<const char_type> memory() const { return { _memory.data(), _memory.size() }; }

	void memory(nonstd::span<char_type> a_memory)
	{
		_memory = a_memory;
		setg(_memory.data(), _memory.data(), _memory.data() + _memory.size());
		setp(_memory.data(), _memory.data(), _memory.data() + _memory.size());
	}

protected:
	using super::eback;
	using super::egptr;
	using super::epptr;
	using super::gptr;
	using super::pbase;
	using super::pptr;
	using super::setg;
	using super::setp;

private:
	nonstd::span<char_type> _memory;
};

template <
	class CharT,
	class Traits>
void swap(basic_memorybuf<CharT, Traits>& a_lhs, basic_memorybuf<CharT, Traits>& a_rhs)
{
	a_lhs.swap(a_rhs);
}

template <
	class CharT,
	class Traits = std::char_traits<CharT>,
	class Container = std::vector<CharT>>
class basic_imemorystream :
	public std::basic_istream<CharT, Traits>
{
private:
	using super = std::basic_istream<CharT, Traits>;

public:
	using char_type = typename super::char_type;
	using traits_type = typename super::traits_type;
	using int_type = typename super::int_type;
	using pos_type = typename super::pos_type;
	using off_type = typename super::off_type;

	using container_type = Container;

	basic_imemorystream() :
		super(),
		_container(),
		_buffer()
	{}

	basic_imemorystream(const basic_imemorystream&) = delete;

	basic_imemorystream(basic_imemorystream&& a_rhs) :
		super(std::move(a_rhs)),
		_container(std::move(a_rhs._container)),
		_buffer(std::move(a_rhs._buffer))
	{}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	basic_imemorystream(Args&&... a_args) :
		super(std::addressof(_buffer)),
		_container(std::forward<Args>(a_args)...),
		_buffer(adl_span(_container))
	{}

	~basic_imemorystream() = default;

	basic_imemorystream& operator=(const basic_imemorystream&) = delete;

	basic_imemorystream& operator=(basic_imemorystream&& a_rhs)
	{
		if (this != std::addressof(a_rhs)) {
			super::operator=(std::move(a_rhs));
			_container = std::move(a_rhs._container);
			_buffer = std::move(a_rhs._buffer);
		}
		return *this;
	}

	void swap(basic_imemorystream& a_rhs)
	{
		using std::swap;
		swap(_container, a_rhs._container);
		swap(_buffer, a_rhs._buffer);
		super::swap(a_rhs);
	}

	[[nodiscard]] basic_memorybuf<char_type, traits_type>* rdbuf() const { return std::addressof(_buffer); }

	[[nodiscard]] nonstd::span<const char_type> span() const { return _buffer.memory(); }

	[[nodiscard]] container_type container() const& { return _container; }

	[[nodiscard]] container_type container() &&
	{
		_buffer.memory(nonstd::span<char_type>{});
		return std::move(_container);
	}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	void container(Args&&... a_args)
	{
		_container = container_type(std::forward<Args>(a_args));
		_buffer.memory(adl_span(_container));
	}

private:
	[[nodiscard]] static nonstd::span<char_type> adl_span(container_type& a_container)
	{
		using std::data;
		using std::size;
		return { data(a_container), size(a_container) };
	}

	container_type _container;
	basic_memorybuf<char_type, traits_type> _buffer;
};

template <
	class CharT,
	class Traits,
	class Container>
void swap(basic_imemorystream<CharT, Traits, Container>& a_lhs, basic_imemorystream<CharT, Traits, Container>& a_rhs)
{
	a_lhs.swap(a_rhs);
}

using imemorystream = basic_imemorystream<char>;
using wimemorystream = basic_imemorystream<wchar_t>;

template <
	class CharT,
	class Traits = std::char_traits<CharT>,
	class Container = std::vector<CharT>>
class basic_omemorystream :
	public std::basic_ostream<CharT, Traits>
{
private:
	using super = std::basic_ostream<CharT, Traits>;

public:
	using char_type = typename super::char_type;
	using traits_type = typename super::traits_type;
	using int_type = typename super::int_type;
	using pos_type = typename super::pos_type;
	using off_type = typename super::off_type;

	using container_type = Container;

	basic_omemorystream() :
		super(),
		_container(),
		_buffer()
	{}

	basic_omemorystream(const basic_omemorystream&) = delete;

	basic_omemorystream(basic_omemorystream&& a_rhs) :
		super(std::move(a_rhs)),
		_container(std::move(a_rhs._container)),
		_buffer(std::move(a_rhs._buffer))
	{}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	basic_omemorystream(Args&&... a_args) :
		super(std::addressof(_buffer)),
		_container(std::forward<Args>(a_args)...),
		_buffer(adl_span(_container))
	{}

	~basic_omemorystream() = default;

	basic_omemorystream& operator=(const basic_omemorystream&) = delete;

	basic_omemorystream& operator=(basic_omemorystream&& a_rhs)
	{
		if (this != std::addressof(a_rhs)) {
			super::operator=(std::move(a_rhs));
			_container = std::move(a_rhs._container);
			_buffer = std::move(a_rhs._buffer);
		}
		return *this;
	}
	void swap(basic_omemorystream& a_rhs)
	{
		using std::swap;
		swap(_container, a_rhs._container);
		swap(_buffer, a_rhs._buffer);
		super::swap(a_rhs);
	}

	[[nodiscard]] basic_memorybuf<char_type, traits_type>* rdbuf() const { return std::addressof(_buffer); }

	[[nodiscard]] nonstd::span<const char_type> span() const { return _buffer.memory(); }

	[[nodiscard]] container_type container() const& { return _container; }

	[[nodiscard]] container_type container() &&
	{
		_buffer.memory(nonstd::span<char_type>{});
		return std::move(_container);
	}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	void container(Args&&... a_args)
	{
		_container = container_type(std::forward<Args>(a_args));
		_buffer.memory(adl_span(_container));
	}

private:
	[[nodiscard]] static nonstd::span<char_type> adl_span(container_type& a_container)
	{
		using std::data;
		using std::size;
		return { data(a_container), size(a_container) };
	}

	container_type _container;
	basic_memorybuf<char_type, traits_type> _buffer;
};

template <
	class CharT,
	class Traits,
	class Container>
void swap(basic_omemorystream<CharT, Traits, Container>& a_lhs, basic_omemorystream<CharT, Traits, Container>& a_rhs)
{
	a_lhs.swap(a_rhs);
}

using omemorystream = basic_omemorystream<char>;
using womemorystream = basic_omemorystream<wchar_t>;

template <
	class CharT,
	class Traits = std::char_traits<CharT>,
	class Container = std::vector<CharT>>
class basic_memorystream :
	public std::basic_iostream<CharT, Traits>
{
private:
	using super = std::basic_iostream<CharT, Traits>;

public:
	using char_type = typename super::char_type;
	using traits_type = typename super::traits_type;
	using int_type = typename super::int_type;
	using pos_type = typename super::pos_type;
	using off_type = typename super::off_type;

	using container_type = Container;

	basic_memorystream() :
		super(),
		_container(),
		_buffer()
	{}

	basic_memorystream(const basic_memorystream&) = delete;

	basic_memorystream(basic_memorystream&& a_rhs) :
		super(std::move(a_rhs)),
		_container(std::move(a_rhs._container)),
		_buffer(std::move(a_rhs._buffer))
	{}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	basic_memorystream(Args&&... a_args) :
		super(std::addressof(_buffer)),
		_container(std::forward<Args>(a_args)...),
		_buffer(adl_span(_container))
	{}

	~basic_memorystream() = default;

	basic_memorystream& operator=(const basic_memorystream&) = delete;

	basic_memorystream& operator=(basic_memorystream&& a_rhs)
	{
		if (this != std::addressof(a_rhs)) {
			super::operator=(std::move(a_rhs));
			_container = std::move(a_rhs._container);
			_buffer = std::move(a_rhs._buffer);
		}
		return *this;
	}

	void swap(basic_memorystream& a_rhs)
	{
		using std::swap;
		swap(_container, a_rhs._container);
		swap(_buffer, a_rhs._buffer);
		super::swap(a_rhs);
	}

	[[nodiscard]] basic_memorybuf<char_type, traits_type>* rdbuf() const { return std::addressof(_buffer); }

	[[nodiscard]] nonstd::span<const char_type> span() const { return _buffer.memory(); }

	[[nodiscard]] container_type container() const& { return _container; }

	[[nodiscard]] container_type container() &&
	{
		_buffer.memory(nonstd::span<char_type>{});
		return std::move(_container);
	}

	template <
		class... Args,
		std::enable_if_t<
			std::is_constructible_v<
				container_type,
				Args...>,
			int> = 0>
	void container(Args&&... a_args)
	{
		_container = container_type(std::forward<Args>(a_args));
		_buffer.memory(adl_span(_container));
	}

private:
	[[nodiscard]] static nonstd::span<char_type> adl_span(container_type& a_container)
	{
		using std::data;
		using std::size;
		return { data(a_container), size(a_container) };
	}

	container_type _container;
	basic_memorybuf<char_type, traits_type> _buffer;
};

template <
	class CharT,
	class Traits,
	class Container>
void swap(basic_memorystream<CharT, Traits, Container>& a_lhs, basic_memorystream<CharT, Traits, Container>& a_rhs)
{
	a_lhs.swap(a_rhs);
}

using memorystream = basic_memorystream<char>;
using wmemorystream = basic_memorystream<wchar_t>;
