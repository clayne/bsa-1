#pragma once

#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>


#if __cplusplus >= 201402L	// C++14

#define BSA_CXX14_CONSTEXPR constexpr
#define BSA_CXX14_NOEXCEPT noexcept(true)

namespace bsa
{
	namespace stl
	{
		using std::add_const_t;
		using std::common_type_t;
		using std::conditional_t;
		using std::enable_if_t;
		using std::make_signed_t;
		using std::make_unsigned_t;
		using std::remove_cv_t;
		using std::underlying_type_t;
	}
}

#else

#define BSA_CXX14_CONSTEXPR inline
#define BSA_CXX14_NOEXCEPT noexcept(false)

namespace bsa
{
	namespace stl
	{
		template <class T>
		using add_const_t = typename std::add_const<T>::type;

		template <class... Args>
		using common_type_t = typename common_type<Args...>::type;

		template <bool B, class T, class F>
		using conditional_t = typename std::conditional<B, T, F>::type;

		template <bool B, class T = void>
		using enable_if_t = typename std::enable_if<B, T>::type;

		template <class T>
		using make_signed_t = typename make_signed<T>::type;

		template <class T>
		using make_unsigned_t = typename make_unsigned<T>::type;

		template <class T>
		using remove_cv_t = typename remove_cv<T>::type;

		template <class T>
		using underlying_type_t = typename std::underlying_type<T>::type;
	}
}

#endif	// C++14


#if __cplusplus >= 201703L	// C++17

#define BSA_CXX17_CONSTEXPR constexpr
#define BSA_CXX17_NOEXCEPT noexcept(true)
#define BSA_CXX17_INLINE inline

#define BSA_FALLTHROUGH [[fallthrough]]
#define BSA_MAYBE_UNUSED [[maybe_unused]]
#define BSA_NODISCARD [[nodiscard]]

#include <filesystem>
#include <optional>
#include <string_view>
#include <variant>

namespace bsa
{
	namespace stl
	{
		namespace filesystem
		{
			using std::filesystem::create_directories;
			using std::filesystem::exists;
			using std::filesystem::path;
		}

		using std::string_view;

		using std::in_place;
		using std::nullopt;
		using std::optional;

		using std::bad_variant_access;
		using std::get;
		using std::monostate;
		using std::variant;
		using std::visit;

		using std::is_same_v;

		using std::conjunction;
		using std::conjunction_v;
		using std::disjunction;
		using std::disjunction_v;
		using std::is_arithmetic_v;
		using std::is_constructible_v;
		using std::is_enum_v;
		using std::is_integral_v;
		using std::is_invocable_r_v;
		using std::is_pointer_v;
		using std::is_signed_v;
		using std::is_unsigned_v;
		using std::negation;
		using std::void_t;

		using std::byte;
		using std::to_integer;
	}
}

#else

#define BSA_CXX17_CONSTEXPR inline
#define BSA_CXX17_NOEXCEPT noexcept(false)
#define BSA_CXX17_INLINE static

#define BSA_FALLTHROUGH
#define BSA_MAYBE_UNUSED
#define BSA_NODISCARD

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/variant2/variant.hpp>

namespace bsa
{
	namespace stl
	{
		namespace filesystem
		{
			using boost::filesystem::create_directories;
			using boost::filesystem::exists;
			using boost::filesystem::path;
		}

		using string_view = boost::string_ref;

		static const auto in_place = boost::in_place_init;
		static const auto nullopt = boost::none;
		using boost::optional;

		using boost::variant2::bad_variant_access;
		using boost::variant2::get;
		using boost::variant2::monostate;
		using boost::variant2::variant;
		using boost::variant2::visit;

		template <class T, class U>
		static constexpr bool is_same_v = std::is_same<T, U>::value;

		template <class...>
		struct conjunction :
			std::false_type
		{};

		template <class B>
		struct conjunction<B> :
			B
		{};

		template <class B1, class... BN>
		struct conjunction<B1, BN...> :
			stl::conditional_t<bool(B1::value), stl::conjunction<BN...>, B1>
		{};

		template <class... B>
		static constexpr bool conjunction_v = stl::conjunction<B...>::value;

		template <class...>
		struct disjunction :
			std::false_type
		{};

		template <class B>
		struct disjunction<B> :
			B
		{};

		template <class B1, class... BN>
		struct disjunction<B1, BN...> :
			stl::conditional_t<bool(B1::value), B1, stl::disjunction<BN...>>
		{};

		template <class... B>
		static constexpr bool disjunction_v = stl::disjunction<B...>::value;

		template <class T>
		static constexpr bool is_arithmetic_v = std::is_arithmetic<T>::value;

		template <class T, class... Args>
		static constexpr bool is_constructible_v = is_constructible<T, Args...>::value;

		template <class T>
		static constexpr bool is_enum_v = std::is_enum<T>::value;

		template <class T>
		static constexpr bool is_integral_v = std::is_integral<T>::value;

		template <class, class, class, class...>
		struct _is_invocable_r :
			std::false_type
		{};

		template <class R, class F, class... Args>
		struct _is_invocable_r<
			stl::enable_if_t<
				stl::is_same_v<
					R,
					decltype(
						std::declval<F>()(std::declval<Args>()...))>>,
			R,
			F,
			Args...> :
			std::true_type
		{};

