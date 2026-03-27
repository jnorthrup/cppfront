# Conductor Tracks — Cpp2 Pure Self-Hosting

## Goal
Rewrite cppfront in Cpp2 (UFCS-free, zero runtime dependency) using EBNF combinator stack for parsing. Then output sea of nodes IR.

## Current state
- Compiler: ~26K lines Cpp1 across 6 files (cppfront.cpp, to_cpp1.h, sema.h, parse.h, lex.h, io.h)
- Combinator stack: 3313 lines Cpp2 in /Users/jim/work/cppfort/include/combinators/
- reflect.h2: 8752 lines Cpp2 (needs rewrite to UFCS-free Cpp2)

## Track 1: Integrate combinator stack into cppfront
- **Status:** OPEN
- Copy combinator headers into cppfront/include/combinators/
- Verify they compile with cppfront's build
- Create include/combinators.hpp umbrella header
- Test: compiler builds with combinator headers included

## Track 2: Write Cpp2 emitter (string output)
- **Status:** PENDING
- New file: source/emit.h (or emit.h2)
- Takes parse tree nodes → emits Cpp1 text strings
- Replaces to_cpp1.h's emit() functions
- Start with: function declarations, type declarations, expressions

## Track 3: Replace parse.h with combinator-based parser
- **Status:** PENDING
- parse.h (10995 lines) → combinator-based parsing
- EBNF grammar drives parsing, not hand-written recursive descent
- Parse tree nodes match combinator output types

## Track 4: Rewrite remaining files in Cpp2 (UFCS-free)
- **Status:** PENDING
- io.h → io.h2
- lex.h → lex.h2
- sema.h → sema.h2
- to_cpp1.h → emit.h2 (from Track 2)
- cppfront.cpp → cppfront.cpp2
- reflect.h2 → rewrite without UFCS

## Track 5: Self-hosting verification
- **Status:** PENDING
- Compiler compiles itself
- Regression tests pass
- Output matches current behavior

## Track 6: Sea of nodes IR
- **Status:** PENDING
- Change emitter from string output to node graph
- Enable optimizations (constant folding, DCE, etc.)
- Codegen from node graph to Cpp1 (or directly to machine code)
