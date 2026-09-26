#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Xiaoguo
#
# POSIX test runner (Linux / macOS / *BSD). Mirrors tests/run_tests.ps1.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(dirname "$here")
bfc="$root/bfc"
[ -x "$bfc" ] || bfc="$root/bfc.exe"
bf="$here/bf"
asm="$here/asm"
bin="$here/bin"

mkdir -p "$asm" "$bin"

pass=0
fail=0

check() {
    name=$1
    exp=$2
    ( cd "$bf" && "$bfc" "$name.bf" -o "$bin/$name" ) >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "FAIL $name: compile"
        fail=$((fail + 1))
        return
    fi
    [ -f "$bf/$name.s" ] && mv -f "$bf/$name.s" "$asm/$name.s"
    if [ "$name" = "echo" ]; then
        "$bin/$name" < "$bf/echo_in.txt" > "$bin/$name.out"
    else
        "$bin/$name" > "$bin/$name.out"
    fi
    got=$(od -An -v -tx1 "$bin/$name.out" | tr -s ' \n' ' ' | tr 'a-f' 'A-F' | sed 's/0D //g; s/^ *//; s/ *$//')
    if [ "$got" = "$exp" ]; then
        pass=$((pass + 1))
    else
        echo "FAIL $name: exp=[$exp] got=[$got]"
        fail=$((fail + 1))
    fi
}

while IFS='|' read -r name exp; do
    [ -z "$name" ] && continue
    check "$name" "$exp"
done <<'CASES'
A|41
move|41
hello|48 65 6C 6C 6F 20 57 6F 72 6C 64 21 0A
run_merge|03
multiply|0C
copy|05 05
move_b|41
move_c|41
move_d|41
scan_right|01
scan_left|01
scan_guard|01
clear_plus|05
cancel_add|02
cancel_move|01
dots|01 01 02 02
nested_fold|40
nested_kept|10
nested_guard|07
nested_reset|00 00 00 00 00 08 00
nested_deep|00 00 00 00 00 00 18
nested_v0|00 00 00 00 07 05 00
wrap255|FF
wrap300|2C
comment|41
echo|5A
CASES

# bracket errors must be rejected
for n in err_open err_close; do
    if "$bfc" "$bf/$n.bf" -o "$bin/$n" >/dev/null 2>&1; then
        echo "FAIL $n: should have been rejected"
        fail=$((fail + 1))
    else
        pass=$((pass + 1))
    fi
done

rm -rf "$bin"

echo ""
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
