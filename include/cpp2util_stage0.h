//  cpp2util_stage0.h — Minimal runtime for cppfront compiler self-hosting.
//  Stripped down to only what the compiler actually uses.
//  No contracts, no is/as, no safety checks, no string interpolation.

#ifndef CPP2_CPP2UTIL_H
#define CPP2_CPP2UTIL_H

#include <utility>
#include <type_traits>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <algorithm>
#include <cstddef>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <variant>
#include <optional>
#include <functional>
#include <unordered_map>
#include <numeric>

//-----------------------------------------------------------------------
//  Core macros
//-----------------------------------------------------------------------

#define CPP2_TYPEOF(x)              std::remove_cvref_t<decltype(x)>
#define CPP2_COPY(x)                CPP2_TYPEOF(x)(x)
#define CPP2_FORWARD(x)             std::forward<decltype(x)>(x)
#define CPP2_CONSTEXPR              constexpr
#define CPP2_FORCE_INLINE_LAMBDA_CLANG /* empty */
#define CPP2_LAMBDA_NO_DISCARD      [[nodiscard]]

#if defined(_MSC_VER)
    #define CPP2_FORCE_INLINE              __forceinline
    #define CPP2_FORCE_INLINE_LAMBDA       [[msvc::forceinline]]
#elif defined(__clang__) || defined(__GNUC__)
    #define CPP2_FORCE_INLINE              __attribute__((always_inline)) inline
    #if defined(__clang__)
        #define CPP2_FORCE_INLINE_LAMBDA_CLANG __attribute__((always_inline))
    #endif
    #define CPP2_FORCE_INLINE_LAMBDA       __attribute__((always_inline))
#endif

//-----------------------------------------------------------------------
//  Integer typedefs
//-----------------------------------------------------------------------

namespace cpp2 {

using i8        = std::int8_t;
using i16       = std::int16_t;
using i32       = std::int32_t;
using i64       = std::int64_t;
using u8        = std::uint8_t;
using u16       = std::uint16_t;
using u32       = std::uint32_t;
using u64       = std::uint64_t;
using ushort    = unsigned short;
using uint      = unsigned int;
using ulong     = unsigned long;
using longlong  = long long;
using ulonglong = unsigned long long;

//-----------------------------------------------------------------------
//  unchecked_narrow
//-----------------------------------------------------------------------

namespace impl {

template< typename To, typename From >
constexpr auto is_narrowing_v =
    (std::is_floating_point_v<From> && std::is_integral_v<To>) ||
    (std::is_floating_point_v<From> && std::is_floating_point_v<To> && sizeof(From) > sizeof(To)) ||
    (std::is_integral_v<From> && std::is_floating_point_v<To>) ||
    (std::is_enum_v<From> && std::is_floating_point_v<To>) ||
    (std::is_integral_v<From> && std::is_integral_v<To> && sizeof(From) > sizeof(To)) ||
    (std::is_enum_v<From> && std::is_integral_v<To> && sizeof(From) > sizeof(To)) ||
    (std::is_pointer_v<From> && std::is_same_v<To, bool>);

}

template <typename C, typename X>
constexpr auto unchecked_narrow( X x ) noexcept -> C
{
    return static_cast<C>(x);
}

template <typename C, typename X>
constexpr auto unchecked_cast( X&& x ) noexcept -> decltype(auto)
{
    return static_cast<C>(CPP2_FORWARD(x));
}

//-----------------------------------------------------------------------
//  cmp_less / cmp_greater — signed/unsigned safe comparisons
//-----------------------------------------------------------------------

namespace impl {

template<typename T, typename U>
constexpr bool cmp_less(T t, U u) noexcept {
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>) {
        return t < 0 || std::make_unsigned_t<T>(t) < u;
    } else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>) {
        return u >= 0 && t < std::make_unsigned_t<U>(u);
    } else {
        return t < u;
    }
}

template<typename T, typename U>
constexpr bool cmp_greater(T t, U u) noexcept {
    return cmp_less(u, t);
}

template<typename T, typename U>
constexpr bool cmp_less_eq(T t, U u) noexcept {
    return !cmp_greater(t, u);
}

template<typename T, typename U>
constexpr bool cmp_greater_eq(T t, U u) noexcept {
    return !cmp_less(t, u);
}

}

//-----------------------------------------------------------------------
//  assert_not_null, assert_in_bounds
//-----------------------------------------------------------------------

namespace impl {

constexpr auto assert_not_null(auto&& arg) -> decltype(auto)
{
    if (arg == nullptr) {
        throw std::runtime_error("null dereference attempt");
    }
    return CPP2_FORWARD(arg);
}

template <class T>
constexpr auto assert_in_bounds(auto&& x) -> decltype(auto)
{
    return CPP2_FORWARD(x);
}

constexpr auto assert_in_bounds(auto&& x, auto&& arg) -> decltype(auto)
{
    assert(!impl::cmp_less(arg, 0) && !impl::cmp_greater_eq(arg, std::ssize(x)));
    return x[arg];
}

template <auto N>
constexpr auto assert_in_bounds(auto&& x) -> decltype(auto)
{
    static_assert(N >= 0);
    return x[N];
}

}

