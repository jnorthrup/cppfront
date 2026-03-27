#ifndef CPP2_COMBINATORS_EBNF_HPP
#define CPP2_COMBINATORS_EBNF_HPP

#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>
#include <tuple>
#include <functional>
#include <utility>

namespace cpp2::combinators::ebnf {

// ============================================================================
// EBNF Combinator Hierarchy - Compile-Time Parser Combinators
// 
// Based on formal EBNF grammar with category-theoretic foundations:
//   - Functor: map/transform
//   - Applicative: sequence/product
//   - Alternative: choice/coproduct
//   - Monad: bind/flatmap
//   - Kleene: repetition (* and +)
// ============================================================================

// ---------------------------------------------------------------------------
// Core Concepts
// ---------------------------------------------------------------------------

template<typename T>
concept ParseResult = requires(T t) {
    { t.success() } -> std::convertible_to<bool>;
    { t.value() };
    { t.remaining() };
};

template<typename P, typename Input>
concept Parser = requires(P p, Input input) {
    { p.parse(input) } -> ParseResult;
};

// ---------------------------------------------------------------------------
// Result Types
// ---------------------------------------------------------------------------

template<typename T, typename Input>
struct Success {
    T val;
    Input rest;
    
    constexpr bool success() const { return true; }
    constexpr const T& value() const { return val; }
    constexpr T& value() { return val; }
    constexpr const Input& remaining() const { return rest; }
};

template<typename Input>
struct Failure {
    Input rest;
    const char* error = nullptr;
    
    constexpr bool success() const { return false; }
    constexpr const Input& remaining() const { return rest; }
};

template<typename T, typename Input>
struct Result {
    std::variant<Success<T, Input>, Failure<Input>> data;
    
    constexpr bool success() const { 
        return std::holds_alternative<Success<T, Input>>(data); 
    }
    
    constexpr const T& value() const { 
        return std::get<Success<T, Input>>(data).val; 
    }
    
    constexpr T& value() { 
        return std::get<Success<T, Input>>(data).val; 
    }
    
    constexpr const Input& remaining() const {
        if (success()) {
            return std::get<Success<T, Input>>(data).rest;
        }
        return std::get<Failure<Input>>(data).rest;
    }
    
    static constexpr Result ok(T val, Input rest) {
        return Result{Success<T, Input>{std::move(val), rest}};
    }
    
    static constexpr Result fail(Input rest, const char* err = nullptr) {
        return Result{Failure<Input>{rest, err}};
    }
};

// ---------------------------------------------------------------------------
// Level 0: Terminal Parsers (Atoms)
// ---------------------------------------------------------------------------

// Match a specific token type
template<typename TokenType>
struct Token {
    TokenType expected;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = decltype(*input.begin());
        if (input.empty() || input.peek().type != expected) {
            return Result<T, Input>::fail(input);
        }
        auto tok = input.advance();
        return Result<T, Input>::ok(tok, input);
    }
};

// Match any token satisfying predicate
template<typename Pred>
struct Satisfy {
    Pred predicate;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = decltype(*input.begin());
        if (input.empty() || !predicate(input.peek())) {
            return Result<T, Input>::fail(input);
        }
        auto tok = input.advance();
        return Result<T, Input>::ok(tok, input);
    }
};

// Match end of input
struct Eof {
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::monostate;
        if (!input.empty()) {
            return Result<T, Input>::fail(input);
        }
        return Result<T, Input>::ok(std::monostate{}, input);
    }
};

// Always succeed without consuming input
template<typename T>
struct Pure {
    T value;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        return Result<T, Input>::ok(value, input);
    }
};

// Always fail
struct Fail {
    const char* message = "parse failed";
    
    template<typename Input, typename T = std::monostate>
    constexpr auto parse(Input input) const {
        return Result<T, Input>::fail(input, message);
    }
};

// ---------------------------------------------------------------------------
// Level 1: Functor (map/transform)
// ---------------------------------------------------------------------------

