# Contributing to bfc

**English** | [简体中文](CONTRIBUTING.md)

Thanks for your interest in bfc.

## Licence

bfc is released under the GNU Affero General Public License v3.0 or later
(AGPL-3.0-or-later). By submitting a contribution you agree that it is licensed
under those terms. See LICENSE.

## Developer Certificate of Origin (DCO)

This project requires the DCO only; there is no separate contributor licence
agreement to sign. Every commit must carry a DCO sign-off:

    git commit -s -m "your message"

which appends a line such as:

    Signed-off-by: Your Name <you@example.com>

Use your real name and a reachable e-mail address. The full DCO 1.1 text below
is binding on you.

### Developer Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I have the
    right to submit it under the open source licence indicated in the file; or

(b) The contribution is based upon previous work that, to the best of my
    knowledge, is covered under an appropriate open source licence and I have
    the right under that licence to submit that work with modifications,
    whether created in whole or in part by me, under the same open source
    licence (unless I am permitted to submit under a different licence), as
    indicated in the file; or

(c) The contribution was provided directly to me by some other person who
    certified (a), (b) or (c) and I have not modified it.

(d) I understand and agree that this project and the contribution are public
    and that a record of the contribution (including all personal information
    I submit with it, including my sign-off) is maintained indefinitely and
    may be redistributed consistent with this project or the open source
    licence(s) involved.

## Reporting bugs

Please include in the issue:

* a minimal Brainfuck source file (.bf) that reproduces the problem
* the bfc command you ran, together with its full output
* expected behaviour and actual behaviour
* platform (operating system / architecture) and g++ version (g++ --version)

If the issue is a security problem, please do not open a public issue; see
SECURITY.md instead.

## Development rules

* C++17, no third-party dependencies, must build warning-free with
  g++ -std=c++17 -Wall -Wextra.
* Keep the public shape: struct Op, class BFCompiler { parse, optimize,
  generate }.
* Run the full test suite before opening a pull request:

      powershell -File tests\run_tests.ps1

* Every behaviour change needs a new case under tests/bf and an update to the
  expected-output table in tests/run_tests.ps1.
