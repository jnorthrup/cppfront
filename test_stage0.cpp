#include "include/cpp2util_stage0.h"
#include <vector>
#include <string>
#include <array>

// Test all the symbols the compiler needs

void test() {
    // Integer typedefs
    cpp2::i32 x = 42;
    cpp2::u8 y = 255;
    cpp2::i64 z = 100;

    // CPP2_FORWARD
    auto fwd = CPP2_FORWARD(x);

    // CPP2_TYPEOF
    CPP2_TYPEOF(x) x2 = x;

    // CPP2_COPY
    auto x3 = CPP2_COPY(x);

    // cpp2::move
    auto m = cpp2::move(x);

    // cpp2::to_string
    auto s = cpp2::to_string(x);
    auto s2 = cpp2::to_string(std::string("hello"));
    auto s3 = cpp2::to_string("world");

    // cpp2::unchecked_narrow
    auto n = cpp2::unchecked_narrow<cpp2::u8>(10);

    // cpp2::unchecked_cast
    auto c = cpp2::unchecked_cast<int>(x);

    // CPP2_ASSERT_IN_BOUNDS
    std::vector<int> v = {1, 2, 3};
    auto& elem = CPP2_ASSERT_IN_BOUNDS(v, 0);

    // CPP2_ASSERT_IN_BOUNDS_LITERAL
    std::array<int, 3> a = {1, 2, 3};
    auto& elem2 = CPP2_ASSERT_IN_BOUNDS_LITERAL(a, 1);

    // impl::cmp_less / impl::cmp_greater
    auto lt = cpp2::impl::cmp_less(x, z);
    auto gt = cpp2::impl::cmp_greater(x, z);

    // unchecked_cmp_less etc
    auto ult = cpp2::unchecked_cmp_less(x, z);
    auto ugt = cpp2::unchecked_cmp_greater(x, z);

    // in<T> (available as cpp2::in or cpp2::impl::in)
    cpp2::in<std::string> str_ref = s;
    cpp2::impl::in<int> int_ref = x;

    // CPP2_CONSTEXPR
    CPP2_CONSTEXPR int ci = 5;

    // CPP2_UFCS_TEMPLATE — verify it's defined
    // (Cannot test easily without proper Cpp2 context)

    // dependent_false — used by UFCS static_assert
    // (Just verify it compiles as a type)
    static_assert(!cpp2::impl::dependent_false<int>::value, "");
}