template<typename P, typename F>
struct Map {
    P parser;
    F func;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto result = parser.parse(input);
        if (!result.success()) {
            using R = decltype(func(result.value()));
            return Result<R, Input>::fail(result.remaining());
        }
        return Result<decltype(func(result.value())), Input>::ok(
            func(result.value()), result.remaining());
    }
};

template<typename P, typename F>
constexpr auto map(P parser, F func) {
    return Map<P, F>{std::move(parser), std::move(func)};
}

// Operator syntax: parser % func
template<typename P, typename F>
constexpr auto operator%(P parser, F func) {
    return map(std::move(parser), std::move(func));
}

// ---------------------------------------------------------------------------
// Level 2: Applicative (sequence/product)
// ---------------------------------------------------------------------------

// Sequence two parsers, keeping both results
template<typename P1, typename P2>
struct Seq {
    P1 first;
    P2 second;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r1 = first.parse(input);
        if (!r1.success()) {
            using T = std::pair<decltype(r1.value()), decltype(second.parse(input).value())>;
            return Result<T, Input>::fail(r1.remaining());
        }
        auto r2 = second.parse(r1.remaining());
        if (!r2.success()) {
            using T = std::pair<decltype(r1.value()), decltype(r2.value())>;
            return Result<T, Input>::fail(input);  // Backtrack
        }
        using T = std::pair<decltype(r1.value()), decltype(r2.value())>;
        return Result<T, Input>::ok({r1.value(), r2.value()}, r2.remaining());
    }
};

// Sequence, keeping only left result
template<typename P1, typename P2>
struct SeqL {
    P1 first;
    P2 second;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r1 = first.parse(input);
        if (!r1.success()) {
            return r1;
        }
        auto r2 = second.parse(r1.remaining());
        if (!r2.success()) {
            using T = decltype(r1.value());
            return Result<T, Input>::fail(input);
        }
        using T = decltype(r1.value());
        return Result<T, Input>::ok(r1.value(), r2.remaining());
    }
};

// Sequence, keeping only right result
template<typename P1, typename P2>
struct SeqR {
    P1 first;
    P2 second;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r1 = first.parse(input);
        if (!r1.success()) {
            using T = decltype(second.parse(input).value());
            return Result<T, Input>::fail(r1.remaining());
        }
        auto r2 = second.parse(r1.remaining());
        if (!r2.success()) {
            using T = decltype(r2.value());
            return Result<T, Input>::fail(input);
        }
        return r2;
    }
};

template<typename P1, typename P2>
constexpr auto seq(P1 p1, P2 p2) { return Seq<P1, P2>{std::move(p1), std::move(p2)}; }

template<typename P1, typename P2>
constexpr auto seq_left(P1 p1, P2 p2) { return SeqL<P1, P2>{std::move(p1), std::move(p2)}; }

template<typename P1, typename P2>
constexpr auto seq_right(P1 p1, P2 p2) { return SeqR<P1, P2>{std::move(p1), std::move(p2)}; }

// Operators: >> for seq, << for seq_left, >> for seq_right
template<typename P1, typename P2>
constexpr auto operator>>(P1 p1, P2 p2) { return seq(std::move(p1), std::move(p2)); }

template<typename P1, typename P2>
constexpr auto operator<<(P1 p1, P2 p2) { return seq_left(std::move(p1), std::move(p2)); }

// ---------------------------------------------------------------------------
// Level 3: Alternative (choice/coproduct)
// ---------------------------------------------------------------------------

template<typename P1, typename P2>
struct Alt {
    P1 first;
    P2 second;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r1 = first.parse(input);
        if (r1.success()) {
            return r1;
        }
        return second.parse(input);
    }
};

template<typename P1, typename P2>
constexpr auto alt(P1 p1, P2 p2) { return Alt<P1, P2>{std::move(p1), std::move(p2)}; }

// Operator: | for alternative
template<typename P1, typename P2>
constexpr auto operator|(P1 p1, P2 p2) { return alt(std::move(p1), std::move(p2)); }

// ---------------------------------------------------------------------------
// Level 4: Kleene Combinators (repetition)
// ---------------------------------------------------------------------------

