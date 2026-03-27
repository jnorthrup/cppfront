#pragma once
// ============================================================================
// Spirit-Like Parser Combinators for Cpp2
// ============================================================================
// EBNF → C++ Operator Mapping:
//   sequence (a b)      → a >> b
//   alternation (a | b) → a | b  
//   repetition ({ a })  → *a
//   one-or-more (a+)    → +a
//   optional ([ a ])    → -a
//   list (a % sep)      → a % sep
//   char literal 'x'    → implicit via operator
//   string literal "x"  → implicit via operator
// ============================================================================
 

#include "ebnf.hpp"
#include "../lexer.hpp"
#include "../slim_ast.hpp"
#include <span>

namespace cpp2::parser::spirit {

namespace ebnf = cpp2::combinators::ebnf;
using TT = cpp2_transpiler::TokenType;

// ============================================================================
// Token Stream
// ============================================================================

struct TokenStream {
    std::span<const cpp2_transpiler::Token> tokens;
    std::size_t pos = 0;
    
    constexpr TokenStream() = default;
    constexpr explicit TokenStream(std::span<const cpp2_transpiler::Token> t) : tokens(t) {}
    constexpr TokenStream(std::span<const cpp2_transpiler::Token> t, std::size_t p) : tokens(t), pos(p) {}
    
    [[nodiscard]] constexpr bool empty() const {
        return pos >= tokens.size() || tokens[pos].type == TT::EndOfFile;
    }
    [[nodiscard]] constexpr const cpp2_transpiler::Token& peek(std::size_t offset = 0) const {
        return (pos + offset) < tokens.size() ? tokens[pos + offset] : tokens.back();
    }
    constexpr cpp2_transpiler::Token advance() {
        return pos < tokens.size() ? tokens[pos++] : tokens.back();
    }
    constexpr auto begin() const { return tokens.begin() + static_cast<std::ptrdiff_t>(pos); }
    constexpr auto end() const { return tokens.end(); }
    // Return new stream with position advanced by 1 (immutable style for Pratt parser)
    [[nodiscard]] constexpr TokenStream next() const {
        return TokenStream{tokens, pos + 1};
    }
};

// ============================================================================
// Parsers
// ============================================================================

struct TokenParser {
    TT expected;
    constexpr auto parse(TokenStream input) const -> ebnf::Result<cpp2_transpiler::Token, TokenStream> {
        // Special handling for EndOfFile: input.empty() is true for EndOfFile, 
        // preventing standard check from working.
        if (expected == TT::EndOfFile) {
             if (input.peek().type == TT::EndOfFile) { 
                 return ebnf::Result<cpp2_transpiler::Token, TokenStream>::ok(input.peek(), input.next());
             }
             return ebnf::Result<cpp2_transpiler::Token, TokenStream>::fail(input);
        }

        if (input.empty() || input.peek().type != expected)
            return ebnf::Result<cpp2_transpiler::Token, TokenStream>::fail(input);
        return ebnf::Result<cpp2_transpiler::Token, TokenStream>::ok(input.advance(), input);
    }
};

// Matches token by lexeme (for operators/keywords as string literals)
struct LexemeParser {
    std::string_view expected;
    constexpr auto parse(TokenStream input) const -> ebnf::Result<cpp2_transpiler::Token, TokenStream> {
        if (input.empty() || input.peek().lexeme != expected)
            return ebnf::Result<cpp2_transpiler::Token, TokenStream>::fail(input);
        return ebnf::Result<cpp2_transpiler::Token, TokenStream>::ok(input.advance(), input);
    }
};

// ============================================================================
// Proto Wrapper
// ============================================================================

template<typename P>
struct Proto {
    P parser;
    template<typename Input>
    constexpr auto parse(Input input) const { return parser.parse(input); }
    
