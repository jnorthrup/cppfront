#ifndef CPP2_COMBINATORS_STRUCTURAL_HPP
#define CPP2_COMBINATORS_STRUCTURAL_HPP

#include <cstddef>
#include <iterator>
#include "../bytebuffer.hpp"
#include "../lazy_iterator.hpp"

namespace cpp2::combinators {

// ============================================================================
// Take Iterator - yields first N elements
// ============================================================================

template<typename I>
struct TakeIter {
    I it;
    I end_it;
    size_t remaining;

    using value_type = typename std::iterator_traits<I>::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr TakeIter(I i, I e, size_t n) : it(i), end_it(e), remaining(n) {}

    constexpr auto operator*() const { return *it; }
    
    constexpr TakeIter& operator++() {
        if (remaining > 0 && it != end_it) {
            ++it;
            --remaining;
        }
        return *this;
    }

    constexpr bool operator!=(const TakeIter& other) const {
        return (remaining > 0 && it != end_it) && (other.remaining == 0 || other.it != it);
    }

    constexpr bool operator==(const TakeIter& other) const {
        return !(*this != other);
    }
};

// ============================================================================
// Skip Iterator - yields elements after first N
// ============================================================================

template<typename I>
struct SkipIter {
    I it;
    I end_it;

    using value_type = typename std::iterator_traits<I>::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const value_type*;
    using reference = value_type;

    constexpr SkipIter(I i, I e, size_t n) : it(i), end_it(e) {
        // Skip n elements during construction
        for (size_t j = 0; j < n && it != end_it; ++j) {
            ++it;
        }
    }

    // End sentinel constructor
    constexpr SkipIter(I e) : it(e), end_it(e) {}

    constexpr auto operator*() const { return *it; }
    
    constexpr SkipIter& operator++() {
        if (it != end_it) {
            ++it;
        }
        return *this;
    }

    constexpr bool operator!=(const SkipIter& other) const {
        return it != other.it;
    }

    constexpr bool operator==(const SkipIter& other) const {
        return it == other.it;
    }
};

// ============================================================================
// Split Iterator - yields sub-ranges split by delimiter
// For ByteBuffer, yields ByteBuffer slices (zero-copy)
// ============================================================================

struct SplitIter {
    const char* current;
    const char* end;
    const char* chunk_start;
    const char* chunk_end;
    char delimiter;

    using value_type = ByteBuffer;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const ByteBuffer*;
    using reference = ByteBuffer;

    constexpr SplitIter(const char* start, const char* e, char delim) 
        : current(start), end(e), chunk_start(start), chunk_end(start), delimiter(delim) {
        find_chunk();
    }

    // End sentinel
    constexpr SplitIter(const char* e) 
        : current(e), end(e), chunk_start(e), chunk_end(e), delimiter('\0') {}

    constexpr void find_chunk() {
        chunk_start = current;
        chunk_end = current;
        while (chunk_end != end && *chunk_end != delimiter) {
            ++chunk_end;
        }
    }

    constexpr ByteBuffer operator*() const {
        return ByteBuffer(chunk_start, static_cast<size_t>(chunk_end - chunk_start));
    }

    constexpr SplitIter& operator++() {
        if (chunk_end != end) {
            current = chunk_end + 1; // Skip delimiter
        } else {
            current = end;
        }
        find_chunk();
        return *this;
    }

    constexpr bool operator!=(const SplitIter& other) const {
        // We're at end when current == end AND we've yielded the last chunk
        if (current == end && chunk_start == end) {
            return false; // At sentinel
        }
        return current != other.current || chunk_start != other.chunk_start;
    }

    constexpr bool operator==(const SplitIter& other) const {
        return !(*this != other);
    }
};

// ============================================================================
// Chunk Iterator - yields fixed-size blocks
// ============================================================================

struct ChunkIter {
    const char* current;
    const char* end;
    size_t chunk_size;

    using value_type = ByteBuffer;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const ByteBuffer*;
    using reference = ByteBuffer;

    constexpr ChunkIter(const char* start, const char* e, size_t size)
        : current(start), end(e), chunk_size(size) {}

    constexpr ByteBuffer operator*() const {
        size_t remaining = static_cast<size_t>(end - current);
        size_t actual_size = remaining < chunk_size ? remaining : chunk_size;
        return ByteBuffer(current, actual_size);
    }

    constexpr ChunkIter& operator++() {
        size_t remaining = static_cast<size_t>(end - current);
        size_t advance = remaining < chunk_size ? remaining : chunk_size;
        current += advance;
        return *this;
    }

    constexpr bool operator!=(const ChunkIter& other) const {
        return current != other.current;
    }

    constexpr bool operator==(const ChunkIter& other) const {
        return current == other.current;
    }
};

// ============================================================================
// Window Iterator - yields overlapping sliding windows
// ============================================================================

struct WindowIter {
    const char* current;
    const char* end;
    size_t window_size;

    using value_type = ByteBuffer;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = const ByteBuffer*;
    using reference = ByteBuffer;

    constexpr WindowIter(const char* start, const char* e, size_t size)
        : current(start), end(e), window_size(size) {
        // If not enough elements for a window, immediately go to end
        if (static_cast<size_t>(end - current) < window_size) {
            current = end;
        }
    }