// Zero or more (Kleene star: *)
template<typename P>
struct Many {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::remove_cvref_t<decltype(parser.parse(input).value())>;
        std::vector<T> results;
        
        while (true) {
            auto r = parser.parse(input);
            if (!r.success()) break;
            results.push_back(std::move(r.value()));
            input = r.remaining();
        }
        
        return Result<std::vector<T>, Input>::ok(std::move(results), input);
    }
};

// One or more (Kleene plus: +)
template<typename P>
struct Some {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::remove_cvref_t<decltype(parser.parse(input).value())>;
        
        auto first = parser.parse(input);
        if (!first.success()) {
            return Result<std::vector<T>, Input>::fail(input);
        }
        
        std::vector<T> results;
        results.push_back(std::move(first.value()));
        input = first.remaining();
        
        while (true) {
            auto r = parser.parse(input);
            if (!r.success()) break;
            results.push_back(std::move(r.value()));
            input = r.remaining();
        }
        
        return Result<std::vector<T>, Input>::ok(std::move(results), input);
    }
};

// Optional (?)
template<typename P>
struct Opt {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::remove_cvref_t<decltype(parser.parse(input).value())>;
        auto r = parser.parse(input);
        if (r.success()) {
            return Result<std::optional<T>, Input>::ok(std::optional<T>(std::move(r.value())), r.remaining());
        }
        return Result<std::optional<T>, Input>::ok(std::nullopt, input);
    }
};

template<typename P>
constexpr auto many(P p) { return Many<P>{std::move(p)}; }

template<typename P>
constexpr auto some(P p) { return Some<P>{std::move(p)}; }

template<typename P>
constexpr auto opt(P p) { return Opt<P>{std::move(p)}; }

// Operator syntax: *parser for many, +parser for some, -parser for optional
template<typename P>
constexpr auto operator*(P p) { return many(std::move(p)); }

template<typename P>
constexpr auto operator+(P p) { return some(std::move(p)); }

template<typename P>
constexpr auto operator-(P p) { return opt(std::move(p)); }

// ---------------------------------------------------------------------------
// Level 5: Separated Lists (common EBNF pattern)
// ---------------------------------------------------------------------------

// item (sep item)* - list separated by delimiter
template<typename P, typename Sep>
struct SepBy {
    P parser;
    Sep separator;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::remove_cvref_t<decltype(parser.parse(input).value())>;
        std::vector<T> results;
        
        auto first = parser.parse(input);
        if (!first.success()) {
            return Result<std::vector<T>, Input>::ok(std::move(results), input);
        }
        
        results.push_back(std::move(first.value()));
        input = first.remaining();
        
        while (true) {
            auto sep_result = separator.parse(input);
            if (!sep_result.success()) break;
            
            auto item_result = parser.parse(sep_result.remaining());
            if (!item_result.success()) break;
            
            results.push_back(std::move(item_result.value()));
            input = item_result.remaining();
        }
        
        return Result<std::vector<T>, Input>::ok(std::move(results), input);
    }
};

// item (sep item)* [sep] - allows trailing separator
template<typename P, typename Sep>
struct SepByTrailing {
    P parser;
    Sep separator;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        using T = std::remove_cvref_t<decltype(parser.parse(input).value())>;
        std::vector<T> results;
        
        auto first = parser.parse(input);
        if (!first.success()) {
            return Result<std::vector<T>, Input>::ok(std::move(results), input);
        }
        
        results.push_back(std::move(first.value()));
        input = first.remaining();
        
        while (true) {
            auto sep_result = separator.parse(input);
            if (!sep_result.success()) break;
            
            auto item_result = parser.parse(sep_result.remaining());
            if (!item_result.success()) {
                // Trailing separator - accept it
                input = sep_result.remaining();
                break;
            }
            
            results.push_back(std::move(item_result.value()));
            input = item_result.remaining();
        }
        
        return Result<std::vector<T>, Input>::ok(std::move(results), input);
    }
};

