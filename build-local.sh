#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src_dir="$repo_dir/source"
out_dir="$repo_dir/build-local"

pick_cxx() {
  if [[ -n "${CXX:-}" ]] && command -v "${CXX}" >/dev/null 2>&1; then
    printf '%s\n' "${CXX}"
    return
  fi

  if command -v clang++ >/dev/null 2>&1; then
    printf '%s\n' clang++
    return
  fi

  if command -v g++ >/dev/null 2>&1; then
    printf '%s\n' g++
    return
  fi

  if command -v c++ >/dev/null 2>&1; then
    printf '%s\n' c++
    return
  fi

  echo "No suitable C++ compiler found on PATH." >&2
  exit 1
}

cxx="$(pick_cxx)"

mkdir -p "$out_dir"

"$cxx" "$src_dir/cppfront.cpp" -std=c++20 -o "$out_dir/cppfront" "$@"

echo "Built trusted cppfront at $out_dir/cppfront"
