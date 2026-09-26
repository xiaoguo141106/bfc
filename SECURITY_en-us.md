# Security Policy

**English** | [简体中文](SECURITY.md)

## Supported versions

Only the latest release (currently beta-0.0.2) receives security fixes.

## Reporting a vulnerability

Please report security issues privately. Send your e-mail as follows:

    To:  xggzyx11@outlook.com
    Cc:  xiaoguo718@gmail.com
    Cc:  xiaoguo1@proton.me

Please do not open a public issue. We will acknowledge your report as soon as
possible and keep you informed while we investigate and prepare a fix. Please
give us reasonable time to release a fix before disclosing the issue publicly.

## Scope

bfc compiles Brainfuck to x86-64 assembly and then invokes g++. Relevant issues
include, but are not limited to:

* unsafe or memory-corrupting code generated for valid input
* out-of-bounds tape accesses produced by the compiler
* command injection through the input file name or the output path
* denial of service on small inputs (unbounded compile time or memory)

## Ordinary bugs

For non-security issues see the "Reporting bugs" section of CONTRIBUTING.md.