// sep item+ sep - delimited content between markers
template<typename P, typename Open, typename Close>
struct Between {
    Open open;
    P parser;
    Close close;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto o = open.parse(input);
        if (!o.success()) {
            using T = decltype(parser.parse(input).value());
            return Result<T, Input>::fail(input);
        }
        
        auto content = parser.parse(o.remaining());
        if (!content.success()) {
            using T = decltype(content.value());
            return Result<T, Input>::fail(input);
        }
        
        auto c = close.parse(content.remaining());
        if (!c.success()) {
            using T = decltype(content.value());
            return Result<T, Input>::fail(input);
        }
        
        using T = decltype(content.value());
        return Result<T, Input>::ok(std::move(content.value()), c.remaining());
    }
};

template<typename P, typename Sep>
constexpr auto sep_by(P p, Sep s) { return SepBy<P, Sep>{std::move(p), std::move(s)}; }

template<typename P, typename Sep>
constexpr auto sep_by_trailing(P p, Sep s) { return SepByTrailing<P, Sep>{std::move(p), std::move(s)}; }

template<typename Open, typename P, typename Close>
constexpr auto between(Open o, P p, Close c) { 
    return Between<P, Open, Close>{std::move(o), std::move(p), std::move(c)}; 
}

// ---------------------------------------------------------------------------
// Level 6: Monad (bind/flatmap for context-sensitive parsing)
// ---------------------------------------------------------------------------

template<typename P, typename F>
struct Bind {
    P parser;
    F func;  // T -> Parser<U>
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (!r.success()) {
            using NextParser = decltype(func(r.value()));
            using U = decltype(NextParser{}.parse(input).value());
            return Result<U, Input>::fail(r.remaining());
        }
        auto next_parser = func(r.value());
        return next_parser.parse(r.remaining());
    }
};

template<typename P, typename F>
constexpr auto bind(P p, F f) { return Bind<P, F>{std::move(p), std::move(f)}; }

// Operator: >>= for bind
template<typename P, typename F>
constexpr auto operator>>=(P p, F f) { return bind(std::move(p), std::move(f)); }

// ---------------------------------------------------------------------------
// Level 7: Lookahead & Error Handling
// ---------------------------------------------------------------------------

// Positive lookahead (doesn't consume input)
template<typename P>
struct Lookahead {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (r.success()) {
            return Result<std::monostate, Input>::ok(std::monostate{}, input);
        }
        return Result<std::monostate, Input>::fail(input);
    }
};

// Negative lookahead (succeeds if parser fails)
template<typename P>
struct NotFollowedBy {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (r.success()) {
            return Result<std::monostate, Input>::fail(input);
        }
        return Result<std::monostate, Input>::ok(std::monostate{}, input);
    }
};

// Try parser with backtracking on failure
template<typename P>
struct Try {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (!r.success()) {
            using T = decltype(r.value());
            return Result<T, Input>::fail(input);  // Return to original position
        }
        return r;
    }
};

// Label parser for better error messages
template<typename P>
struct Label {
    P parser;
    const char* name;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (!r.success()) {
            using T = decltype(r.value());
            return Result<T, Input>::fail(r.remaining(), name);
        }
        return r;
    }
};

template<typename P>
constexpr auto lookahead(P p) { return Lookahead<P>{std::move(p)}; }

template<typename P>
constexpr auto not_followed_by(P p) { return NotFollowedBy<P>{std::move(p)}; }

template<typename P>
constexpr auto try_(P p) { return Try<P>{std::move(p)}; }

template<typename P>
constexpr auto label(P p, const char* name) { return Label<P>{std::move(p), name}; }

// ---------------------------------------------------------------------------
// Level 8: Recursive Parsers (fix-point)
// ---------------------------------------------------------------------------

// Lazy parser for recursion - uses std::function for type erasure
template<typename T, typename Input>
struct Lazy {
    std::function<Result<T, Input>(Input)> parser_fn;
    
    constexpr auto parse(Input input) const {
        return parser_fn(input);
    }
};

