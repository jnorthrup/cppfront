#ifndef CPP2_COMBINATORS_PARSING_HPP
#define CPP2_COMBINATORS_PARSING_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <utility>
#include <algorithm>
#include "../bytebuffer.hpp"

namespace cpp2::combinators {

// ============================================================================
// ParseResult - wraps parsed value and remaining input
// ============================================================================

template<typename T>
struct ParseResult {
    T value;
    ByteBuffer remaining;
    
    constexpr ParseResult(T v, ByteBuffer r) : value(v), remaining(r) {}
    
    // Allow chaining parsers
    template<typename F>
    constexpr auto and_then(F f) const {
        return f(remaining);
    }
};

// ============================================================================
// Basic Byte Parsers
// ============================================================================

// byte() - consume single byte
inline auto byte(const ByteBuffer& buf) -> std::optional<ParseResult<uint8_t>> {
    if (buf.empty()) {
        return std::nullopt;
    }
    return ParseResult<uint8_t>(
        static_cast<uint8_t>(buf.data()[0]),
        ByteBuffer(buf.data() + 1, buf.size() - 1)
    );
}

// bytes(n) - consume exactly n bytes
inline auto bytes(const ByteBuffer& buf, size_t n) -> std::optional<ParseResult<ByteBuffer>> {
    if (buf.size() < n) {
        return std::nullopt;
    }
    return ParseResult<ByteBuffer>(
        ByteBuffer(buf.data(), n),
        ByteBuffer(buf.data() + n, buf.size() - n)
    );
}

// peek() - look at next byte without consuming
inline auto peek(const ByteBuffer& buf) -> std::optional<uint8_t> {
    if (buf.empty()) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(buf.data()[0]);
}

// skip(n) - skip n bytes
inline auto skip(const ByteBuffer& buf, size_t n) -> std::optional<ByteBuffer> {
    if (buf.size() < n) {
        return std::nullopt;
    }
    return ByteBuffer(buf.data() + n, buf.size() - n);
}

// ============================================================================
// Conditional Parsers
// ============================================================================

// until(delimiter) - consume until delimiter byte found
inline auto until(const ByteBuffer& buf, uint8_t delimiter) 
    -> std::optional<ParseResult<ByteBuffer>> {
    for (size_t i = 0; i < buf.size(); ++i) {
        if (static_cast<uint8_t>(buf.data()[i]) == delimiter) {
            return ParseResult<ByteBuffer>(
                ByteBuffer(buf.data(), i),
                ByteBuffer(buf.data() + i + 1, buf.size() - i - 1)
            );
        }
    }
    return std::nullopt;  // Delimiter not found
}

// until_seq(seq) - consume until byte sequence found
inline auto until_seq(const ByteBuffer& buf, const ByteBuffer& seq) 
    -> std::optional<ParseResult<ByteBuffer>> {
    if (seq.empty()) {
        return ParseResult<ByteBuffer>(ByteBuffer(buf.data(), 0), buf);
    }
    if (buf.size() < seq.size()) {
        return std::nullopt;
    }
    
    for (size_t i = 0; i <= buf.size() - seq.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < seq.size(); ++j) {
            if (buf.data()[i + j] != seq.data()[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return ParseResult<ByteBuffer>(
                ByteBuffer(buf.data(), i),
                ByteBuffer(buf.data() + i + seq.size(), buf.size() - i - seq.size())
            );
        }
    }
    return std::nullopt;
}

// while_pred(pred) - consume while predicate holds
template<typename P>
auto while_pred(const ByteBuffer& buf, P pred) -> ParseResult<ByteBuffer> {
    size_t i = 0;
    while (i < buf.size() && pred(static_cast<uint8_t>(buf.data()[i]))) {
        ++i;
    }
    return ParseResult<ByteBuffer>(
        ByteBuffer(buf.data(), i),
        ByteBuffer(buf.data() + i, buf.size() - i)
    );
}

// take_while(pred) - alias for while_pred
template<typename P>
auto take_while(const ByteBuffer& buf, P pred) -> ParseResult<ByteBuffer> {
    return while_pred(buf, pred);
}

// ============================================================================
// Endian Integer Parsers
// ============================================================================

// Little-endian readers
inline auto le_u16(const ByteBuffer& buf) -> std::optional<ParseResult<uint16_t>> {
    if (buf.size() < 2) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint16_t v = static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    return ParseResult<uint16_t>(v, ByteBuffer(buf.data() + 2, buf.size() - 2));
}

inline auto le_u32(const ByteBuffer& buf) -> std::optional<ParseResult<uint32_t>> {
    if (buf.size() < 4) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint32_t v = static_cast<uint32_t>(p[0]) |
                 (static_cast<uint32_t>(p[1]) << 8) |
                 (static_cast<uint32_t>(p[2]) << 16) |
                 (static_cast<uint32_t>(p[3]) << 24);
    return ParseResult<uint32_t>(v, ByteBuffer(buf.data() + 4, buf.size() - 4));
}

inline auto le_u64(const ByteBuffer& buf) -> std::optional<ParseResult<uint64_t>> {
    if (buf.size() < 8) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint64_t v = static_cast<uint64_t>(p[0]) |
                 (static_cast<uint64_t>(p[1]) << 8) |
                 (static_cast<uint64_t>(p[2]) << 16) |
                 (static_cast<uint64_t>(p[3]) << 24) |
                 (static_cast<uint64_t>(p[4]) << 32) |
                 (static_cast<uint64_t>(p[5]) << 40) |
                 (static_cast<uint64_t>(p[6]) << 48) |
                 (static_cast<uint64_t>(p[7]) << 56);
    return ParseResult<uint64_t>(v, ByteBuffer(buf.data() + 8, buf.size() - 8));
}

// Big-endian readers
inline auto be_u16(const ByteBuffer& buf) -> std::optional<ParseResult<uint16_t>> {
    if (buf.size() < 2) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint16_t v = (static_cast<uint16_t>(p[0]) << 8) | static_cast<uint16_t>(p[1]);
    return ParseResult<uint16_t>(v, ByteBuffer(buf.data() + 2, buf.size() - 2));
}

inline auto be_u32(const ByteBuffer& buf) -> std::optional<ParseResult<uint32_t>> {
    if (buf.size() < 4) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint32_t v = (static_cast<uint32_t>(p[0]) << 24) |
                 (static_cast<uint32_t>(p[1]) << 16) |
                 (static_cast<uint32_t>(p[2]) << 8) |
                 static_cast<uint32_t>(p[3]);
    return ParseResult<uint32_t>(v, ByteBuffer(buf.data() + 4, buf.size() - 4));
}

inline auto be_u64(const ByteBuffer& buf) -> std::optional<ParseResult<uint64_t>> {
    if (buf.size() < 8) return std::nullopt;
    const auto* p = reinterpret_cast<const uint8_t*>(buf.data());
    uint64_t v = (static_cast<uint64_t>(p[0]) << 56) |
                 (static_cast<uint64_t>(p[1]) << 48) |
                 (static_cast<uint64_t>(p[2]) << 40) |
                 (static_cast<uint64_t>(p[3]) << 32) |
                 (static_cast<uint64_t>(p[4]) << 24) |
                 (static_cast<uint64_t>(p[5]) << 16) |
                 (static_cast<uint64_t>(p[6]) << 8) |
                 static_cast<uint64_t>(p[7]);
    return ParseResult<uint64_t>(v, ByteBuffer(buf.data() + 8, buf.size() - 8));
}

// Signed variants (reinterpret unsigned)
inline auto le_i16(const ByteBuffer& buf) -> std::optional<ParseResult<int16_t>> {
    auto r = le_u16(buf);
    if (!r) return std::nullopt;
    return ParseResult<int16_t>(static_cast<int16_t>(r->value), r->remaining);
}

inline auto le_i32(const ByteBuffer& buf) -> std::optional<ParseResult<int32_t>> {
    auto r = le_u32(buf);
    if (!r) return std::nullopt;
    return ParseResult<int32_t>(static_cast<int32_t>(r->value), r->remaining);
}

inline auto le_i64(const ByteBuffer& buf) -> std::optional<ParseResult<int64_t>> {
    auto r = le_u64(buf);
    if (!r) return std::nullopt;
    return ParseResult<int64_t>(static_cast<int64_t>(r->value), r->remaining);
}

inline auto be_i16(const ByteBuffer& buf) -> std::optional<ParseResult<int16_t>> {
    auto r = be_u16(buf);
    if (!r) return std::nullopt;
    return ParseResult<int16_t>(static_cast<int16_t>(r->value), r->remaining);
}

inline auto be_i32(const ByteBuffer& buf) -> std::optional<ParseResult<int32_t>> {
    auto r = be_u32(buf);
    if (!r) return std::nullopt;
    return ParseResult<int32_t>(static_cast<int32_t>(r->value), r->remaining);
}

inline auto be_i64(const ByteBuffer& buf) -> std::optional<ParseResult<int64_t>> {
    auto r = be_u64(buf);
    if (!r) return std::nullopt;
    return ParseResult<int64_t>(static_cast<int64_t>(r->value), r->remaining);
}

// ============================================================================
// String Parsers
// ============================================================================

// c_str() - null-terminated string
inline auto c_str(const ByteBuffer& buf) -> std::optional<ParseResult<ByteBuffer>> {
    return until(buf, 0);  // Find null terminator
}

// pascal_string() - length-prefixed string (1-byte length)
inline auto pascal_string(const ByteBuffer& buf) -> std::optional<ParseResult<ByteBuffer>> {
    auto len_result = byte(buf);
    if (!len_result) return std::nullopt;
    
    size_t len = len_result->value;
    if (len_result->remaining.size() < len) return std::nullopt;
    
    return ParseResult<ByteBuffer>(
        ByteBuffer(len_result->remaining.data(), len),
        ByteBuffer(len_result->remaining.data() + len, len_result->remaining.size() - len)
    );
}

// pascal_string16() - length-prefixed string (2-byte length, little-endian)
inline auto pascal_string16_le(const ByteBuffer& buf) -> std::optional<ParseResult<ByteBuffer>> {
    auto len_result = le_u16(buf);
    if (!len_result) return std::nullopt;
    
    size_t len = len_result->value;
    if (len_result->remaining.size() < len) return std::nullopt;
    
    return ParseResult<ByteBuffer>(
        ByteBuffer(len_result->remaining.data(), len),
        ByteBuffer(len_result->remaining.data() + len, len_result->remaining.size() - len)
    );
}

// fixed_string(n) - fixed-length string
inline auto fixed_string(const ByteBuffer& buf, size_t n) -> std::optional<ParseResult<ByteBuffer>> {
    return bytes(buf, n);
}

// ============================================================================
// Validation Predicates
// ============================================================================

// length_eq(n) - check if buffer has exactly n bytes
inline bool length_eq(const ByteBuffer& buf, size_t n) {
    return buf.size() == n;
}

// length_ge(n) - check if buffer has at least n bytes
inline bool length_ge(const ByteBuffer& buf, size_t n) {
    return buf.size() >= n;
}

// length_le(n) - check if buffer has at most n bytes  
inline bool length_le(const ByteBuffer& buf, size_t n) {
    return buf.size() <= n;
}

// length_between(min, max) - check if length is in range [min, max]
inline bool length_between(const ByteBuffer& buf, size_t min_len, size_t max_len) {
    return buf.size() >= min_len && buf.size() <= max_len;
}

// starts_with(prefix) - check if buffer starts with prefix
inline bool starts_with(const ByteBuffer& buf, const ByteBuffer& prefix) {
    if (buf.size() < prefix.size()) return false;
    return std::memcmp(buf.data(), prefix.data(), prefix.size()) == 0;
}

// ends_with(suffix) - check if buffer ends with suffix
inline bool ends_with(const ByteBuffer& buf, const ByteBuffer& suffix) {
    if (buf.size() < suffix.size()) return false;
    return std::memcmp(buf.data() + buf.size() - suffix.size(), suffix.data(), suffix.size()) == 0;
}

// contains(element) - check if buffer contains byte
inline bool contains(const ByteBuffer& buf, uint8_t element) {
    for (size_t i = 0; i < buf.size(); ++i) {
        if (static_cast<uint8_t>(buf.data()[i]) == element) {
            return true;
        }
    }
    return false;
}

// contains_seq(seq) - check if buffer contains sequence
inline bool contains_seq(const ByteBuffer& buf, const ByteBuffer& seq) {
    if (seq.empty()) return true;
    if (buf.size() < seq.size()) return false;
    
    for (size_t i = 0; i <= buf.size() - seq.size(); ++i) {
        if (std::memcmp(buf.data() + i, seq.data(), seq.size()) == 0) {
            return true;
        }
    }
    return false;
}

// is_sorted() - check if bytes are in ascending order
inline bool is_sorted(const ByteBuffer& buf) {
    for (size_t i = 1; i < buf.size(); ++i) {
        if (static_cast<uint8_t>(buf.data()[i]) < static_cast<uint8_t>(buf.data()[i - 1])) {
            return false;
        }
    }
    return true;
}

// is_unique() - check if all bytes are unique
inline bool is_unique(const ByteBuffer& buf) {
    // Simple O(n²) for small buffers
    for (size_t i = 0; i < buf.size(); ++i) {
        for (size_t j = i + 1; j < buf.size(); ++j) {
            if (buf.data()[i] == buf.data()[j]) {
                return false;
            }
        }
    }
    return true;
}

// all_of(pred) - check if all bytes satisfy predicate
template<typename P>
bool all_of(const ByteBuffer& buf, P pred) {
    for (size_t i = 0; i < buf.size(); ++i) {
        if (!pred(static_cast<uint8_t>(buf.data()[i]))) {
            return false;
        }
    }
    return true;
}

// any_of(pred) - check if any byte satisfies predicate
template<typename P>
bool any_of(const ByteBuffer& buf, P pred) {
    for (size_t i = 0; i < buf.size(); ++i) {
        if (pred(static_cast<uint8_t>(buf.data()[i]))) {
            return true;
        }
    }
    return false;
}

// none_of(pred) - check if no byte satisfies predicate
template<typename P>
bool none_of(const ByteBuffer& buf, P pred) {
    return !any_of(buf, pred);
}

// ============================================================================
// Curried Versions for Pipeline Composition
// ============================================================================

namespace curried {

// bytes(n)
struct BytesCombinator {
    size_t n;
    constexpr explicit BytesCombinator(size_t sz) : n(sz) {}
    
    auto operator()(const ByteBuffer& buf) const {
        return bytes(buf, n);
    }
};

inline auto bytes(size_t n) { return BytesCombinator(n); }

// until(delimiter)
struct UntilCombinator {
    uint8_t delimiter;
    constexpr explicit UntilCombinator(uint8_t d) : delimiter(d) {}
    
    auto operator()(const ByteBuffer& buf) const {
        return until(buf, delimiter);
    }
};

inline auto until(uint8_t delimiter) { return UntilCombinator(delimiter); }

// while_pred(pred)
template<typename P>
struct WhilePredCombinator {
    P pred;
    explicit WhilePredCombinator(P p) : pred(p) {}
    
    auto operator()(const ByteBuffer& buf) const {
        return while_pred(buf, pred);
    }
};

template<typename P>
auto while_pred(P p) { return WhilePredCombinator<P>(p); }

// Note: skip(n) combinator is in structural.hpp to avoid redefinition

// Validation curried versions
struct LengthEqCombinator {
    size_t n;
    constexpr explicit LengthEqCombinator(size_t sz) : n(sz) {}
    
    bool operator()(const ByteBuffer& buf) const {
        return length_eq(buf, n);
    }
};

inline auto length_eq(size_t n) { return LengthEqCombinator(n); }

struct LengthBetweenCombinator {
    size_t min_len;
    size_t max_len;
    constexpr LengthBetweenCombinator(size_t mn, size_t mx) : min_len(mn), max_len(mx) {}
    
    bool operator()(const ByteBuffer& buf) const {
        return length_between(buf, min_len, max_len);
    }
};

inline auto length_between(size_t min_len, size_t max_len) { 
    return LengthBetweenCombinator(min_len, max_len); 
}

struct StartsWithCombinator {
    ByteBuffer prefix;
    explicit StartsWithCombinator(const ByteBuffer& p) : prefix(p) {}
    
    bool operator()(const ByteBuffer& buf) const {
        return starts_with(buf, prefix);
    }
};

inline auto starts_with(const ByteBuffer& prefix) { return StartsWithCombinator(prefix); }

struct EndsWithCombinator {
    ByteBuffer suffix;
    explicit EndsWithCombinator(const ByteBuffer& s) : suffix(s) {}
    
    bool operator()(const ByteBuffer& buf) const {
        return ends_with(buf, suffix);
    }
};

inline auto ends_with(const ByteBuffer& suffix) { return EndsWithCombinator(suffix); }

struct ContainsCombinator {
    uint8_t element;
    constexpr explicit ContainsCombinator(uint8_t e) : element(e) {}
    
    bool operator()(const ByteBuffer& buf) const {
        return contains(buf, element);
    }
};

inline auto contains(uint8_t element) { return ContainsCombinator(element); }

} // namespace curried

} // namespace cpp2::combinators

#endif // CPP2_COMBINATORS_PARSING_HPP
