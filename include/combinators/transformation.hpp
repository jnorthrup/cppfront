#ifndef CPP2_COMBINATORS_TRANSFORMATION_HPP
#define CPP2_COMBINATORS_TRANSFORMATION_HPP

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include "structural.hpp"

namespace cpp2::combinators {

// ============================================================================
// Map Iterator - applies function to each element
// ============================================================================

template<typename I, typename F>
struct MapIter {
    I it;
    I end_it;
    F func;

    using input_value_type = typename std::iterator_traits<I>::value_type;
    using value_type = decltype(std::declval<F>()(std::declval<input_value_type>()));
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr MapIter(I i, I e, F f) : it(i), end_it(e), func(f) {}

    constexpr auto operator*() const { return func(*it); }
    
    constexpr MapIter& operator++() {
        if (it != end_it) {
            ++it;
        }
        return *this;
    }

    constexpr bool operator!=(const MapIter& other) const {
        return it != other.it;
    }

    constexpr bool operator==(const MapIter& other) const {
        return it == other.it;
    }
};

// ============================================================================
// Filter Iterator - yields elements matching predicate
// ============================================================================

template<typename I, typename P>
struct FilterIter {
    I it;
    I end_it;
    P pred;

    using value_type = typename std::iterator_traits<I>::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr FilterIter(I i, I e, P p) : it(i), end_it(e), pred(p) {
        find_next();
    }

    constexpr void find_next() {
        while (it != end_it && !pred(*it)) {
            ++it;
        }
    }

    constexpr auto operator*() const { return *it; }
    
    constexpr FilterIter& operator++() {
        if (it != end_it) {
            ++it;
            find_next();
        }
        return *this;
    }

    constexpr bool operator!=(const FilterIter& other) const {
        return it != other.it;
    }

    constexpr bool operator==(const FilterIter& other) const {
        return it == other.it;
    }
};

// ============================================================================
// Enumerate Iterator - yields (index, element) pairs
// ============================================================================

template<typename I>
struct EnumerateIter {
    I it;
    I end_it;
    size_t index;

    using inner_value_type = typename std::iterator_traits<I>::value_type;
    using value_type = std::pair<size_t, inner_value_type>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr EnumerateIter(I i, I e, size_t idx = 0) : it(i), end_it(e), index(idx) {}

    constexpr auto operator*() const { 
        return std::make_pair(index, *it); 
    }
    
    constexpr EnumerateIter& operator++() {
        if (it != end_it) {
            ++it;
            ++index;
        }
        return *this;
    }

    constexpr bool operator!=(const EnumerateIter& other) const {
        return it != other.it;
    }

    constexpr bool operator==(const EnumerateIter& other) const {
        return it == other.it;
    }
};

// ============================================================================
// Zip Iterator - combines two sequences element-wise
// ============================================================================

template<typename I1, typename I2>
struct ZipIter {
    I1 it1;
    I1 end1;
    I2 it2;
    I2 end2;

    using value_type1 = typename std::iterator_traits<I1>::value_type;
    using value_type2 = typename std::iterator_traits<I2>::value_type;
    using value_type = std::pair<value_type1, value_type2>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr ZipIter(I1 i1, I1 e1, I2 i2, I2 e2) 
        : it1(i1), end1(e1), it2(i2), end2(e2) {}

    constexpr auto operator*() const { 
        return std::make_pair(*it1, *it2); 
    }
    
    constexpr ZipIter& operator++() {
        if (it1 != end1 && it2 != end2) {
            ++it1;
            ++it2;
        }
        return *this;
    }

    constexpr bool operator!=(const ZipIter& other) const {
        // End when either sequence ends
        return it1 != other.it1 && it2 != other.it2;
    }

    constexpr bool operator==(const ZipIter& other) const {
        return !(*this != other);
    }
};

// ============================================================================
// Intersperse Iterator - inserts separator between elements
// ============================================================================

template<typename I>
struct IntersperseIter {
    I it;
    I end_it;
    typename std::iterator_traits<I>::value_type separator;
    bool emit_separator;
    bool first;

    using value_type = typename std::iterator_traits<I>::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr IntersperseIter(I i, I e, value_type sep, bool is_end = false) 
        : it(i), end_it(e), separator(sep), emit_separator(false), first(true) {
        if (is_end) {
            it = end_it;
        }
    }

