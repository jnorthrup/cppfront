# Conductor Tracks

## Track 1: Fix uninitialized variable warnings in reflect.h2
- **Status:** CLOSED — committed 39ecdac

## Track 2: io.h Cpp2 uplift (free functions + braces_tracker)
- **Status:** CLOSED — learned: .h2 files don't pass through Cpp1, namespace issues, runtime bugs

## Track 3: Create cpp2util_stage0.h — minimal runtime for compiler dogfooding
- **Status:** OPEN
- **Goal:** Replace cpp2util.h (3100 lines) with ~300 lines the compiler actually needs
- **Scope:** include/cpp2util_stage0.h + modify common.h to use it
- **What the compiler needs:**
  - Integer typedefs (u8, i32, etc.)
  - unchecked_narrow<T>
  - cmp_less/cmp_greater (signed/unsigned safety)
  - assert_not_null
  - CPP2_UFCS macros (the bulk — ~400 lines)
  - out<T>, inout<T>, move wrappers
  - impl::in<T>
- **What the compiler does NOT need:** is/as, contracts, safety checks, string interpolation, compiler ICE workarounds
- **Verification:** compiler builds with stage0 header, all regression tests pass

## Track 4: Wire EBNF combinator stack as compiler's parser
- **Status:** PENDING
- **Goal:** Use /Users/jim/work/cppfort/include/combinators/ for parsing instead of cppfront's hand-written parser
- **Scope:** parse.h integration with ebnf.hpp, parsing.hpp, structural.hpp