    constexpr ByteBuffer operator*() const {
        return ByteBuffer(current, window_size);
    }

    constexpr WindowIter& operator++() {
        ++current;
        // Check if we still have enough for a full window
        if (static_cast<size_t>(end - current) < window_size) {
            current = end;
        }
        return *this;
    }

    constexpr bool operator!=(const WindowIter& other) const {
        return current != other.current;
    }

    constexpr bool operator==(const WindowIter& other) const {
        return current == other.current;
    }
};

// ============================================================================
// Lazy Range Wrappers
// ============================================================================

template<typename Iter>
struct StructuralRange {
    Iter m_begin;
    Iter m_end;

    constexpr StructuralRange(Iter b, Iter e) : m_begin(b), m_end(e) {}

    constexpr auto begin() const { return m_begin; }
    constexpr auto end() const { return m_end; }

    // Collect to vector
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
// Combinator Functions (return lazy ranges)
// ============================================================================

// take(n) - returns lazy range of first n elements
template<typename Range>
constexpr auto take(const Range& r, size_t n) {
    using I = decltype(r.begin());
    using TI = TakeIter<I>;
    return StructuralRange<TI>(
        TI(r.begin(), r.end(), n),
        TI(r.end(), r.end(), 0)
    );
}

// skip(n) - returns lazy range skipping first n elements
template<typename Range>
constexpr auto skip(const Range& r, size_t n) {
    using I = decltype(r.begin());
    using SI = SkipIter<I>;
    return StructuralRange<SI>(
        SI(r.begin(), r.end(), n),
        SI(r.end())
    );
}

// slice(start, end) - returns lazy range from start to end
template<typename Range>
constexpr auto slice(const Range& r, size_t start, size_t end_idx) {
    // slice = skip(start) |> take(end - start)
    auto skipped = skip(r, start);
    return take(skipped, end_idx > start ? end_idx - start : 0);
}

// split(delimiter) - returns lazy range of ByteBuffer slices
inline constexpr auto split(const ByteBuffer& buf, char delimiter) {
    return StructuralRange<SplitIter>(
        SplitIter(buf.data(), buf.data() + buf.size(), delimiter),
        SplitIter(buf.data() + buf.size())
    );
}

// chunk(size) - returns lazy range of fixed-size ByteBuffer chunks
inline constexpr auto chunk(const ByteBuffer& buf, size_t size) {
    return StructuralRange<ChunkIter>(
        ChunkIter(buf.data(), buf.data() + buf.size(), size),
        ChunkIter(buf.data() + buf.size(), buf.data() + buf.size(), size)
    );
}

// window(size) - returns lazy range of overlapping ByteBuffer windows
inline constexpr auto window(const ByteBuffer& buf, size_t size) {
    return StructuralRange<WindowIter>(
        WindowIter(buf.data(), buf.data() + buf.size(), size),
        WindowIter(buf.data() + buf.size(), buf.data() + buf.size(), size)
    );
}

// ============================================================================
// Curried versions for pipeline composition
// ============================================================================

namespace curried {

struct TakeCombinator {
    size_t n;
    constexpr explicit TakeCombinator(size_t count) : n(count) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return take(r, n);
    }
};

struct SkipCombinator {
    size_t n;
    constexpr explicit SkipCombinator(size_t count) : n(count) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return skip(r, n);
    }
};

struct SliceCombinator {
    size_t start;
    size_t end_idx;
    constexpr SliceCombinator(size_t s, size_t e) : start(s), end_idx(e) {}
    
    template<typename Range>
    constexpr auto operator()(const Range& r) const {
        return slice(r, start, end_idx);
    }
};

struct SplitCombinator {
    char delimiter;
    constexpr explicit SplitCombinator(char d) : delimiter(d) {}
    
    constexpr auto operator()(const ByteBuffer& buf) const {
        return split(buf, delimiter);
    }
};

struct ChunkCombinator {
    size_t size;
    constexpr explicit ChunkCombinator(size_t s) : size(s) {}
    
    constexpr auto operator()(const ByteBuffer& buf) const {
        return chunk(buf, size);
    }
};

struct WindowCombinator {
    size_t size;
    constexpr explicit WindowCombinator(size_t s) : size(s) {}
    
    constexpr auto operator()(const ByteBuffer& buf) const {
        return window(buf, size);
    }
};

// Factory functions
inline constexpr auto take(size_t n) { return TakeCombinator(n); }
inline constexpr auto skip(size_t n) { return SkipCombinator(n); }
inline constexpr auto slice(size_t start, size_t end) { return SliceCombinator(start, end); }
inline constexpr auto split(char d) { return SplitCombinator(d); }
inline constexpr auto chunk(size_t s) { return ChunkCombinator(s); }
inline constexpr auto window(size_t s) { return WindowCombinator(s); }

} // namespace curried

// ============================================================================
// Pipe operator for composition
// ============================================================================

template<typename Range, typename Combinator>
constexpr auto operator|(const Range& r, Combinator&& c) {
    return c(r);
}

} // namespace cpp2::combinators

#endif // CPP2_COMBINATORS_STRUCTURAL_HPP