    constexpr auto operator*() const { 
        if (emit_separator) {
            return separator;
        }
        return *it; 
    }
    
    constexpr IntersperseIter& operator++() {
        if (it == end_it) return *this;
        
        if (emit_separator) {
            // We just emitted separator, now move to emit actual element
            emit_separator = false;
        } else {
            // We just emitted element, advance and prepare separator
            ++it;
            if (it != end_it) {
                emit_separator = true;
            }
        }
        return *this;
    }

    constexpr bool operator!=(const IntersperseIter& other) const {
        if (it == end_it && other.it == end_it) return false;
        return it != other.it || emit_separator != other.emit_separator;
    }

    constexpr bool operator==(const IntersperseIter& other) const {
        return !(*this != other);
    }
};

// ============================================================================
// FlatMap Iterator - maps and flattens nested sequences
// ============================================================================

template<typename I, typename F>
struct FlatMapIter {
    I outer_it;
    I outer_end;
    F func;
    
    using OuterValue = typename std::iterator_traits<I>::value_type;
    using InnerRange = decltype(std::declval<F>()(std::declval<OuterValue>()));
    using InnerIter = decltype(std::declval<InnerRange>().begin());
    using value_type = typename std::iterator_traits<InnerIter>::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    std::optional<InnerRange> current_inner;
    InnerIter inner_it;
    InnerIter inner_end;

    FlatMapIter(I outer, I outer_e, F f) 
        : outer_it(outer), outer_end(outer_e), func(f) {
        advance_to_valid();
    }

    void advance_to_valid() {
        while (outer_it != outer_end) {
            current_inner = func(*outer_it);
            inner_it = current_inner->begin();
            inner_end = current_inner->end();
            if (inner_it != inner_end) {
                return; // Found a non-empty inner range
            }
            ++outer_it;
        }
        // Exhausted
        current_inner.reset();
    }

    auto operator*() const { 
        return *inner_it; 
    }
    
    FlatMapIter& operator++() {
        if (!current_inner) return *this;
        
        ++inner_it;
        if (inner_it == inner_end) {
            ++outer_it;
            advance_to_valid();
        }
        return *this;
    }

    bool operator!=(const FlatMapIter& other) const {
        if (outer_it != other.outer_it) return true;
        if (!current_inner && !other.current_inner) return false;
        if (!current_inner || !other.current_inner) return true;
        return inner_it != other.inner_it;
    }

    bool operator==(const FlatMapIter& other) const {
        return !(*this != other);
    }
};

// ============================================================================
// Transformation Range - wraps iterators with chaining methods
// ============================================================================

template<typename Iter>
struct TransformRange {
    Iter m_begin;
    Iter m_end;

    constexpr TransformRange(Iter b, Iter e) : m_begin(b), m_end(e) {}

    constexpr auto begin() const { return m_begin; }
    constexpr auto end() const { return m_end; }

    // Map: apply function to each element
    template<typename F>
    constexpr auto map(F f) const {
        using MI = MapIter<Iter, F>;
        return TransformRange<MI>(MI(m_begin, m_end, f), MI(m_end, m_end, f));
    }

    // Filter: keep elements matching predicate
    template<typename P>
    constexpr auto filter(P p) const {
        using FI = FilterIter<Iter, P>;
        return TransformRange<FI>(FI(m_begin, m_end, p), FI(m_end, m_end, p));
    }

    // Enumerate: add indices
    constexpr auto enumerate() const {
        using EI = EnumerateIter<Iter>;
        return TransformRange<EI>(EI(m_begin, m_end, 0), EI(m_end, m_end, 0));
    }