		template <class R, class F, class... Args>
		struct is_invocable_r :
			_is_invocable_r<void, R, F, Args...>
		{};

		template <class R, class F, class... Args>
		static constexpr bool is_invocable_r_v = stl::is_invocable_r<R, F, Args...>::value;

		template <class T>
		static constexpr bool is_pointer_v = std::is_pointer<T>::value;

		template <class T>
		static constexpr bool is_signed_v = std::is_signed<T>::value;

		template <class T>
		static constexpr bool is_unsigned_v = std::is_unsigned<T>::value;

		template <class B>
		struct negation :
			std::bool_constant<!bool(B::value)>
		{};

		template <class...>
		using void_t = void;

		enum class byte : unsigned char
		{
		};

		template <
			class IntegerType,
			stl::enable_if_t<
				stl::is_integral_v<IntegerType>,
				int> = 0>
		constexpr IntegerType to_integer(stl::byte a_byte) noexcept
		{
			return static_cast<IntegerType>(a_byte);
		}
	}
}

#endif	// C++17


#if __cplusplus >= 202002L	// C++20

#include <bit>
#include <span>

#define BSA_CXX20_CONSTEXPR constexpr
#define BSA_CXX20_NOEXCEPT noexcept(true)

#define BSA_NO_UNIQUE_ADDRESS [[no_unique_address]]

namespace bsa
{
	namespace stl
	{
		using std::rotl;
		using std::rotr;
		using std::ssize;

		using std::span;
	}
}

#else

#define BSA_CXX20_CONSTEXPR inline
#define BSA_CXX20_NOEXCEPT noexcept(false)

#define BSA_NO_UNIQUE_ADDRESS

#include <boost/beast/core/span.hpp>

namespace bsa
{
	namespace stl
	{
		namespace detail
		{
			template <class, class = void>
			struct implements_size :
				std::false_type
			{};

			template <class T>
			struct implements_size<
				T,
				stl::void_t<
					decltype(std::declval<T>().size())>> :
				std::is_integral<
					decltype(std::declval<T>().size())>
			{};

			template <class T>
			inline constexpr bool implements_size_v = implements_size<T>::value;
		}

		template <class T, stl::enable_if_t<stl::is_unsigned_v<T>, int> = 0>
		BSA_NODISCARD constexpr T rotl(T a_val, int a_pos) noexcept;
		template <class T, stl::enable_if_t<stl::is_unsigned_v<T>, int> = 0>
		BSA_NODISCARD constexpr T rotr(T a_val, int a_pos) noexcept;


		template <
			class T,
			stl::enable_if_t<
				stl::is_unsigned_v<T>,
				int>>
		BSA_NODISCARD constexpr T rotl(T a_val, int a_pos) noexcept
		{
			constexpr auto N = std::numeric_limits<T>::digits;
			const auto rot = a_pos % N;
			if (rot == 0) {
				return a_val;
			} else if (rot < 0) {
				return rotr(a_val, -rot);
			} else {
				return (a_val << rot) | (a_val >> (N - rot));
			}
		}


		template <
			class T,
			stl::enable_if_t<
				stl::is_unsigned_v<T>,
				int>>
		BSA_NODISCARD constexpr T rotr(T a_val, int a_pos) noexcept
		{
			constexpr auto N = std::numeric_limits<T>::digits;
			const auto rot = a_pos % N;
			if (rot == 0) {
				return a_val;
			} else if (rot < 0) {
				return rotl(a_val, -rot);
			} else {
				return (a_val >> rot) | (a_val << (N - rot));
			}
		}

		template <
			class C,
			stl::enable_if_t<
				detail::implements_size_v<const C&>,
				int> = 0>
		BSA_NODISCARD constexpr auto ssize(const C& a_container)
			-> stl::common_type_t<
				std::ptrdiff_t,
				stl::make_signed_t<decltype(a_container.size())>>
		{
			using result_t =
				stl::common_type_t<
					std::ptrdiff_t,
					stl::make_signed_t<decltype(a_container.size())>>;
			return static_cast<result_t>(a_container.size());
		}

		template <class T, std::ptrdiff_t N>
		BSA_NODISCARD constexpr std::ptrdiff_t ssize(const T (&a_array)[N]) noexcept
		{
			return N;
		}

		using boost::beast::span;
	}
}

#endif	// C++20


namespace bsa
{
	namespace stl
	{
		// C++??

		// Krystian Stasiowski
		// https://www.reddit.com/r/cpp/comments/gb2oz1/convert_bytes_to_object_memcpy_vs_reinterpret/fp6lhdr/
		template <class T>
		T* start_lifetime_as(void* a_ptr) noexcept
		{
			// std::memmove will implicitly create objects within the
			// destination prior to copying, which means that we
			// can force an unsigned char to be created that has
			// the same value as whatever the storage had prior.
			// since the destination is the same as the source,
			// we can deduce from the intent in the paper that
			// because the object is created prior to when the copy
			// occurs some kind of pointer rebinding must occur
			// in order to copy into the created object.
			// one of the preconditions of std::launder is that an object of type
			// T exists within the storage, so this provides us with the UB we need
			// to force the creation of the object.

			auto ptr = static_cast<T*>(
				std::memmove(a_ptr, a_ptr, sizeof(T)));

#if __cplusplus >= 201703L
			return std::launder(ptr);
#else
			return ptr;
#endif
		}
	}
}
