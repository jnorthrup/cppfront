// bytebuffer.hpp — Stub for combinator stack compatibility
#ifndef CPP2_BYTEBUFFER_HPP
#define CPP2_BYTEBUFFER_HPP

#include <span>
#include <string>
#include <string_view>
#include <cstdint>

namespace cpp2 {

using ByteBuffer = std::span<const std::uint8_t>;

inline auto to_bytebuffer(std::string_view s) -> ByteBuffer {
    return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
}

}
#endif
