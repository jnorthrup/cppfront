#ifndef CPP2_COMBINATORS_REDUCTION_HPP
#define CPP2_COMBINATORS_REDUCTION_HPP

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include "transformation.hpp"

namespace cpp2::combinators {

// ============================================================================
// Scan Iterator - yields running accumulation values
// ============================================================================

template<typename I, typename T, typename F>
struct ScanIter {
    I it;
    I end_it;
    T acc;
    F func;
    bool first;

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const T*;
    using reference = T;

    constexpr ScanIter(I i, I e, T init, F f, bool is_end = false) 
        : it(i), end_it(e), acc(init), func(f), first(true) {
        if (is_end) {
            it = end_it;
        }
    }

    constexpr auto operator*() const { return acc; }
    
    constexpr ScanIter& operator++() {
        if (it == end_it) return *this;
        
        if (first) {
            first = false;
        }
        ++it;
        if (it != end_it) {
            acc = func(acc, *it);
        }
        return *this;
    }

    constexpr bool operator!=(const ScanIter& other) const {
        return it != other.it;
    }

    constexpr bool operator==(const ScanIter& other) const {
        return it == other.it;
    }
};

// ============================================================================
// Reduction Range - wraps iterators with reduction methods
// ============================================================================

template<typename Iter>
struct ReductionRange {
    Iter m_begin;
    Iter m_end;

    constexpr ReductionRange(Iter b, Iter e) : m_begin(b), m_end(e) {}

    constexpr auto begin() const { return m_begin; }
    constexpr auto end() const { return m_end; }

    // ========================================================================
    // Terminal Operations (consume the range)
    // ========================================================================

    // fold(init, f) - left fold with initial value
    template<typename T, typename F>
    constexpr T fold(T init, F f) const {
        T acc = init;
        for (auto it = m_begin; it != m_end; ++it) {
            acc = f(acc, *it);
        }
        return acc;
    }

