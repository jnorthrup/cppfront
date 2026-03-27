#pragma once
// ============================================================================
// BBcursive: Composition-from-Within Rule Pattern
// ============================================================================
//
// Heap-free parser combinator pattern. Key design principles:
//
// 1. FLYWEIGHT TERMINALS: tok(TT::X) produces zero-size types that inline away
// 2. FUNCTION-LOCAL STATICS: Rules initialize once, no per-parse allocation
// 3. STATE THREADING: Parse state flows through without copying
// 4. FEATURE RECORDING: Results recorded to external buffer (stack or arena)
//
// Pattern:
//   inline auto& expression() {
//       static auto r = term() >> *(tok(TT::Plus) >> term());
//       return r;
//   }
//
// Usage:
//   ParseState state{tokens, features_buffer};
//   auto result = translation_unit().parse(state);
//
// ============================================================================

#include "spirit.hpp"

namespace cpp2::parser::bbcursive {

using namespace cpp2::parser::spirit;
using TT = cpp2_transpiler::TokenType;

// ============================================================================
// Parse State (Heap-Free)
// ============================================================================
// Carries mutable state through parsing without allocation.
// Feature buffer is external (stack array or arena).

template<typename FeatureT, std::size_t MaxFeatures = 1024>
struct ParseState {
    TokenStream stream;
    std::size_t feature_count = 0;
    FeatureT* features;  // External buffer - stack or arena allocated
    
    constexpr ParseState(TokenStream s, FeatureT* buf) : stream(s), features(buf) {}
    
    constexpr void record(FeatureT f) {
        if (feature_count < MaxFeatures) features[feature_count++] = f;
    }
    
    constexpr std::span<FeatureT> recorded() const {
        return {features, feature_count};
    }
};

// ============================================================================
// Lazy Rule Wrapper (Zero-Cost)
// ============================================================================
// Wraps a function pointer for deferred rule evaluation.
// No std::function - pure compile-time indirection.

template<typename F>
struct LazyRule {
    F fn;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        return fn().parse(input);
    }
};

template<typename F>
constexpr auto lazy(F fn) { return lift(LazyRule<F>{fn}); }

// ============================================================================
// Semantic Action (Records to External Buffer)
// ============================================================================
// Attaches a recording action to a parser without heap allocation.

template<typename P, typename RecordFn>
struct RecordingParser {
    P parser;
    RecordFn record_fn;
    
    template<typename Input>
    constexpr auto parse(Input input) const {
        auto result = parser.parse(input);
        if (result.success()) record_fn(input, result);
        return result;
    }
};

template<typename P, typename RecordFn>
constexpr auto record(Proto<P> p, RecordFn fn) {
    return lift(RecordingParser<P, RecordFn>{p.parser, fn});
}

} // namespace cpp2::parser::bbcursive