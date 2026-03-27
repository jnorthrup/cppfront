#!/usr/bin/env python3
"""
Build io.h from io2.h2 + original Cpp1 code.

Strategy:
1. Transpile io2.h2 (Cpp2 free functions) → generates declarations + definitions
2. Take original io.h, strip the free functions that Cpp2 replaces
3. Combine: Cpp2 output (wrapped in namespace cpp2) + remaining Cpp1 code
"""

import subprocess, re

# 1. Transpile io2.h2 (only the Cpp2 free functions, not the passthrough)
# First, build io2.h2 with just the free functions
with open('source/io2.h2') as f:
    content = f.read()

# Extract just the Cpp2 free functions (everything before braces_tracker marker)
lines = content.split('\n')
cut = len(lines)
for i, line in enumerate(lines):
    if 'pre_if_depth_info:' in line or 'braces_tracker:' in line:
        cut = i
        break
    if line.strip() == '}  // namespace cpp2' and i > 200:
        cut = i + 1
        break

cpp2_only = '\n'.join(lines[:cut])
# Ensure namespace is closed
if '}  // namespace cpp2' not in cpp2_only:
    cpp2_only += '\n}  // namespace cpp2\n'
with open('/tmp/io2_funcs.h2', 'w') as f:
    f.write(cpp2_only)

# Transpile
result = subprocess.run(
    ['build-local/cppfront', '/tmp/io2_funcs.h2', '-o', '/tmp/io2_funcs.h'],
    capture_output=True, text=True
)
if result.returncode != 0:
    print(f"Transpile failed: {result.stderr}")
    exit(1)
print("Transpiled Cpp2 functions")

# 2. Read original io.h from git
result2 = subprocess.run(['git', 'show', 'HEAD:source/io.h'], capture_output=True, text=True)
orig = result2.stdout
orig_lines = orig.split('\n')

# 3. Read transpiled Cpp2 functions
with open('/tmp/io2_funcs.h') as f:
    cpp2_gen = f.read()

# 4. Build final io.h
# Structure:
# - Original header (copyright, includes, namespace open)
# - Cpp2 forward declarations (from generated file)
# - Original Cpp1 code minus free functions (braces_tracker onwards)
# - Cpp2 function definitions (from generated file)
# - Closing namespace + include guard

# Original: lines 0-16 are header (copyright, #ifndef, #include)
# Lines 17 is "namespace cpp2 {"
# Lines 18-21 are comments
# Lines 22-~336 are free functions (to be replaced)
# Lines ~337-1107 are braces_tracker, remaining code, closing }
# Line 1108 is #endif

# Find where free functions end (braces_tracker start)
bt_line = None
for i, line in enumerate(orig_lines):
    if line.strip() == 'class braces_tracker':
        bt_line = i
        break

# Extract parts
header = '\n'.join(orig_lines[:17])  # up to and including namespace open
remaining = '\n'.join(orig_lines[bt_line:-2])  # braces_tracker onwards, minus closing } and #endif

# Parse generated file for declarations and definitions sections
gen_lines = cpp2_gen.split('\n')
# Find the two namespace blocks (declarations and definitions)
decls = []
defs = []
in_decls = False
in_defs = False
for line in gen_lines:
    if 'type declarations' in line:
        in_decls = True
        in_defs = False
        continue
    if 'function definitions' in line:
        in_defs = True
        in_decls = False
        continue
    if in_decls:
        decls.append(line)
    if in_defs:
        defs.append(line)

decls_text = '\n'.join(decls).strip()
defs_text = '\n'.join(defs).strip()

# Assemble
final = header + '\n'
final += '//  Cpp2 declarations\n\n'
final += decls_text + '\n\n'
final += remaining + '\n\n'
final += '//  Cpp2 function definitions\n\n'
final += defs_text + '\n'
final += '\n}\n\n#endif\n'

with open('source/io.h', 'w') as f:
    f.write(final)

print(f"Final: {final.count(chr(10))+1} lines")