    // Intersperse: insert separator between elements
    template<typename T>
    constexpr auto intersperse(T sep) const {
        using II = IntersperseIter<Iter>;
        return TransformRange<II>(
            II(m_begin, m_end, sep, false),
            II(m_end, m_end, sep, true)
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

    // First element
    constexpr auto first() const -> std::optional<typename std::iterator_traits<Iter>::value_type> {
        if (m_begin != m_end) {
            return *m_begin;
        }
        return std::nullopt;
    }

    // Last element (requires forward iteration to end)
    auto last() const -> std::optional<typename std::iterator_traits<Iter>::value_type> {
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
};

// ============================================================================
// Combinator Functions
// ============================================================================

// map(f) - applies function to each element
template<typename Range, typename F>
constexpr auto map(const Range& r, F f) {
    using I = decltype(r.begin());
    using MI = MapIter<I, F>;
    return TransformRange<MI>(MI(r.begin(), r.end(), f), MI(r.end(), r.end(), f));
}

// filter(pred) - keeps elements matching predicate
template<typename Range, typename P>
constexpr auto filter(const Range& r, P pred) {
    using I = decltype(r.begin());
    using FI = FilterIter<I, P>;
    return TransformRange<FI>(FI(r.begin(), r.end(), pred), FI(r.end(), r.end(), pred));
}

// enumerate() - adds indices to elements
template<typename Range>
constexpr auto enumerate(const Range& r) {
    using I = decltype(r.begin());
    using EI = EnumerateIter<I>;
    return TransformRange<EI>(EI(r.begin(), r.end(), 0), EI(r.end(), r.end(), 0));
}

// zip(r1, r2) - combines two ranges element-wise
template<typename Range1, typename Range2>
constexpr auto zip(const Range1& r1, const Range2& r2) {
    using I1 = decltype(r1.begin());
    using I2 = decltype(r2.begin());
    using ZI = ZipIter<I1, I2>;
    return TransformRange<ZI>(
        ZI(r1.begin(), r1.end(), r2.begin(), r2.end()),
        ZI(r1.end(), r1.end(), r2.end(), r2.end())
    );
}

// intersperse(r, sep) - inserts separator between elements
template<typename Range, typename T>
constexpr auto intersperse(const Range& r, T sep) {
    using I = decltype(r.begin());
    using II = IntersperseIter<I>;
    return TransformRange<II>(
        II(r.begin(), r.end(), sep, false),
        II(r.end(), r.end(), sep, true)
    );
}

// flat_map(r, f) - maps and flattens
template<typename Range, typename F>
auto flat_map(const Range& r, F f) {
    using I = decltype(r.begin());
    using FMI = FlatMapIter<I, F>;
    return TransformRange<FMI>(
        FMI(r.begin(), r.end(), f),
        FMI(r.end(), r.end(), f)
    );
}

// ============================================================================
// Curried versions for pipeline composition
// ============================================================================

namespace curried {

template<typename F>
struct MapCombinator {
    F func;
    constexpr explicit MapCombinator(F f) : func(f) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return map(r, func);
    }
};

template<typename P>
struct FilterCombinator {
    P pred;
    constexpr explicit FilterCombinator(P p) : pred(p) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return filter(r, pred);
    }
};

struct EnumerateCombinator {
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return enumerate(r);
    }
};

template<typename T>
struct IntersperseCombinator {
    T separator;
    constexpr explicit IntersperseCombinator(T sep) : separator(sep) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return intersperse(r, separator);
    }
};

template<typename Range2>
struct ZipCombinator {
    const Range2& other;
    constexpr explicit ZipCombinator(const Range2& r) : other(r) {}
    
    template<typename Range1>
    constexpr auto operator()(const Range1& r) const {
        return zip(r, other);
    }
};

template<typename F>
struct FlatMapCombinator {
    F func;
    explicit FlatMapCombinator(F f) : func(f) {}
    
    template<typename Range>
    auto operator()(const Range& r) const {
        return flat_map(r, func);
    }
};

// Factory functions
template<typename F>
constexpr auto map(F f) { return MapCombinator<F>(f); }

template<typename P>
constexpr auto filter(P p) { return FilterCombinator<P>(p); }

inline constexpr auto enumerate() { return EnumerateCombinator{}; }

template<typename T>
constexpr auto intersperse(T sep) { return IntersperseCombinator<T>(sep); }

template<typename Range>
constexpr auto zip(const Range& r) { return ZipCombinator<Range>(r); }

template<typename F>
auto flat_map(F f) { return FlatMapCombinator<F>(f); }

} // namespace curried

// ============================================================================
// Helper: Create TransformRange from any range
// ============================================================================

template<typename Range>
constexpr auto from(const Range& r) {
    using I = decltype(r.begin());
    return TransformRange<I>(r.begin(), r.end());
}

} // namespace cpp2::combinators

#endif // CPP2_COMBINATORS_TRANSFORMATION_HPP