    // reduce(f) - fold using first element as init
    template<typename F>
    constexpr auto reduce(F f) const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin == m_end) {
            return std::nullopt;
        }
        auto it = m_begin;
        auto acc = *it;
        ++it;
        while (it != m_end) {
            acc = f(acc, *it);
            ++it;
        }
        return acc;
    }

    // find(pred) - first element matching predicate
    template<typename P>
    constexpr auto find(P pred) const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        for (auto it = m_begin; it != m_end; ++it) {
            if (pred(*it)) {
                return *it;
            }
        }
        return std::nullopt;
    }

    // find_index(pred) - index of first matching element
    template<typename P>
    constexpr auto find_index(P pred) const -> std::optional<size_t> {
        size_t idx = 0;
        for (auto it = m_begin; it != m_end; ++it, ++idx) {
            if (pred(*it)) {
                return idx;
            }
        }
        return std::nullopt;
    }

    // all(pred) - true if all elements match
    template<typename P>
    constexpr bool all(P pred) const {
        for (auto it = m_begin; it != m_end; ++it) {
            if (!pred(*it)) {
                return false;
            }
        }
        return true;
    }

    // any(pred) - true if any element matches
    template<typename P>
    constexpr bool any(P pred) const {
        for (auto it = m_begin; it != m_end; ++it) {
            if (pred(*it)) {
                return true;
            }
        }
        return false;
    }

    // none(pred) - true if no element matches
    template<typename P>
    constexpr bool none(P pred) const {
        return !any(pred);
    }

    // count(pred) - count elements matching predicate
    template<typename P>
    constexpr size_t count(P pred) const {
        size_t cnt = 0;
        for (auto it = m_begin; it != m_end; ++it) {
            if (pred(*it)) {
                ++cnt;
            }
        }
        return cnt;
    }

    // count_all() - count all elements
    constexpr size_t count_all() const {
        size_t cnt = 0;
        for (auto it = m_begin; it != m_end; ++it) {
            ++cnt;
        }
        return cnt;
    }

    // first() - first element
    constexpr auto first() const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin != m_end) {
            return *m_begin;
        }
        return std::nullopt;
    }

    // last() - last element (requires forward iteration)
    auto last() const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin == m_end) {
            return std::nullopt;
        }
        auto it = m_begin;
        auto prev = it;
        while (it != m_end) {
            prev = it;
            ++it;
        }
        return *prev;
    }

    // nth(n) - element at index n
    constexpr auto nth(size_t n) const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        size_t idx = 0;
        for (auto it = m_begin; it != m_end; ++it, ++idx) {
            if (idx == n) {
                return *it;
            }
        }
        return std::nullopt;
    }

    // sum() - sum all elements (requires + operator)
    constexpr auto sum() const -> typename std::iterator_traits<Iter>::value_type {
        using T = typename std::iterator_traits<Iter>::value_type;
        return fold(T{}, [](T a, T b) { return a + b; });
    }

    // product() - multiply all elements (requires * operator)
    constexpr auto product() const -> typename std::iterator_traits<Iter>::value_type {
        using T = typename std::iterator_traits<Iter>::value_type;
        if (m_begin == m_end) return T{};
        auto it = m_begin;
        T acc = *it;
        ++it;
        while (it != m_end) {
            acc = acc * (*it);
            ++it;
        }
        return acc;
    }

    // min() - minimum element
    constexpr auto min() const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin == m_end) return std::nullopt;
        auto it = m_begin;
        auto result = *it;
        ++it;
        while (it != m_end) {
            if (*it < result) {
                result = *it;
            }
            ++it;
        }
        return result;
    }

    // max() - maximum element
    constexpr auto max() const 
        -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin == m_end) return std::nullopt;
        auto it = m_begin;
        auto result = *it;
        ++it;
        while (it != m_end) {
            if (*it > result) {
                result = *it;
            }
            ++it;
        }
        return result;
    }

    // ========================================================================
    // Lazy Operations (return new ranges)
    // ========================================================================

    // scan(init, f) - running accumulation
    template<typename T, typename F>
    constexpr auto scan(T init, F f) const {
        using SI = ScanIter<Iter, T, F>;
        // Compute initial accumulated value
        T initial_acc = init;
        if (m_begin != m_end) {
            initial_acc = f(init, *m_begin);
        }
        return ReductionRange<SI>(
            SI(m_begin, m_end, initial_acc, f, false),
            SI(m_end, m_end, init, f, true)
        );
    }

    // Collect to container
    template<typename Container>
    Container collect() const {
        Container c;
        for (auto it = begin(); it != end(); ++it) {
            c.push_back(*it);
        }
        return c;
    }
};

// ============================================================================
// Free Functions
// ============================================================================

// fold(r, init, f)
template<typename Range, typename T, typename F>
constexpr T fold(const Range& r, T init, F f) {
    T acc = init;
    for (auto it = r.begin(); it != r.end(); ++it) {
        acc = f(acc, *it);
    }
    return acc;
}

// reduce(r, f)
template<typename Range, typename F>
constexpr auto reduce(const Range& r, F f) 
    -> std::optional<typename std::iterator_traits<decltype(r.begin())>::value_type> {
    auto it = r.begin();
    auto end = r.end();
    if (it == end) {
        return std::nullopt;
    }
    auto acc = *it;
    ++it;
    while (it != end) {
        acc = f(acc, *it);
        ++it;
    }
    return acc;
}

// scan(r, init, f)
template<typename Range, typename T, typename F>
constexpr auto scan(const Range& r, T init, F f) {
    using I = decltype(r.begin());
    using SI = ScanIter<I, T, F>;
    T initial_acc = init;
    if (r.begin() != r.end()) {
        initial_acc = f(init, *r.begin());
    }
    return ReductionRange<SI>(
        SI(r.begin(), r.end(), initial_acc, f, false),
        SI(r.end(), r.end(), init, f, true)
    );
}

// find(r, pred)
template<typename Range, typename P>
constexpr auto find(const Range& r, P pred) 
    -> std::optional<typename std::iterator_traits<decltype(r.begin())>::value_type> {
    for (auto it = r.begin(); it != r.end(); ++it) {
        if (pred(*it)) {
            return *it;
        }
    }
    return std::nullopt;
}