// Create a recursive parser using a factory function
template<typename T, typename Input, typename F>
constexpr auto fix(F factory) {
    // Factory receives a reference to the parser being defined
    Lazy<T, Input> self;
    self.parser_fn = [&self, factory](Input input) {
        return factory(self).parse(input);
    };
    return self;
}

// ---------------------------------------------------------------------------
// Level 9: Discard - Drops parsed value, returns std::monostate
// ---------------------------------------------------------------------------
// This is crucial for making alternatives work when parsers return different types

template<typename P>
struct Discard {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (r.success()) {
            return Result<std::monostate, Input>::ok(std::monostate{}, r.remaining());
        }
        return Result<std::monostate, Input>::fail(input);
    }
};

template<typename P>
constexpr auto discard(P p) { return Discard<P>{std::move(p)}; }

// ---------------------------------------------------------------------------
// Level 10: Rule - Type-erased parser for recursive grammars
// ---------------------------------------------------------------------------
// Stores any parser via std::function, always returns std::monostate

template<typename Input>
struct Rule {
    std::function<Result<std::monostate, Input>(Input)> parser_fn;
    
    auto parse(Input input) const -> Result<std::monostate, Input> {
        if (!parser_fn) {
            return Result<std::monostate, Input>::fail(input, "Rule not initialized");
        }
        return parser_fn(input);
    }
    
    template<typename P>
    void set(P&& parser) {
        parser_fn = [p = std::forward<P>(parser)](Input input) -> Result<std::monostate, Input> {
            auto result = p.parse(input);
            if (result.success()) {
                return Result<std::monostate, Input>::ok(std::monostate{}, result.remaining());
            }
            return Result<std::monostate, Input>::fail(input);
        };
    }
};


// ---------------------------------------------------------------------------
// EBNF DSL Helpers
// ---------------------------------------------------------------------------

// Create token parser
template<typename T>
constexpr auto token(T type) { return Token<T>{type}; }

// Create predicate parser
template<typename P>
constexpr auto satisfy(P pred) { return Satisfy<P>{std::move(pred)}; }

// Pure value
template<typename T>
constexpr auto pure(T val) { return Pure<T>{std::move(val)}; }

// End of input
inline constexpr Eof eof{};

// Failure
inline constexpr Fail fail{};

// ---------------------------------------------------------------------------
// Utility: Tuple/Variadic Sequences
// ---------------------------------------------------------------------------

// Sequence multiple parsers
template<typename... Ps>
struct SeqN;

template<typename P>
struct SeqN<P> {
    P parser;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r = parser.parse(input);
        if (!r.success()) {
            return Result<std::tuple<decltype(r.value())>, Input>::fail(input);
        }
        return Result<std::tuple<decltype(r.value())>, Input>::ok(
            std::make_tuple(std::move(r.value())), r.remaining());
    }
};

template<typename P, typename... Rest>
struct SeqN<P, Rest...> {
    P first;
    SeqN<Rest...> rest;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto r1 = first.parse(input);
        if (!r1.success()) {
            using T = decltype(std::tuple_cat(
                std::make_tuple(r1.value()),
                rest.parse(input).value()));
            return Result<T, Input>::fail(input);
        }
        
        auto r2 = rest.parse(r1.remaining());
        if (!r2.success()) {
            using T = decltype(std::tuple_cat(
                std::make_tuple(r1.value()),
                r2.value()));
            return Result<T, Input>::fail(input);
        }
        
        auto combined = std::tuple_cat(
            std::make_tuple(std::move(r1.value())),
            std::move(r2.value()));
        return Result<decltype(combined), Input>::ok(std::move(combined), r2.remaining());
    }
};

template<typename... Ps>
constexpr auto seq_all(Ps... ps) { return SeqN<Ps...>{std::move(ps)...}; }

// Choice among multiple alternatives
template<typename P, typename... Rest>
constexpr auto choice(P first, Rest... rest) {
    if constexpr (sizeof...(rest) == 0) {
        return first;
    } else {
        return alt(std::move(first), choice(std::move(rest)...));
    }
}

} // namespace cpp2::combinators::ebnf

#endif // CPP2_COMBINATORS_EBNF_HPP
