#!/usr/bin/env python3
with open('source/io2.h2') as f:
    lines = f.readlines()

out = []
for i, line in enumerate(lines):
    if line.strip() == 'class braces_tracker':
        # Found it - stop here
        break
    out.append(line)

# Remove trailing blank lines and comments
while out and (out[-1].strip() == '' or out[-1].strip().startswith('//')):
    out.pop()

out.append('\n}\n')

with open('/tmp/io2_funcs.h2', 'w') as f:
    f.writelines(out)

print(f"Wrote {len(out)} lines")