#define CPP2_ASSERT_IN_BOUNDS(x,arg)         (cpp2::impl::assert_in_bounds((x),(arg)))
#define CPP2_ASSERT_IN_BOUNDS_LITERAL(x,arg) (cpp2::impl::assert_in_bounds<(arg)>(x))

//-----------------------------------------------------------------------
//  in<T> — pass-by-value-or-const-ref
//-----------------------------------------------------------------------

namespace impl {

template<typename T>
constexpr bool prefer_pass_by_value =
    sizeof(T) <= 2*sizeof(void*)
    && std::is_trivially_copy_constructible_v<T>
    && std::is_trivially_destructible_v<T>;

template <typename T>
using in = std::conditional_t<prefer_pass_by_value<T>, T, T const&>;

}

//-----------------------------------------------------------------------
//  move
//-----------------------------------------------------------------------

inline constexpr auto move(auto&& x) noexcept -> decltype(auto)
{
    return std::move(x);
}

//-----------------------------------------------------------------------
//  to_string
//-----------------------------------------------------------------------

inline auto to_string(auto const& x) -> std::string
{
    if constexpr (std::is_same_v<CPP2_TYPEOF(x), std::string>) {
        return x;
    }
    else if constexpr (std::is_same_v<CPP2_TYPEOF(x), bool>) {
        return x ? "true" : "false";
    }
    else if constexpr (std::is_same_v<CPP2_TYPEOF(x), char>) {
        return std::string(1, x);
    }
    else if constexpr (std::is_convertible_v<CPP2_TYPEOF(x), std::string_view>) {
        return std::string(std::string_view(x));
    }
    else {
        std::ostringstream o;
        o << x;
        return std::move(o).str();
    }
}

//-----------------------------------------------------------------------
//  dependent_false (for UFCS static_assert)
//-----------------------------------------------------------------------

namespace impl {

template <class T> struct dependent_false : std::false_type {};

// as<T> — narrowing/nonsafe cast
template<typename To, typename From>
constexpr auto as(From const& x) -> To
{
    if constexpr (impl::is_narrowing_v<To, From>) {
        auto ret = static_cast<To>(x);
        if (static_cast<From>(ret) != x) {
            throw std::runtime_error("as: narrowing conversion failed");
        }
        return ret;
    }
    else {
        return static_cast<To>(x);
    }
}

}

//-----------------------------------------------------------------------
//  UFCS macros
//-----------------------------------------------------------------------

#define CPP2_UFCS_EMPTY(...)
#define CPP2_UFCS_IDENTITY(...)  __VA_ARGS__
#define CPP2_UFCS_REMPARENS(...) __VA_ARGS__

#define CPP2_UFCS_IS_NOTHROW_PARAM(...)                     /*empty*/
#define CPP2_UFCS_IS_NOTHROW_ARG(MVFWD,QUALID,TEMPKW,...)   /*empty*/
#define CPP2_UFCS_CONSTRAINT_PARAM(...)                     /*empty*/
#define CPP2_UFCS_CONSTRAINT_ARG(MVFWD,QUALID,TEMPKW,...) \
   requires { CPP2_FORWARD(obj).CPP2_UFCS_REMPARENS QUALID TEMPKW __VA_ARGS__(CPP2_FORWARD(params)...); } \
|| requires { MVFWD(CPP2_UFCS_REMPARENS QUALID __VA_ARGS__)(CPP2_FORWARD(obj), CPP2_FORWARD(params)...); }
#if defined(_MSC_VER)
    #undef  CPP2_UFCS_CONSTRAINT_PARAM
    #undef  CPP2_UFCS_CONSTRAINT_ARG
    #define CPP2_UFCS_CONSTRAINT_PARAM(MVFWD,QUALID,TEMPKW,...) , bool IsViable = \
   requires { std::declval<Obj>().CPP2_UFCS_REMPARENS QUALID TEMPKW __VA_ARGS__(std::declval<Params>()...); } \
|| requires { MVFWD(CPP2_UFCS_REMPARENS QUALID __VA_ARGS__)(std::declval<Obj>(), std::declval<Params>()...); }
    #define CPP2_UFCS_CONSTRAINT_ARG(...)                 IsViable
#endif