// find_index(r, pred)
template<typename Range, typename P>
constexpr auto find_index(const Range& r, P pred) -> std::optional<size_t> {
    size_t idx = 0;
    for (auto it = r.begin(); it != r.end(); ++it, ++idx) {
        if (pred(*it)) {
            return idx;
        }
    }
    return std::nullopt;
}

// all(r, pred)
template<typename Range, typename P>
constexpr bool all(const Range& r, P pred) {
    for (auto it = r.begin(); it != r.end(); ++it) {
        if (!pred(*it)) {
            return false;
        }
    }
    return true;
}

// any(r, pred)
template<typename Range, typename P>
constexpr bool any(const Range& r, P pred) {
    for (auto it = r.begin(); it != r.end(); ++it) {
        if (pred(*it)) {
            return true;
        }
    }
    return false;
}

// none(r, pred)
template<typename Range, typename P>
constexpr bool none(const Range& r, P pred) {
    return !any(r, pred);
}

// count(r, pred)
template<typename Range, typename P>
constexpr size_t count(const Range& r, P pred) {
    size_t cnt = 0;
    for (auto it = r.begin(); it != r.end(); ++it) {
        if (pred(*it)) {
            ++cnt;
        }
    }
    return cnt;
}

// ============================================================================
// Curried versions for pipeline composition
// ============================================================================

namespace curried {

template<typename T, typename F>
struct FoldCombinator {
    T init;
    F func;
    constexpr FoldCombinator(T i, F f) : init(i), func(f) {}
    
    template<typename Range>
    constexpr T operator()(const Range& r) const {
        return fold(r, init, func);
    }
};

template<typename F>
struct ReduceCombinator {
    F func;
    constexpr explicit ReduceCombinator(F f) : func(f) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return reduce(r, func);
    }
};

template<typename T, typename F>
struct ScanCombinator {
    T init;
    F func;
    constexpr ScanCombinator(T i, F f) : init(i), func(f) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return scan(r, init, func);
    }
};

template<typename P>
struct FindCombinator {
    P pred;
    constexpr explicit FindCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return find(r, pred);
    }
};

template<typename P>
struct FindIndexCombinator {
    P pred;
    constexpr explicit FindIndexCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return find_index(r, pred);
    }
};

template<typename P>
struct AllCombinator {
    P pred;
    constexpr explicit AllCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr bool operator()(const Range& r) const {
        return all(r, pred);
    }
};

template<typename P>
struct AnyCombinator {
    P pred;
    constexpr explicit AnyCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr bool operator()(const Range& r) const {
        return any(r, pred);
    }
};

template<typename P>
struct NoneCombinator {
    P pred;
    constexpr explicit NoneCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr bool operator()(const Range& r) const {
        return none(r, pred);
    }
};

template<typename P>
struct CountCombinator {
    P pred;
    constexpr explicit CountCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr size_t operator()(const Range& r) const {
        return count(r, pred);
    }
};

// Factory functions
template<typename T, typename F>
constexpr auto fold(T init, F f) { return FoldCombinator<T, F>(init, f); }

template<typename F>
constexpr auto reduce(F f) { return ReduceCombinator<F>(f); }

template<typename T, typename F>
constexpr auto scan(T init, F f) { return ScanCombinator<T, F>(init, f); }

template<typename P>
constexpr auto find(P p) { return FindCombinator<P>(p); }

template<typename P>
constexpr auto find_index(P p) { return FindIndexCombinator<P>(p); }

template<typename P>
constexpr auto all(P p) { return AllCombinator<P>(p); }

template<typename P>
constexpr auto any(P p) { return AnyCombinator<P>(p); }

template<typename P>
constexpr auto none(P p) { return NoneCombinator<P>(p); }

template<typename P>
constexpr auto count(P p) { return CountCombinator<P>(p); }

} // namespace curried

// ============================================================================
// Helper: Create ReductionRange from any range
// ============================================================================

template<typename Range>
constexpr auto reduce_from(const Range& r) {
    using I = decltype(r.begin());
    return ReductionRange<I>(r.begin(), r.end());
}

} // namespace cpp2::combinators

#endif // CPP2_COMBINATORS_REDUCTION_HPP
