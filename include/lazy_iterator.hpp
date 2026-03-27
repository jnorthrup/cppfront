// lazy_iterator.hpp — Stub for combinator stack compatibility
#ifndef CPP2_LAZY_ITERATOR_HPP
#define CPP2_LAZY_ITERATOR_HPP

#include <iterator>
#include <functional>

namespace cpp2 {

template<typename T, typename F>
class lazy_iterator {
    T current;
    F advance_fn;
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    lazy_iterator(T init, F fn) : current(std::move(init)), advance_fn(std::move(fn)) {}
    auto operator*() const -> const T& { return current; }
    auto operator++() -> lazy_iterator& { advance_fn(current); return *this; }
    auto operator==(std::default_sentinel_t) const -> bool { return false; }
};

}
#endif
