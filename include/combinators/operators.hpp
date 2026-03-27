#ifndef CPP2_COMBINATORS_OPERATORS_HPP
#define CPP2_COMBINATORS_OPERATORS_HPP

#include "ebnf.hpp"

namespace cpp2::parser::operators {

namespace ebnf = cpp2::combinators::ebnf;

// ---------------------------------------------------------------------------
// Proto Wrapper
// ---------------------------------------------------------------------------
// Wraps any EBNF parser to provide:
// 1. Spirit-like member operators (operator[])
// 2. Strict ADL isolation (prevents conflicts with ebnf.hpp operators)
// 3. Chainable operator grammar

template<typename P>
struct Proto {
    P parser;
    
    // Forward parse call to underlying combinator
    template<typename Input>
    constexpr auto parse(Input input) const {
        return parser.parse(input);
    }
    
    // Transform: p[f]
    // Spirit-style semantic action
    template<typename F>
    constexpr auto operator[](F f) const {
        auto m = ebnf::map(parser, std::move(f));
        using M = decltype(m);
        return Proto<M>{std::move(m)};
    }
};

// Helper: Lift an EBNF parser into Proto
template<typename P>
constexpr auto lift(P p) {
    return Proto<P>{std::move(p)};
}

// ---------------------------------------------------------------------------
// Operator Overloads (Proto -> Proto)
// ---------------------------------------------------------------------------

// Sequence: a >> b
template<typename L, typename R>
constexpr auto operator>>(Proto<L> l, Proto<R> r) {
    return lift(ebnf::seq(std::move(l.parser), std::move(r.parser)));
}

// Alternative: a | b
template<typename L, typename R>
constexpr auto operator|(Proto<L> l, Proto<R> r) {
    return lift(ebnf::alt(std::move(l.parser), std::move(r.parser)));
}

// List: a % b (sep_by)
// Spirit style list operator
template<typename L, typename R>
constexpr auto operator%(Proto<L> l, Proto<R> r) {
    return lift(ebnf::sep_by(std::move(l.parser), std::move(r.parser)));
}

// Difference: a - b
// Spirit: Matches a but not b (a - b)
template<typename L, typename R>
constexpr auto operator-(Proto<L> l, Proto<R> r) {
    // !b >> a (check b doesn't match, then match a)
    // using seq_right to discard the lookahead result
    return lift(ebnf::seq_right(ebnf::not_followed_by(std::move(r.parser)), std::move(l.parser)));
}

// Zero-or-more: *p
template<typename P>
constexpr auto operator*(Proto<P> p) {
    return lift(ebnf::many(std::move(p.parser)));
}

// One-or-more: +p
template<typename P>
constexpr auto operator+(Proto<P> p) {
    return lift(ebnf::some(std::move(p.parser)));
}

// Optional: -p
template<typename P>
constexpr auto operator-(Proto<P> p) {
    return lift(ebnf::opt(std::move(p.parser)));
}

// ---------------------------------------------------------------------------
// Factories (Lifted)
// ---------------------------------------------------------------------------

template<typename T>
constexpr auto token(T t) {
    return lift(ebnf::token(std::move(t)));
}

// Allow lifting arbitrary parsers
template<typename P>
constexpr auto rule(P p) {
    return lift(std::move(p));
}

// Recursive rule helper
// Wraps ebnf::fix but ensures the passed builder receives a Proto
template<typename T, typename Input, typename F>
constexpr auto recursive_rule(F factory) {
    return lift(ebnf::fix<T, Input>([factory](auto self) {
        // self is ebnf::Lazy. We need to wrap it? 
        // No, factory returns a parser. We expect factory to return a Proto.
        // But ebnf::fix expects factory to return ebnf parser.
        // Adapter needed.
        return factory(lift(self)).parser;
    }));
}

} // namespace cpp2::parser::operators

#endif // CPP2_COMBINATORS_OPERATORS_HPP
