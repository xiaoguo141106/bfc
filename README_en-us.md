# bfc - Brainfuck to x86-64 assembly compiler

[![License](https://img.shields.io/github/license/xiaoguo141106/bfc?style=flat-square&color=111111)](./LICENSE)
[![Stars](https://img.shields.io/github/stars/xiaoguo141106/bfc?style=flat-square&logo=github&color=111111)](https://github.com/xiaoguo141106/bfc/stargazers)
[![Forks](https://img.shields.io/github/forks/xiaoguo141106/bfc?style=flat-square&logo=github&color=111111)](https://github.com/xiaoguo141106/bfc/forks)

**English** | [简体中文](README.md)

Version beta 0.0.3 · [Changelog](CHANGELOG_en-us.md) · [Versioning](VERSIONING_en-us.md)

A C++17 compiler that turns Brainfuck into x86-64 assembly (AT&T syntax) and
then drives g++ to assemble and link it into a standalone executable that
depends only on the operating system runtime.

## Build and usage

    g++ -std=c++17 -O2 -static -o bfc.exe bfc.cpp
    bfc.exe <input.bf> [-o output.exe]

Or use the Makefile (Linux / macOS / MinGW):

    make
    make test

* Intermediate output: an .s file named after the input (hello.bf -> hello.s)
* Final output: an executable named after the input; override with -o
* Runs internally: g++ -O2 -static -o "output" "input.s"

## Command-line options

| Option | Effect |
|--------|--------|
| -o FILE | Output file (default: an executable, or a .o with -c) |
| --target NAME | Target ABI: x86_64-windows / x86_64-linux / x86_64-freebsd / x86_64-macos (default auto = host) |
| --cc CMD | Assembler/linker to use, e.g. --cc x86_64-linux-gnu-g++ |
| --tape-size N | Tape size in bytes, default 30000 |
| --bounds-check | Emit bounds checks for the data pointer; exit code 2 on escape (see below) |
| --no-link / -S | Emit .s only |
| --compile-only / -c | Emit a .o only, do not link |
| --version / --targets | Print the version / list targets |

--bounds-check checks the data pointer itself. Optimised instructions that use a
fixed offset ([->+<], [->+>+<<], ...) stay memory-safe because of a guard region
(the tape is allocated with 2 x max-offset extra bytes). Without the option no
checks are emitted and going out of bounds is undefined behaviour.

## Code structure

    struct Op { char kind; int value; };
    class BFCompiler { void parse(); void optimize(); void generate(); };

parse filters meta-characters, validates brackets, merges runs and cancels
opposite neighbours; optimize performs loop abstract interpretation; generate
emits assembly per Op.kind.

Op.kind: ordinary BF instructions, plus

    Z   clear             A B C D  [->+<] / [->-<] / [-<+>] / [-<->]
    S   scan loop         M        general transfer loop (index into transfers_)

## Optimizations

1. Meta-character filtering.
2. Run merging, opposite-neighbour cancellation with re-merging
   (+++--+ -> addb $2).
3. [-] and [+] folded to movb $0.
4. Six-token move patterns A/B/C/D.
5. Scan loops [<] / [>] (any step): zero test on entry, then move-before-test.
6. General multiply / copy / transfer loops: any offset, any coefficient,
   multiple targets ([->+++<] uses imull, [->+>+<<] copies).
7. Whole nested loops folded: summaries are computed inside-out and inner
   summaries are substituted into the outer analysis.

## Loop analysis

Every loop body is abstractly interpreted. Each data cell records
(valKnown, val) and (deltaKnown, delta): whether its value and its
per-iteration delta are constant. Nested loops are summarised recursively and
then applied: a known control value is applied exactly, an unknown one poisons
the targets, clear sets 0, scan abandons the outer loop.

Acceptance conditions: net pointer displacement 0; the control cell changes by
exactly -1 per iteration; every other touched cell either accumulates a
constant or is reset to a constant. Codegen uses imull/addb for accumulation,
movb for resets, and zeroes the control cell.

A reset cell only changes if the loop body ran at least once, so all resets -
no matter how deeply nested - share a single testb/je (V!=0) guard and the
semantics stay complete. When a proof fails the compiler conservatively keeps
the real loop, so semantics never change.

## x86-64 and portability

* r12 = tape base, r13 = data pointer; a 30000-byte tape in .bss.
* Entry rsp = 8 (mod 16), 0 after push rbp/r12/r13; Windows additionally
  reserves 32 bytes of shadow space, so every call site stays 16-byte aligned.
* The platform branch is chosen at compile time: Windows x64 (no prefix,
  shadow space, -static), Linux/*BSD (no prefix, -static), macOS (underscore
  prefix, no -static).

## Platform support

| Platform | Build bfc | Build BF output | Status |
|----------|:---------:|:---------------:|--------|
| Windows 10 / 11 (x64) | yes | yes | CI verified |
| Windows 8 / 8.1 (x64) | yes | yes | needs the UCRT (KB2999226) |
| Windows 7 SP1 (x64) | partial | partial | needs the UCRT update, not CI verified |
| Linux (x86-64, glibc) | yes | yes | verified locally (WSL) + CI |
| Linux (x86-64, musl) | yes | yes | use --cc musl-g++ |
| macOS 10.15+ (x86-64) | yes | yes | CI verified (macos-13 runner) |
| FreeBSD / OpenBSD (x86-64) | partial | partial | SysV ELF path, not CI verified |
| Windows XP | no | no | no UCRT |
| ARM64 (Apple Silicon / Raspberry Pi) | no | no | needs an aarch64 backend |
| DOS / z/OS / z/VSE / RTOS | no | no | out of scope |

The target ABI is chosen at run time, so one host can emit assembly for another:

    bfc --target x86_64-linux --cc x86_64-linux-gnu-g++ hello.bf
    bfc --target x86_64-windows --no-link hello.bf     # emit .s only
    bfc --targets                                      # list targets

## Error handling

* Cannot open the file -> error, exit code 1.
* Unmatched [ / ] -> error, exit code 1, no executable produced.

## Tests

    powershell -File tests\run_tests.ps1

26 functional cases plus 2 error cases, all compared byte-for-byte against an
independent interpreter (currently 28 passed, 0 failed). Sources live in
tests/bf and the generated assembly is written to tests/asm; see
tests/README.md. Random differential testing covers general transfer loops and
multi-level reset nesting. g++ -Wall -Wextra is clean.

## Licence

This project is released under the GNU Affero General Public License v3.0 or
later (AGPL-3.0-or-later). SPDX identifier: AGPL-3.0-or-later.

* Full licence text: LICENSE
* Output exception for compiler artefacts: LICENSE-EXCEPTION.md
* Contribution guide (DCO sign-off, bug reports): CONTRIBUTING.md
* Security policy (private vulnerability reports): SECURITY.md

### Licence of compiler output

The AGPL has no built-in compiler output exception. So that downstream users
are not left guessing about the licence of what they compile, this project
grants an additional permission under section 7 of the AGPLv3: the assembly,
object files and executables that bfc generates from your own Brainfuck source
may be used and distributed under terms of your choice. See
LICENSE-EXCEPTION.md. The exception does not cover the bfc compiler itself.

### Third-party components

* libstdc++ / libgcc (statically linked with -static): GPL-3.0-or-later WITH
  GCC-exception-3.1, which explicitly permits linking and redistribution.
* MinGW-w64 runtime and headers: permissive licences (Public Domain / BSD /
  ZPL style).
* UCRT api-ms-win-crt-* and KERNEL32: Windows system components, not bundled
  libraries.
* bfc invokes an external g++ at run time; that is a process invocation, not
  linking.