#define CPP2_UFCS_(LAMBDADEFCAPT,SFINAE,MVFWD,QUALID,TEMPKW,...) \
[LAMBDADEFCAPT]< \
    typename Obj, typename... Params \
    CPP2_UFCS_IS_NOTHROW_PARAM(MVFWD,QUALID,TEMPKW,__VA_ARGS__) \
    CPP2_UFCS_CONSTRAINT_PARAM(MVFWD,QUALID,TEMPKW,__VA_ARGS__) \
  > \
  CPP2_LAMBDA_NO_DISCARD (Obj&& obj, Params&& ...params) CPP2_FORCE_INLINE_LAMBDA_CLANG \
  noexcept(CPP2_UFCS_IS_NOTHROW_ARG(MVFWD,QUALID,TEMPKW,__VA_ARGS__)) CPP2_FORCE_INLINE_LAMBDA -> decltype(auto) \
    SFINAE( requires CPP2_UFCS_CONSTRAINT_ARG(MVFWD,QUALID,TEMPKW,__VA_ARGS__) ) \
  { \
    if constexpr      (requires{ CPP2_FORWARD(obj).CPP2_UFCS_REMPARENS QUALID TEMPKW __VA_ARGS__(CPP2_FORWARD(params)...); }) { \
        return                   CPP2_FORWARD(obj).CPP2_UFCS_REMPARENS QUALID TEMPKW __VA_ARGS__(CPP2_FORWARD(params)...); \
    } \
    else if constexpr (requires{ MVFWD(CPP2_UFCS_REMPARENS QUALID __VA_ARGS__)(CPP2_FORWARD(obj), CPP2_FORWARD(params)...); }) { \
        return                   MVFWD(CPP2_UFCS_REMPARENS QUALID __VA_ARGS__)(CPP2_FORWARD(obj), CPP2_FORWARD(params)...); \
    } \
    else { \
        static_assert( cpp2::impl::dependent_false<Obj>::value, "UFCS: neither method nor free function found" ); \
    } \
  }

#define CPP2_UFCS(...)                                    CPP2_UFCS_(&,CPP2_UFCS_EMPTY,CPP2_UFCS_IDENTITY,(),,__VA_ARGS__)
#define CPP2_UFCS_MOVE(...)                               CPP2_UFCS_(&,CPP2_UFCS_EMPTY,std::move,(),,__VA_ARGS__)
#define CPP2_UFCS_FORWARD(...)                            CPP2_UFCS_(&,CPP2_UFCS_EMPTY,CPP2_FORWARD,(),,__VA_ARGS__)
#define CPP2_UFCS_TEMPLATE(...)                           CPP2_UFCS_(&,CPP2_UFCS_EMPTY,CPP2_UFCS_IDENTITY,(),template,__VA_ARGS__)
#define CPP2_UFCS_NONLOCAL(...)                           CPP2_UFCS_(,CPP2_UFCS_IDENTITY,CPP2_UFCS_IDENTITY,(),,__VA_ARGS__)

#define CPP2_REQUIRES(...) requires (__VA_ARGS__)
#define CPP2_REQUIRES_(...) requires (__VA_ARGS__)

//-----------------------------------------------------------------------
//  Contract stubs
//-----------------------------------------------------------------------

class contract_group {
public:
    using handler = void (*)(const char* msg);
    constexpr contract_group(handler h = {}) : reporter{h} { }
    constexpr auto set_handler(handler h = {}) { reporter = h; }
    constexpr auto is_active() const -> bool { return reporter != handler{}; }
    constexpr auto enforce(bool b, const char* msg = "") -> void { if (!b && reporter) reporter(msg); }
    constexpr auto report_violation(const char* msg = "") -> void { if (reporter) reporter(msg); }
private:
    handler reporter = {};
};

inline auto null_safety   = contract_group();
inline auto bounds_safety = contract_group();
inline auto testing       = contract_group();
inline auto cpp2_default  = contract_group();

// finally — RAII scope exit
template<typename F>
class finally {
    F f;
    bool active;
public:
    finally(F f_) : f(std::move(f_)), active(true) {}
    finally(finally&& other) : f(std::move(other.f)), active(other.active) { other.active = false; }
    ~finally() { if (active) f(); }
    finally(const finally&) = delete;
    auto operator=(const finally&) -> finally& = delete;
};

inline auto make_finally(auto f) -> finally<decltype(f)> {
    return finally<decltype(f)>(std::move(f));
}

// string_util — minimal stubs for @regex
namespace string_util {

template<typename CharT, std::size_t N>
struct fixed_string {
    constexpr fixed_string(const CharT (&s)[N+1]) {
        std::copy_n(s, N + 1, c_str);
    }
    constexpr const CharT* data() const { return c_str; }
    constexpr std::size_t size() const { return N; }
    constexpr auto str() const { return std::basic_string<CharT>(c_str); }
    CharT c_str[N+1];
};

template<typename CharT, std::size_t N>
fixed_string(const CharT (&)[N])->fixed_string<CharT, N-1>;

inline auto replace_all(std::string& s, std::string_view from, std::string_view to) -> void {
    auto pos = std::string::size_type{0};
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}
}

}  // namespace cpp2

#endif // CPP2_CPP2UTIL_H
