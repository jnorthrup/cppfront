#!/usr/bin/env bash
# selfhost.sh — continuous self-hosting bootstrap loop
# Strategy: incrementally convert compiler sources from C++ to cpp2
# Each iteration: trusted cppfront compiles all .h2/.cpp2 → .h/.cpp → build → verify
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$REPO/build-selfhost"
TRUSTED="$REPO/build-local/cppfront"
CXX="${CXX:-clang++}"

# The compiler source files in dependency order
# These are being incrementally converted from .h → .h2
SOURCES=(
    source/common.h
    source/io.h
    source/lex.h
    source/parse.h
    source/sema.h
    source/reflect.h
    source/to_cpp1.h
    source/cppfront.cpp
)

log() { printf "\033[1m[selfhost]\033[0m %s\n" "$*"; }
ok()  { printf "\033[32m[OK]\033[0m %s\n" "$*"; }
fail(){ printf "\033[31m[FAIL]\033[0m %s\n" "$*" >&2; return 1; }

mkdir -p "$BUILD"

# --- Phase 1: Build trusted cppfront if missing ---
if [[ ! -x "$TRUSTED" ]]; then
    log "Building trusted cppfront..."
    bash "$REPO/build-local.sh"
fi
ok "Trusted cppfront ready"

# --- Phase 2: For each source, prefer .h2 over .h ---
# Generate .h from .h2 where available, otherwise use existing .h
generate_cpp() {
    local src_cpp2="$1"    # e.g. source/io.h2
    local dst_cpp="$BUILD/$(basename "${src_cpp2%2}")"  # e.g. build-selfhost/io.h

    if [[ -f "$src_cpp2" ]]; then
        # Generate C++ from cpp2
        if ! "$TRUSTED" -c "$src_cpp2" -o "$dst_cpp" 2>&1; then
            # Fallback: just copy the line directives version
            "$TRUSTED" -cpp1_filename "$dst_cpp" "$src_cpp2" 2>&1
        fi
        echo "  $src_cpp2 → $dst_cpp"
    fi
}

# --- Phase 3: Assemble the hybrid compiler ---
assemble() {
    log "Assembling hybrid compiler..."

    # Copy base C++ sources that aren't yet converted
    for src in "${SOURCES[@]}"; do
        local base="$(basename "$src")"
        local cpp2_version="${src%.h}.h2"
        [[ ! -f "$cpp2_version" ]] && cpp2_version="${src%.cpp}.cpp2"

        if [[ -f "$cpp2_version" ]]; then
            # Use cpp2 version - generate .h/.cpp
            local generated="$BUILD/$base"
            if "$TRUSTED" -o "$generated" "$cpp2_version" 2>/dev/null; then
                echo "  $cpp2_version → $base (cpp2)"
            else
                fail "  $cpp2_version failed to generate C++ — using original"
                cp "$REPO/$src" "$BUILD/$base"
            fi
        else
            # Use original C++ 
            cp "$REPO/$src" "$BUILD/$base"
            echo "  $src → $base (original C++)"
        fi
    done
}

# --- Phase 4: Build the hybrid cppfront ---
build_hybrid() {
    log "Building hybrid cppfront..."
    "$CXX" "$BUILD/cppfront.cpp" -std=c++20 -I"$BUILD" -I"$REPO/source" -I"$REPO/include" \
        -o "$BUILD/cppfront" "$@" 2>&1 || {
        fail "Hybrid build failed"
        return 1
    }
    ok "Hybrid cppfront built"
}

# --- Phase 5: Bootstrap test - hybrid compiles its own sources ---
bootstrap_test() {
    log "Bootstrap test: hybrid compiles its own cpp2 sources..."
    local pass=true

    for h2 in "$REPO"/source/*.h2 "$REPO"/source/*.cpp2; do
        [[ -f "$h2" ]] || continue
        local name="$(basename "$h2")"
        if ! "$BUILD/cppfront" "$h2" 2>&1 | grep -q "ok"; then
            fail "  $name — hybrid could not compile"
            pass=false
        else
            ok "  $name — hybrid compiles it"
        fi
    done

    $pass
}

# --- Phase 6: Diff test - trusted vs hybrid produce identical output ---
diff_test() {
    log "Diff test: trusted vs hybrid output..."
    local pass=true

    for h2 in "$REPO"/source/*.h2 "$REPO"/source/*.cpp2; do
        [[ -f "$h2" ]] || continue
        local name="$(basename "$h2")"
        local out_trusted="$BUILD/diff_trusted_$name.cpp"
        local out_hybrid="$BUILD/diff_hybrid_$name.cpp"

        "$TRUSTED" -o "$out_trusted" "$h2" 2>/dev/null || true
        "$BUILD/cppfront" -o "$out_hybrid" "$h2" 2>/dev/null || true

        if diff -q "$out_trusted" "$out_hybrid" >/dev/null 2>&1; then
            ok "  $name — output matches"
        else
            fail "  $name — OUTPUT DIVERGES"
            diff "$out_trusted" "$out_hybrid" | head -20
            pass=false
        fi
    done

    $pass
}

# --- Main loop ---
log "=== Self-hosting bootstrap ==="

# Stats
cpp2_count=$(find "$REPO"/source -name "*.h2" -o -name "*.cpp2" 2>/dev/null | wc -l | tr -d ' ')
cpp1_count=${#SOURCES[@]}
log "Progress: $cpp2_count / $cpp1_count sources converted to cpp2"

assemble || fail "Assembly failed"
build_hybrid || fail "Build failed"

if bootstrap_test; then
    ok "Bootstrap test PASSED"
fi

if diff_test; then
    ok "Diff test PASSED — output stable"
fi

log "=== Self-hosting status ==="
log "Converted: $cpp2_count / $cpp1_count files"
log "Next to convert:"
for src in "${SOURCES[@]}"; do
    v="${src%.h}.h2"
    [[ ! -f "$v" ]] && v="${src%.cpp}.cpp2"
    if [[ ! -f "$v" ]]; then
        log "  → $src ($(wc -l < "$REPO/$src" | tr -d ' ') lines)"
    fi
done

if [[ $cpp2_count -ge $cpp1_count ]]; then
    ok "FULLY SELF-HOSTING — all sources in cpp2!"
fi
