#!/usr/bin/env python3
"""Merge Cpp2-generated free functions into original io.h."""

import subprocess

# 1. Get original io.h
result = subprocess.run(['git', 'show', 'HEAD:source/io.h'], capture_output=True, text=True)
orig_lines = result.stdout.split('\n')

# 2. Read transpiled Cpp2 functions
with open('/tmp/io2_gen.h') as f:
    gen = f.read()
gen_lines = gen.split('\n')

# 3. Find boundaries in original
# Free functions: lines 18 (namespace open) to braces_tracker start
bt_line = None
for i, line in enumerate(orig_lines):
    if line.strip() == 'class braces_tracker':
        bt_line = i
        break

# 4. Extract Cpp2 declarations and definitions from generated file
# Generated structure:
# - Header + include guard
# - Forward declarations section
# - Function definitions section
# - #endif

# Find forward decl section (after "type declarations" marker)
decl_start = decl_end = None
def_start = def_end = None
for i, line in enumerate(gen_lines):
    if 'type declarations' in line:
        decl_start = i
    if 'type definitions and function declarations' in line:
        if decl_start and not decl_end:
            decl_end = i
        def_start = i
    if 'function definitions' in line:
        if def_start and not def_end:
            def_end = i

# Get declarations (between markers)
decls = []
for i, line in enumerate(gen_lines):
    if decl_start and i > decl_start and (not decl_end or i < decl_end):
        if 'namespace cpp2' in line or line.strip() == '}' or line.strip().startswith('#') or line.strip() == '':
            decls.append(line)
        elif '[[nodiscard]]' in line or line.startswith('class ') or line.startswith('template'):
            decls.append(line)

# Get definitions (after function definitions marker)
defs = []
in_defs = False
for line in gen_lines:
    if 'function definitions' in line:
        in_defs = True
        continue
    if in_defs and line.strip() != '#endif':
        defs.append(line)

# 5. Build final: original header + Cpp2 decls + original from braces_tracker + Cpp2 defs + close
final_lines = orig_lines[:bt_line]  # header + namespace open + comments

# Add Cpp2 declarations
final_lines.append('')
final_lines.append('//  Cpp2 type declarations and function declarations')
final_lines.append('')
for line in decls:
    if line.strip() and '#line' not in line and '#ifndef' not in line and '#define' not in line and '#include "cpp2util.h"' not in line:
        final_lines.append(line)

# Add original code from braces_tracker onwards (minus closing })
final_lines.append('')
for line in orig_lines[bt_line:-2]:  # skip final } and #endif
    final_lines.append(line)

# Add Cpp2 function definitions
final_lines.append('')
final_lines.append('//  Cpp2 function definitions')
final_lines.append('')
for line in defs:
    if '#line' not in line and '#ifndef' not in line and '#define' not in line and '#include' not in line:
        final_lines.append(line)

# Close
final_lines.append('}')
final_lines.append('')
final_lines.append('#endif')

final = '\n'.join(final_lines)

with open('source/io.h', 'w') as f:
    f.write(final)

print(f"Final: {len(final_lines)} lines")
