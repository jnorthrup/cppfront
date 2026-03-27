// lexer.hpp — Stub for combinator stack compatibility
#ifndef CPP2_LEXER_HPP
#define CPP2_LEXER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace cpp2::lexer {

enum class token_type : std::uint8_t {
    identifier, keyword, literal, operator_,
    lparen, rparen, lbrace, rbrace, lbracket, rbracket,
    semicolon, comma, colon, dot, arrow,
    string_literal, char_literal, number,
    comment, whitespace, newline, EndOfFile, unknown
};

}

// Compatibility namespace for combinator stack
namespace cpp2_transpiler {

using TokenType = cpp2::lexer::token_type;

struct Token {
    TokenType type = TokenType::unknown;
    std::string_view value;
    std::size_t line = 0;
    std::size_t col = 0;
    std::string_view lexeme;  // token text for spirit.hpp compat
};

}

#endif