    template<typename F>
    constexpr auto operator[](F f) const {
        return Proto<decltype(ebnf::map(parser, f))>{ebnf::map(parser, f)};
    }
};

template<typename P> constexpr auto lift(P p) { return Proto<P>{p}; }

// tok(TT::X) - by enum
constexpr auto tok(TT type) { return lift(TokenParser{type}); }

// User-defined literals: "||"_l or '+'_l
constexpr auto operator""_l(const char* s, std::size_t n) { return lift(LexemeParser{std::string_view{s, n}}); }
constexpr auto operator""_l(char c) { return lift(LexemeParser{std::string_view{&c, 1}}); }

// lit() wrapper for compatibility
template<std::size_t N>
constexpr auto lit(const char (&s)[N]) { return lift(LexemeParser{std::string_view{s, N-1}}); }

// ============================================================================
// Operators
// ============================================================================

// Sequence: a >> b
template<typename L, typename R>
constexpr auto operator>>(Proto<L> l, Proto<R> r) {
    return lift(ebnf::discard(ebnf::seq(l.parser, r.parser)));
}

// String literal on right: a >> "+"
template<typename L, std::size_t N>
constexpr auto operator>>(Proto<L> l, const char (&s)[N]) {
    return l >> lift(LexemeParser{std::string_view{s, N-1}});
}

// String literal on left: "(" >> a
template<typename R, std::size_t N>
constexpr auto operator>>(const char (&s)[N], Proto<R> r) {
    return lift(LexemeParser{std::string_view{s, N-1}}) >> r;
}

// Alternative: a | b
template<typename L, typename R>
constexpr auto operator|(Proto<L> l, Proto<R> r) {
    return lift(ebnf::alt(ebnf::discard(l.parser), ebnf::discard(r.parser)));
}

// String literal alternatives: a | "+"
template<typename L, std::size_t N>
constexpr auto operator|(Proto<L> l, const char (&s)[N]) {
    return l | lift(LexemeParser{std::string_view{s, N-1}});
}

template<typename R, std::size_t N>
constexpr auto operator|(const char (&s)[N], Proto<R> r) {
    return lift(LexemeParser{std::string_view{s, N-1}}) | r;
}

// Both string literals: "+" | "-"
template<std::size_t M, std::size_t N>
constexpr auto operator|(const char (&l)[M], const char (&r)[N]) {
    return lift(LexemeParser{std::string_view{l, M-1}}) | lift(LexemeParser{std::string_view{r, N-1}});
}

// List: a % sep
template<typename L, typename R>
constexpr auto operator%(Proto<L> l, Proto<R> r) {
    return lift(ebnf::sep_by(l.parser, r.parser));
}

template<typename L, std::size_t N>
constexpr auto operator%(Proto<L> l, const char (&s)[N]) {
    return l % lift(LexemeParser{std::string_view{s, N-1}});
}

// Zero-or-more: *p
template<typename P>
constexpr auto operator*(Proto<P> p) { return lift(ebnf::many(p.parser)); }

// One-or-more: +p
template<typename P>
constexpr auto operator+(Proto<P> p) { return lift(ebnf::some(p.parser)); }

// Optional: -p (unary)
template<typename P>
constexpr auto operator-(Proto<P> p) { return lift(ebnf::opt(p.parser)); }

// Difference: a - b
template<typename L, typename R>
constexpr auto operator-(Proto<L> l, Proto<R> r) {
    return lift(ebnf::seq_right(ebnf::not_followed_by(r.parser), l.parser));
}


// ============================================================================
// Semantic Actions
// ============================================================================

struct NodeAnnotation { cpp2::ast::NodeKind kind; };
struct BinaryAnnotation { cpp2::ast::NodeKind kind; };
struct PrefixAnnotation { cpp2::ast::NodeKind kind; };
struct PostfixAnnotation { cpp2::ast::NodeKind kind; };
template<typename T> struct TypeHint {};

constexpr auto with_node(cpp2::ast::NodeKind k) { return NodeAnnotation{k}; }
constexpr auto with_binary(cpp2::ast::NodeKind k) { return BinaryAnnotation{k}; }
constexpr auto with_prefix(cpp2::ast::NodeKind k) { return PrefixAnnotation{k}; }
constexpr auto with_postfix(cpp2::ast::NodeKind k) { return PostfixAnnotation{k}; }
template<typename T> constexpr auto ast_node() { return TypeHint<T>{}; }

// Annotated Parser (Wraps P with begin/end)
template<typename P>
struct AnnotatedParser {
    P parser;
    cpp2::ast::NodeKind kind;

    constexpr auto parse(TokenStream input) const {
        auto cp = cpp2::ast::tree_checkpoint();
        auto start_pos = input.pos;
        
        cpp2::ast::begin(kind, start_pos);
        
        auto res = parser.parse(input);
        
        if (res.success()) {
            cpp2::ast::end(res.remaining().pos);
            return res;
        } else {
            cpp2::ast::tree_restore(cp);
            return res;
        }
    }
};

// Binary Annotated Parser (Uses start_infix)
template<typename P>
struct BinaryAnnotatedParser {
    P parser;
    cpp2::ast::NodeKind kind;

    constexpr auto parse(TokenStream input) const {
        auto cp = cpp2::ast::tree_checkpoint();
        auto start_pos = input.pos;
        
        // Use start_infix to adopt previous sibling as Lhs
        cpp2::ast::start_infix(kind, start_pos);
        
        auto res = parser.parse(input);
        
        if (res.success()) {
            cpp2::ast::end(res.remaining().pos);
            return res;
        } else {
            cpp2::ast::tree_restore(cp);
            return res;
        }
    }
};

// Operator % Overloads for Annotations

template<typename P>
constexpr auto operator%(Proto<P> p, NodeAnnotation a) {
    return lift(AnnotatedParser<decltype(p.parser)>{p.parser, a.kind});
}

template<typename P>
constexpr auto operator%(Proto<P> p, BinaryAnnotation a) {
    return lift(BinaryAnnotatedParser<decltype(p.parser)>{p.parser, a.kind});
}

template<typename P>
constexpr auto operator%(Proto<P> p, PrefixAnnotation a) {
    // Prefix currently maps to standard annotation (begin/end)
    return lift(AnnotatedParser<decltype(p.parser)>{p.parser, a.kind});
}

template<typename P>
constexpr auto operator%(Proto<P> p, PostfixAnnotation a) {
    // Postfix currently maps to standard annotation (begin/end)
    return lift(AnnotatedParser<decltype(p.parser)>{p.parser, a.kind});
}

// Ignore Type Hints for now (preserving P)
template<typename P, typename T>
constexpr auto operator%(Proto<P> p, TypeHint<T>) {
    return p;
}

} // namespace cpp2::parser::spirit

 