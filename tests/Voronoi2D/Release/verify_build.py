# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual compiler commands, including library sanitizer instrumentation."""
import json
import pathlib
import shlex
import sys

build = pathlib.Path(sys.argv[1])
commands = json.loads((build / 'compile_commands.json').read_text())
banned = {'-ffast-math', '-funsafe-math-optimizations', '-fassociative-math', '-Ofast'}
checked = 0
violations = []
for entry in commands:
    arguments = entry.get('arguments') or shlex.split(entry['command'])
    invalid = sorted(banned.intersection(arguments))
    invalid += [flag for flag in arguments if flag == '-flto' or flag.startswith('-flto=')]
    if invalid:
        violations.append({'file': entry['file'], 'flags': invalid})
    if '/VMMLib/src/' in entry['file'] or '/tests/' in entry['file']:
        checked += 1
        if not {'-fsanitize=address', '-fsanitize=undefined'}.issubset(arguments):
            violations.append({'file': entry['file'], 'sanitizers': 'missing'})
print(json.dumps({'compiler_commands': len(commands), 'instrumented_commands_checked': checked,
                  'violations': violations}, indent=2))
assert checked and not violations
