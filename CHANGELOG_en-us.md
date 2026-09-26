# Changelog

**English** | [简体中文](CHANGELOG.md)

This project is in beta; version numbers are not guaranteed to follow strict
semantic versioning.

## beta 0.0.3

- New --tape-size N to configure the tape size in bytes (default 30000)
- New --bounds-check: emits bounds checks for the data pointer and aborts with
  exit code 2 when it leaves the tape; a guard region covers +/-off neighbour
  accesses so no read or write can leave the allocation
- New --compile-only (-c) to produce an object file (.o) without linking
- Security: the toolchain is no longer invoked through a shell. It is spawned
  with an explicit argument vector (Windows _spawnvp, POSIX fork + execvp), so
  file names and output paths cannot inject commands
- More specific errors: "file not found" and "permission denied" are
  distinguished, with the errno text attached
- Fixed some known bugs

## beta 0.0.2

- Supported OSes: Windows 8 / 8.1 / 10 / 11 (x64), Linux (x86-64, glibc and
  musl), macOS 10.15+ (x86-64), FreeBSD / OpenBSD (x86-64)
- New run-time target selection with --target, so one host can emit assembly
  for another (cross compilation): x86_64-windows / x86_64-linux /
  x86_64-freebsd / x86_64-macos
- New --cc CMD to choose the assembler/linker, --no-link (-S) to emit assembly
  only, --targets to list targets, --version to print the version
- New cross-platform build and test: Makefile, tests/run_tests.sh (POSIX sh),
  GitHub Actions CI (ubuntu-latest / macos-13 / windows-latest)
- Fixed some known bugs:
  - the target ABI (symbol prefix, argument register, shadow space, -static)
    was fixed when bfc itself was built, which made cross compilation impossible
  - ambiguous .align semantics in the macOS target; now .p2align
  - missing version number and target list

Out of scope: Windows XP (no UCRT), ARM64 (Apple Silicon / Raspberry Pi, needs
an aarch64 backend), DOS / z/OS / z/VSE / RTOS.

## beta 0.0.1

- First release: Brainfuck to x86-64 assembly compiler (C++17)
- Optimizations: meta-character filtering, run merging, opposite cancellation,
  [-] / [+] clearing, the [->+<] move family, scan loops, general
  multiply/copy/transfer loops, whole nested loop folding
- 26 functional cases plus 2 error cases, compared byte-for-byte against an
  independent interpreter
- Licence: AGPL-3.0-or-later with an output exception for compiled artefacts
