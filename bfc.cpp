// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Xiaoguo
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
// License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Additional permission under AGPL-3.0 section 7 governs the compiler's
// output; see LICENSE-EXCEPTION.md.

// ============================================================================
//  bfc.cpp -- Brainfuck -> x86-64 assembly compiler
// ----------------------------------------------------------------------------
//  Usage:  bfc <input.bf> [-o output]
//
//  Pipeline:
//      parse()     filter meta-characters, collapse runs, cancel opposites
//                  -> vector<Op>
//      optimize()  loop-aware abstract interpretation:
//                    * [-] / [+]                -> clear cell
//                    * [<] / [>]                -> tight scan loop
//                    * [->+<] family            -> 3-instruction move
//                    * [->+++<], [->+>+<<], ... -> general transfer loop
//                    * nested loops             -> innermost-first summaries
//                  -> vector<Op>
//      generate()  emit AT&T-syntax GAS assembly
//
//  Then drives:  g++ -O2 -static -o <output> <input>.s
//
//  Targets (host == target, selected at compile time):
//      Windows x64  (MinGW-w64) : MS x64 ABI, 32-byte shadow space, UCRT
//      Linux / *BSD / Unix      : System V AMD64 ABI, glibc
//      macOS x86-64             : System V AMD64 ABI, underscore-prefixed syms
//
//  Register discipline: r12 = tape base, r13 = data pointer.
//  Tape: 30000 bytes of zero-initialised storage in .bss.
// ============================================================================

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#  include <process.h>
#else
#  include <sys/wait.h>
#  include <unistd.h>
#endif

// ---------------------------------------------------------------------------
//  Target description -- the ABI the generated assembly is written for.
//  Selected at run time (--target), so a compiler built on one OS can emit
//  assembly for another (cross compilation).
// ---------------------------------------------------------------------------
enum class TargetOS { Windows, Elf, MacOS };

struct Target {
    const char* name;
    TargetOS    os;
    bool        shadowSpace;   // MS x64: reserve 32 bytes of home space
    const char* symPrefix;     // Mach-O symbols take a leading underscore
    const char* arg0;          // register holding the first integer argument
    const char* exeSuffix;
    bool        staticLink;    // pass -static to the toolchain
    bool        gnuStackNote;  // emit .note.GNU-stack (ELF linkers only)
};

static const Target kTargets[] = {
    { "x86_64-windows", TargetOS::Windows, true,  "",  "%ecx", ".exe", true,  false },
    { "x86_64-linux",   TargetOS::Elf,     false, "",  "%edi", "",     true,  true  },
    { "x86_64-freebsd", TargetOS::Elf,     false, "",  "%edi", "",     true,  false },
    { "x86_64-macos",   TargetOS::MacOS,   false, "_", "%edi", "",     false, false },
};
static const int kTargetCount = static_cast<int>(sizeof(kTargets) / sizeof(kTargets[0]));

static const char* const kVersion = "beta 0.0.3";

// Target of the machine this compiler runs on.  Only x86-64 hosts are served;
// on another architecture --target (plus a matching --cc) must be given.
static const char* hostTargetName() {
#if defined(_WIN32)
#  if defined(_M_ARM64) || defined(__aarch64__)
    return nullptr;
#  else
    return "x86_64-windows";
#  endif
#elif defined(__APPLE__)
#  if defined(__aarch64__) || defined(__arm64__)
    return nullptr;
#  else
    return "x86_64-macos";
#  endif
#else
#  if defined(__x86_64__) || defined(__amd64__)
    return "x86_64-linux";
#  else
    return nullptr;
#  endif
#endif
}

static const Target* findTarget(const std::string& name) {
    for (int i = 0; i < kTargetCount; ++i)
        if (name == kTargets[i].name) return &kTargets[i];
    return nullptr;
}

static const int kTapeSize = 30000;

// ---------------------------------------------------------------------------
//  Op -- one code-generation unit
//    kind : plain BF   '>' '<' '+' '-' '.' ',' '[' ']'
//           'Z'        movb $0,(%r13)              (folded [-] / [+])
//           'A'        [->+<]     'B'  [->-<]
//           'C'        [-<+>]     'D'  [-<->]
//           'S'        scan loop, value = signed step (e.g. [<] -> -1)
//           'M'        general transfer loop, value = index into transfers_
//    value: repeat count for '>' '<' '+' '-', step for 'S', index for 'M'
// ---------------------------------------------------------------------------
struct Op {
    char kind;
    int  value;
};

// Effect of a transfer loop, per iteration, relative to its control cell:
//     cell[offset] += coefficient * control
//     cell[offset]  = value                       (reset cells)
struct TransferSummary {
    std::vector<std::pair<int, int> > accum;   // (offset, coefficient 1..255)
    std::vector<std::pair<int, int> > resets;  // (offset, value 0..255)
};

enum class LoopKind { None, Clear, Scan, Special, Transfer };

struct LoopInfo {
    LoopKind        kind      = LoopKind::None;
    char            special   = 0;      // 'A'..'D'
    int             scanDelta = 0;      // signed step for Scan
    TransferSummary tr;                 // for Special / Transfer
};

// ---------------------------------------------------------------------------
//  BFCompiler
// ---------------------------------------------------------------------------
class BFCompiler {
public:
    BFCompiler(std::string sourceName, const Target& target, int tapeSize, bool boundsCheck)
        : sourceName_(std::move(sourceName)), target_(&target),
          tapeSize_(tapeSize), boundsCheck_(boundsCheck) {}

    void parse(const std::string& path);
    void optimize();
    void generate(std::ostream& out) const;

    const std::vector<Op>& ops() const { return ops_; }

private:
    // Abstract per-cell state used while analysing one loop iteration.
    //   valKnown : true  -> the cell holds the constant 'val' no matter what
    //                       it held when the iteration started
    //   deltaKnown: true -> the cell changed by the constant 'delta' during
    //                       this iteration, independent of entry value
    struct Cell {
        bool valKnown   = false;
        int  val        = 0;
        bool deltaKnown = true;
        int  delta      = 0;
    };

    void     buildMatch();
    void     optimizeRange(std::size_t lo, std::size_t hi, std::vector<Op>& out);
    LoopInfo classify(std::size_t l, std::size_t r);
    bool     analyzeBody(std::size_t lo, std::size_t hi, TransferSummary& out);
    void     computeMaxOffset();
    void     emitBoundsCheck(std::ostream& out, int oobLabel) const;

    std::string              sourceName_;
    const Target*            target_ = nullptr;
    int                      tapeSize_ = 30000;
    bool                     boundsCheck_ = false;
    int                      maxOff_ = 0;
    std::vector<Op>          ops_;
    std::vector<int>         match_;      // bracket partner index
    std::vector<TransferSummary> transfers_;
    std::map<std::pair<std::size_t, std::size_t>, LoopInfo> memo_;
};

// --- helpers ---------------------------------------------------------------
static bool isBF(char c) {
    switch (c) {
        case '>': case '<': case '+': case '-':
        case '.': case ',': case '[': case ']':
            return true;
        default:
            return false;
    }
}

static bool isRepeatable(char c) {
    return c == '>' || c == '<' || c == '+' || c == '-';
}

static bool isOpposite(char a, char b) {
    return (a == '+' && b == '-') || (a == '-' && b == '+')
        || (a == '>' && b == '<') || (a == '<' && b == '>');
}

static int norm256(int x) { return ((x % 256) + 256) % 256; }

// ===========================================================================
//  parse(): read the file, drop every non-BF byte, validate bracket balance,
//           collapse consecutive same-direction instructions, then cancel
//           adjacent opposite pairs ("> <" and "+ -" vanish).
// ===========================================================================
void BFCompiler::parse(const std::string& path) {
    errno = 0;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        const int e = errno;
        if (e == ENOENT) throw std::runtime_error("input file not found: " + path);
        if (e == EACCES) throw std::runtime_error("permission denied reading: " + path);
        if (e != 0)
            throw std::runtime_error("cannot open input file: " + path + ": " +
                                     std::strerror(e));
        throw std::runtime_error("cannot open input file: " + path);
    }

    const std::string raw((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    std::string code;
    code.reserve(raw.size());
    for (char c : raw)
        if (isBF(c)) code.push_back(c);

    std::vector<char> open;
    for (std::size_t i = 0; i < code.size(); ++i) {
        if (code[i] == '[') {
            open.push_back('[');
        } else if (code[i] == ']') {
            if (open.empty())
                throw std::runtime_error("unmatched ']' in " + path);
            open.pop_back();
        }
    }
    if (!open.empty())
        throw std::runtime_error("unmatched '[' in " + path);

    // collapse runs of the same instruction
    std::vector<Op> merged;
    for (std::size_t i = 0; i < code.size(); ) {
        const char c = code[i];
        if (c == '>' || c == '<' || c == '+' || c == '-') {
            int n = 0;
            while (i < code.size() && code[i] == c) { ++n; ++i; }
            merged.push_back(Op{c, n});
        } else {
            merged.push_back(Op{c, 0});
            ++i;
        }
    }

    // cancel adjacent opposite pairs
    std::vector<Op> norm;
    norm.reserve(merged.size());
    for (const Op& op : merged) {
        if (norm.empty()) { norm.push_back(op); continue; }
        Op& back = norm.back();
        if (back.kind == op.kind && isRepeatable(op.kind)) {
            back.value += op.value;                 // re-merge a run split by cancellation
        } else if (isOpposite(back.kind, op.kind)) {
            const int d = back.value - op.value;
            if (d > 0) {
                back.value = d;
            } else if (d < 0) {
                norm.pop_back();
                norm.push_back(Op{op.kind, -d});
            } else {
                norm.pop_back();
            }
        } else {
            norm.push_back(op);
        }
    }

    ops_.swap(norm);
}

// ===========================================================================
//  buildMatch(): pair every '[' with its ']' (parse() already validated).
// ===========================================================================
void BFCompiler::buildMatch() {
    match_.assign(ops_.size(), -1);
    std::vector<std::size_t> st;
    for (std::size_t i = 0; i < ops_.size(); ++i) {
        if (ops_[i].kind == '[') {
            st.push_back(i);
        } else if (ops_[i].kind == ']') {
            if (st.empty())
                throw std::runtime_error("internal error: unmatched ']'");
            const std::size_t l = st.back();
            st.pop_back();
            match_[l] = static_cast<int>(i);
            match_[i] = static_cast<int>(l);
        }
    }
    if (!st.empty())
        throw std::runtime_error("internal error: unmatched '['");
}

// ===========================================================================
//  optimize(): classify every loop, innermost first (memoised), then rewrite.
// ===========================================================================
void BFCompiler::optimize() {
    buildMatch();
    transfers_.clear();
    memo_.clear();

    std::vector<Op> out;
    out.reserve(ops_.size());
    optimizeRange(0, ops_.size(), out);
    ops_.swap(out);
    computeMaxOffset();
}

void BFCompiler::optimizeRange(std::size_t lo, std::size_t hi, std::vector<Op>& out) {
    std::size_t i = lo;                       // explicit index, no for/continue
    while (i < hi) {
        const Op& op = ops_[i];

        if (op.kind == '[') {
            const std::size_t r = static_cast<std::size_t>(match_[i]);
            const LoopInfo li = classify(i, r);

            if (li.kind == LoopKind::Clear) {
                out.push_back(Op{'Z', 0});
                i = r + 1;
                continue;
            }
            if (li.kind == LoopKind::Scan) {
                out.push_back(Op{'S', li.scanDelta});
                i = r + 1;
                continue;
            }
            if (li.kind == LoopKind::Special) {
                out.push_back(Op{li.special, 0});
                i = r + 1;
                continue;
            }
            if (li.kind == LoopKind::Transfer) {
                // collapse single unit transfers onto the compact A/B/C/D forms
                const TransferSummary& t = li.tr;
                if (t.resets.empty() && t.accum.size() == 1) {
                    const int off  = t.accum[0].first;
                    const int coef = t.accum[0].second;
                    char sp = 0;
                    if      (off ==  1 && coef ==   1) sp = 'A';
                    else if (off ==  1 && coef == 255) sp = 'B';
                    else if (off == -1 && coef ==   1) sp = 'C';
                    else if (off == -1 && coef == 255) sp = 'D';
                    if (sp) {
                        out.push_back(Op{sp, 0});
                        i = r + 1;
                        continue;
                    }
                }
                out.push_back(Op{'M', static_cast<int>(transfers_.size())});
                transfers_.push_back(t);
                i = r + 1;
                continue;
            }

            // unrecognised loop: keep it, but optimise its body recursively
            out.push_back(op);
            optimizeRange(i + 1, r, out);
            out.push_back(Op{']', 0});
            i = r + 1;
            continue;
        }

        out.push_back(op);
        ++i;
    }
}

// ===========================================================================
//  classify(): what kind of loop is ops_[l] .. ops_[r]?
//              Pure function of the loop -> memoised.
// ===========================================================================
LoopInfo BFCompiler::classify(std::size_t l, std::size_t r) {
    const std::pair<std::size_t, std::size_t> key(l, r);
    const std::map<std::pair<std::size_t, std::size_t>, LoopInfo>::iterator cached = memo_.find(key);
    if (cached != memo_.end()) return cached->second;

    LoopInfo info;
    const std::size_t bodyLen = r - l - 1;   // ops strictly between [ and ]

    if (bodyLen == 0) {                      // "[]" -- undecidable, leave alone
        memo_[key] = info;
        return info;
    }

    if (bodyLen == 1) {
        const Op& b = ops_[l + 1];
        if (b.kind == '+' || b.kind == '-') {
            const int net = (b.kind == '+') ? b.value : -b.value;
            const int m = norm256(net);
            if (m == 1 || m == 255) {        // net +/-1 per iteration => clears
                info.kind = LoopKind::Clear;
                memo_[key] = info;
                return info;
            }
        }
        if (b.kind == '>') { info.kind = LoopKind::Scan; info.scanDelta =  b.value; memo_[key] = info; return info; }
        if (b.kind == '<') { info.kind = LoopKind::Scan; info.scanDelta = -b.value; memo_[key] = info; return info; }
        memo_[key] = info;
        return info;
    }

    // [->+<] / [->-<] / [-<+>] / [-<->] : six primitive tokens
    if (bodyLen == 4) {
        const Op& o0 = ops_[l + 1];
        const Op& o1 = ops_[l + 2];
        const Op& o2 = ops_[l + 3];
        const Op& o3 = ops_[l + 4];
        if (o0.kind == '-' && norm256(o0.value) == 1
            && (o1.kind == '>' || o1.kind == '<') && o1.value == 1
            && (o2.kind == '+' || o2.kind == '-') && o2.value == 1
            && (o3.kind == '>' || o3.kind == '<') && o3.value == 1
            && ((o1.kind == '>' && o3.kind == '<') || (o1.kind == '<' && o3.kind == '>'))) {
            info.kind = LoopKind::Special;
            if (o1.kind == '>') info.special = (o2.kind == '+') ? 'A' : 'B';
            else                info.special = (o2.kind == '+') ? 'C' : 'D';
            info.tr.accum.push_back(std::make_pair(o1.kind == '>' ? 1 : -1,
                                                   o2.kind == '+' ? 1 : 255));
            memo_[key] = info;
            return info;
        }
    }

    // general transfer loop: [->+++<], [->+>+<<], nested variants, ...
    TransferSummary ts;
    if (analyzeBody(l + 1, r, ts)) {
        info.kind = LoopKind::Transfer;
        info.tr   = ts;
    }
    memo_[key] = info;
    return info;
}

// ===========================================================================
//  analyzeBody(): abstract interpretation of one loop iteration.
//
//  Returns true iff the body is a *linear transfer*: pointer returns to its
//  start, the control cell changes by exactly -1 per iteration, and every
//  other touched cell either accumulates a constant delta or is reset to a
//  constant.  On success 'out' holds the per-iteration effect.
//
//  Nested loops are folded via classify():
//    * clear/transfer with a statically known control value  -> exact effect
//    * transfer with an unknown control value                -> poisons targets
//    * scan                                                  -> pointer dynamic
//    * anything else                                         -> reject
// ===========================================================================
bool BFCompiler::analyzeBody(std::size_t lo, std::size_t hi, TransferSummary& out) {
    std::map<int, Cell> st;

    std::size_t i = lo;
    int  p = 0;                 // pointer offset from the loop's control cell
    bool ptrKnown = true;

    while (i < hi) {
        const Op& op = ops_[i];

        if (op.kind == '>') { p += op.value; ++i; continue; }
        if (op.kind == '<') { p -= op.value; ++i; continue; }
        if (op.kind == '+' || op.kind == '-') {
            Cell& c = st[p];
            if (op.kind == '+') {
                c.delta = norm256(c.delta + op.value);
                if (c.valKnown) c.val = norm256(c.val + op.value);
            } else {
                c.delta = norm256(c.delta - op.value);
                if (c.valKnown) c.val = norm256(c.val - op.value);
            }
            ++i;
            continue;
        }
        if (op.kind == '.' || op.kind == ',') return false;   // side effects

        if (op.kind == '[') {
            const std::size_t r = static_cast<std::size_t>(match_[i]);
            const LoopInfo li = classify(i, r);

            if (li.kind == LoopKind::Clear) {
                Cell& c = st[p];
                if (c.valKnown) { c.delta = norm256(c.delta + (256 - c.val)); c.val = 0; }
                else            { c.valKnown = true; c.val = 0; c.deltaKnown = false; }
            } else if (li.kind == LoopKind::Scan) {
                return false;                                  // pointer becomes dynamic
            } else if (li.kind == LoopKind::Special || li.kind == LoopKind::Transfer) {
                Cell& ctl = st[p];
                if (ctl.valKnown) {
                    const int K = ctl.val;
                    ctl.delta = norm256(ctl.delta + (256 - K)); // ends at 0
                    ctl.val   = 0;
                    if (K != 0) {                              // body actually runs
                        for (std::size_t a = 0; a < li.tr.accum.size(); ++a) {
                            Cell& t = st[p + li.tr.accum[a].first];
                            const int add = norm256(li.tr.accum[a].second * K);
                            t.delta = norm256(t.delta + add);
                            if (t.valKnown) t.val = norm256(t.val + add);
                        }
                        for (std::size_t s = 0; s < li.tr.resets.size(); ++s) {
                            Cell& t = st[p + li.tr.resets[s].first];
                            t.valKnown = true; t.val = li.tr.resets[s].second; t.deltaKnown = false;
                        }
                    }
                } else {
                    ctl.valKnown = true; ctl.val = 0; ctl.deltaKnown = false;
                    for (std::size_t a = 0; a < li.tr.accum.size(); ++a) {
                        Cell& t = st[p + li.tr.accum[a].first];
                        t.valKnown = false; t.deltaKnown = false;
                    }
                    for (std::size_t s = 0; s < li.tr.resets.size(); ++s) {
                        Cell& t = st[p + li.tr.resets[s].first];
                        t.valKnown = false; t.deltaKnown = false;
                    }
                }
            } else {
                return false;                                  // unknown nested loop
            }
            i = r + 1;
            continue;
        }

        return false;                                          // ']' or garbage
    }

    if (!ptrKnown || p != 0) return false;

    const std::map<int, Cell>::const_iterator it0 = st.find(0);
    if (it0 == st.end()) return false;
    if (!it0->second.deltaKnown || it0->second.delta != 255) return false;  // needs -1

    TransferSummary ts;
    for (std::map<int, Cell>::const_iterator it = st.begin(); it != st.end(); ++it) {
        const int   off = it->first;
        const Cell& c   = it->second;
        if (off == 0) continue;

        if (c.deltaKnown) {
            if (c.delta != 0)                        ts.accum.push_back(std::make_pair(off, c.delta));
            else if (c.valKnown)                     ts.resets.push_back(std::make_pair(off, c.val));
            // delta 0 and no known value: cell is untouched
        } else {
            if (c.valKnown)                          ts.resets.push_back(std::make_pair(off, c.val));
            else                                     return false;   // cannot summarise
        }
    }

    out = ts;
    return true;
}

// ===========================================================================
//  computeMaxOffset(): largest fixed cell offset any optimised op touches.
//  Only used with --bounds-check, to pad the tape with guard cells so that an
//  access such as 1(%r13) can never leave the allocation.
// ===========================================================================
void BFCompiler::computeMaxOffset() {
    maxOff_ = 0;
    if (!boundsCheck_) return;
    for (std::size_t i = 0; i < ops_.size(); ++i) {
        const Op& op = ops_[i];
        if (op.kind == 'A' || op.kind == 'B' || op.kind == 'C' || op.kind == 'D') {
            if (maxOff_ < 1) maxOff_ = 1;
        } else if (op.kind == 'M') {
            const TransferSummary& t = transfers_[static_cast<std::size_t>(op.value)];
            for (std::size_t a = 0; a < t.accum.size(); ++a) {
                const int o = t.accum[a].first < 0 ? -t.accum[a].first : t.accum[a].first;
                if (o > maxOff_) maxOff_ = o;
            }
            for (std::size_t r = 0; r < t.resets.size(); ++r) {
                const int o = t.resets[r].first < 0 ? -t.resets[r].first : t.resets[r].first;
                if (o > maxOff_) maxOff_ = o;
            }
        }
    }
}

// ===========================================================================
//  emitBoundsCheck(): trap if the data pointer left [r12, r12 + tapeSize).
//  %r12 holds the (guard-padded) tape base and is never modified.
// ===========================================================================
void BFCompiler::emitBoundsCheck(std::ostream& out, int oobLabel) const {
    if (!boundsCheck_) return;
    out << "    cmpq    %r12, %r13\n";
    out << "    jb      .L" << oobLabel << "\n";
    out << "    leaq    " << target_->symPrefix << "tape+" << (maxOff_ + tapeSize_)
        << "(%rip), %rax\n";
    out << "    cmpq    %rax, %r13\n";
    out << "    jae     .L" << oobLabel << "\n";
}

// ===========================================================================
//  generate(): AT&T syntax.
//
//  Stack layout (frame keeps %rsp 16-byte aligned at every call site):
//
//      entry            rsp % 16 == 8      (return address pushed)
//      push %rbp        rsp % 16 == 0
//      push %r12        rsp % 16 == 8
//      push %r13        rsp % 16 == 0
//      sub  $32         rsp % 16 == 0      (Windows shadow space only)
//
//  => %rsp is 16-byte aligned immediately before each "call", as both the
//     Microsoft x64 and System V AMD64 ABIs require.
// ===========================================================================
void BFCompiler::generate(std::ostream& out) const {
    const Target& T = *target_;
    const std::string S(T.symPrefix);
    const std::string mainSym = S + "main";
    const std::string tapeSym = S + "tape";
    const std::string putSym  = S + "putchar";
    const std::string getSym  = S + "getchar";

    out << "    .file   \"" << sourceName_ << "\"\n";
    out << "    .text\n";
    out << "    .globl  " << mainSym << "\n";
    if (T.os == TargetOS::Elf)
        out << "    .type   " << mainSym << ", @function\n";
    out << mainSym << ":\n";
    out << "    pushq   %rbp\n";
    out << "    movq    %rsp, %rbp\n";
    out << "    pushq   %r12\n";
    out << "    pushq   %r13\n";
    if (T.shadowSpace)
        out << "    subq    $32, %rsp\n";
    if (maxOff_ > 0)
        out << "    leaq    " << tapeSym << "+" << maxOff_ << "(%rip), %r12\n";
    else
        out << "    leaq    " << tapeSym << "(%rip), %r12\n";
    out << "    movq    %r12, %r13\n";
    out << "\n";

    std::vector<int> loops;   // stack of open-loop label ids
    int nextLabel = 0;
    const int oobId     = boundsCheck_ ? nextLabel++ : -1;   // out-of-bounds trap
    const int allocSize = tapeSize_ + 2 * maxOff_;           // guard cells for +/-off

    for (std::size_t oi = 0; oi < ops_.size(); ++oi) {
        const Op& op = ops_[oi];
        switch (op.kind) {

        case '>':
            out << "    addq    $" << op.value << ", %r13\n";
            emitBoundsCheck(out, oobId);
            break;
        case '<':
            out << "    subq    $" << op.value << ", %r13\n";
            emitBoundsCheck(out, oobId);
            break;

        case '+': {
            const int n = norm256(op.value);
            if (n) out << "    addb    $" << n << ", (%r13)\n";
            break;
        }
        case '-': {
            const int n = norm256(op.value);
            if (n) out << "    subb    $" << n << ", (%r13)\n";
            break;
        }

        case '.':
            out << "    movzbl  (%r13), " << T.arg0 << "\n";
            out << "    call    " << putSym << "\n";
            break;
        case ',':
            out << "    call    " << getSym << "\n";
            out << "    movb    %al, (%r13)\n";
            break;

        case '[': {
            const int id = nextLabel++;
            loops.push_back(id);
            out << ".L" << id << ":\n";
            out << "    cmpb    $0, (%r13)\n";
            out << "    je      .L" << id << "_end\n";
            break;
        }
        case ']': {
            const int id = loops.back();
            loops.pop_back();
            out << "    jmp     .L" << id << "\n";
            out << ".L" << id << "_end:\n";
            break;
        }

        // ---- peephole expansions -----------------------------------------
        case 'Z':  // [-]
            out << "    movb    $0, (%r13)\n";
            break;
        case 'A':  // [->+<]
            out << "    movzbl  (%r13), %eax\n";
            out << "    addb    %al, 1(%r13)\n";
            out << "    movb    $0, (%r13)\n";
            break;
        case 'B':  // [->-<]
            out << "    movzbl  (%r13), %eax\n";
            out << "    subb    %al, 1(%r13)\n";
            out << "    movb    $0, (%r13)\n";
            break;
        case 'C':  // [-<+>]
            out << "    movzbl  (%r13), %eax\n";
            out << "    addb    %al, -1(%r13)\n";
            out << "    movb    $0, (%r13)\n";
            break;
        case 'D':  // [-<->]
            out << "    movzbl  (%r13), %eax\n";
            out << "    subb    %al, -1(%r13)\n";
            out << "    movb    $0, (%r13)\n";
            break;

        case 'S': {   // scan loop: while (*p) p += step;
            const int  d    = op.value;
            const bool fwd  = d > 0;
            const int  step = fwd ? d : -d;
            const int  id   = nextLabel++;
            out << "    cmpb    $0, (%r13)\n";           // [ ... ] entry test
            out << "    je      .L" << id << "_end\n";
            out << ".L" << id << ":\n";
            out << "    " << (fwd ? "addq    $" : "subq    $") << step << ", %r13\n";
            emitBoundsCheck(out, oobId);
            out << "    cmpb    $0, (%r13)\n";
            out << "    jne     .L" << id << "\n";
            out << ".L" << id << "_end:\n";
            break;
        }

        case 'M': {   // general transfer loop
            const TransferSummary& t = transfers_[static_cast<std::size_t>(op.value)];
            const bool guard = !t.resets.empty();     // reset cells only when V != 0
            const int  id    = nextLabel++;
            out << "    movzbl  (%r13), %eax\n";
            if (guard) {
                out << "    testb   %al, %al\n";
                out << "    je      .L" << id << "_end\n";
            }
            for (std::size_t a = 0; a < t.accum.size(); ++a) {
                const int off = t.accum[a].first;
                const int cf  = t.accum[a].second;
                if (cf == 1) {
                    out << "    addb    %al, " << off << "(%r13)\n";
                } else if (cf == 255) {
                    out << "    subb    %al, " << off << "(%r13)\n";
                } else {
                    out << "    imull   $" << cf << ", %eax, %edx\n";
                    out << "    addb    %dl, " << off << "(%r13)\n";
                }
            }
            for (std::size_t s = 0; s < t.resets.size(); ++s)
                out << "    movb    $" << t.resets[s].second << ", " << t.resets[s].first << "(%r13)\n";
            out << "    movb    $0, (%r13)\n";
            if (guard)
                out << ".L" << id << "_end:\n";
            break;
        }

        default:
            break;   // unreachable
        }
    }

    out << "\n";
    out << "    xorl    %eax, %eax\n";             // main() returns 0
    if (T.shadowSpace)
        out << "    addq    $32, %rsp\n";
    out << "    popq    %r13\n";
    out << "    popq    %r12\n";
    out << "    popq    %rbp\n";
    out << "    ret\n";
    if (T.os == TargetOS::Elf)
        out << "    .size   " << mainSym << ", .-" << mainSym << "\n";

    // ---- bounds-check trap: reached only when %r13 leaves the tape -------
    if (boundsCheck_) {
        out << "\n.L" << oobId << ":\n";
        out << "    movl    $2, " << T.arg0 << "\n";
        out << "    call    " << S << "exit\n";
    }

    // ---- tape: zero-filled data area in .bss -----------------------------
    if (T.os == TargetOS::MacOS)
        out << "\n    .section __DATA,__bss\n";
    else if (T.os == TargetOS::Windows)
        out << "\n    .section .bss\n";
    else
        out << "\n    .bss\n";
    out << "    .p2align 4\n";
    if (T.os == TargetOS::Elf) {
        out << "    .type   " << tapeSym << ", @object\n";
        out << "    .size   " << tapeSym << ", " << allocSize << "\n";
    }
    out << "    .globl  " << tapeSym << "\n";
    out << tapeSym << ":\n";
    out << "    .zero   " << allocSize << "\n";

    if (T.gnuStackNote)
        out << "\n    .section .note.GNU-stack,\"\",@progbits\n";
}

// ---------------------------------------------------------------------------
//  Driver helpers
// ---------------------------------------------------------------------------
static std::string replaceExtension(const std::string& path, const std::string& newExt) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::size_t dot   = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return path + newExt;
    return path.substr(0, dot) + newExt;
}

// Split a --cc value on whitespace so that e.g. "clang -arch x86_64" works.
static std::vector<std::string> splitWords(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static std::string joinWords(const std::vector<std::string>& v) {
    std::string s;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) s += ' ';
        s += v[i];
    }
    return s;
}

// Run a program with an explicit argument vector.  No shell is involved, so
// file names cannot inject commands.
static int runProcess(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (std::size_t i = 0; i < args.size(); ++i)
        argv.push_back(const_cast<char*>(args[i].c_str()));
    argv.push_back(nullptr);

#if defined(_WIN32)
    const intptr_t rc = _spawnvp(_P_WAIT, argv[0], argv.data());
    return static_cast<int>(rc);
#else
    const pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) { /* retry */ }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 128;
#endif
}

static void usage(const char* prog) {
    std::cerr
        << "usage: " << prog << " <input.bf> [-o output]\n"
        << "       [--target NAME] [--cc CMD] [--tape-size N] [--bounds-check]\n"
        << "       [--no-link|-S] [--compile-only|-c]\n"
        << "       " << prog << " --version | --targets\n";
}

int main(int argc, char** argv) {
    std::string input;
    std::string output;
    std::string targetName  = "auto";
    std::string cc          = "g++";
    int         tapeSize    = kTapeSize;
    bool        noLink      = false;
    bool        compileOnly = false;
    bool        boundsCheck = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-o") {
            if (i + 1 >= argc) { std::cerr << "error: -o requires an argument\n"; return 1; }
            output = argv[++i];
        } else if (a == "--target" || a == "-t") {
            if (i + 1 >= argc) { std::cerr << "error: --target requires an argument\n"; return 1; }
            targetName = argv[++i];
        } else if (a == "--cc") {
            if (i + 1 >= argc) { std::cerr << "error: --cc requires an argument\n"; return 1; }
            cc = argv[++i];
        } else if (a == "--tape-size") {
            if (i + 1 >= argc) { std::cerr << "error: --tape-size requires an argument\n"; return 1; }
            const std::string v = argv[++i];
            char* endp = nullptr;
            const long n = std::strtol(v.c_str(), &endp, 10);
            if (endp == v.c_str() || *endp != '\0' || n <= 0 || n > (1L << 28)) {
                std::cerr << "error: invalid --tape-size '" << v << "' (expected 1..268435456)\n";
                return 1;
            }
            tapeSize = static_cast<int>(n);
        } else if (a == "--bounds-check") {
            boundsCheck = true;
        } else if (a == "--compile-only" || a == "-c") {
            compileOnly = true;
        } else if (a == "--no-link" || a == "-S") {
            noLink = true;
        } else if (a == "--version" || a == "-V") {
            std::cout << "bfc " << kVersion << "\n";
            return 0;
        } else if (a == "--targets") {
            for (int t = 0; t < kTargetCount; ++t) std::cout << kTargets[t].name << "\n";
            return 0;
        } else if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        } else if (a.size() > 1 && a[0] == '-') {
            std::cerr << "error: unknown option '" << a << "'\n";
            return 1;
        } else if (input.empty()) {
            input = a;
        } else {
            std::cerr << "error: more than one input file\n";
            return 1;
        }
    }

    if (input.empty()) { usage(argv[0]); return 1; }

    const Target* target = nullptr;
    if (targetName == "auto") {
        const char* host = hostTargetName();
        if (!host) {
            std::cerr << "error: this host architecture is not served by 'auto'; "
                         "pass --target and a matching --cc (see --targets)\n";
            return 1;
        }
        target = findTarget(host);
    } else {
        target = findTarget(targetName);
        if (!target) {
            std::cerr << "error: unknown target '" << targetName << "' (see --targets)\n";
            return 1;
        }
    }

    const std::string asmPath = replaceExtension(input, ".s");
    if (output.empty())
        output = compileOnly ? replaceExtension(input, ".o")
                             : replaceExtension(input, target->exeSuffix);

    try {
        BFCompiler compiler(input, *target, tapeSize, boundsCheck);
        compiler.parse(input);
        compiler.optimize();

        std::ofstream out(asmPath, std::ios::binary);
        if (!out) throw std::runtime_error("cannot write assembly file: " + asmPath);
        compiler.generate(out);
        out.close();
        if (!out) throw std::runtime_error("failed while writing assembly file: " + asmPath);

        std::cout << "[bfc] " << input << " -> " << asmPath
                  << "  (" << compiler.ops().size() << " ops, target " << target->name
                  << ", tape " << tapeSize << (boundsCheck ? ", bounds-check" : "") << ")\n";

        if (noLink) return 0;

        std::vector<std::string> args = splitWords(cc);
        if (args.empty()) { std::cerr << "error: --cc is empty\n"; return 1; }

        if (compileOnly) {
            args.push_back("-c");
            args.push_back("-o");
            args.push_back(output);
            args.push_back(asmPath);
        } else {
            args.push_back("-O2");
            if (target->staticLink) args.push_back("-static");
            args.push_back("-o");
            args.push_back(output);
            args.push_back(asmPath);
        }

        std::cout << "[bfc] " << joinWords(args) << "\n";
        std::cout.flush();

        errno = 0;
        const int rc = runProcess(args);
        if (rc < 0) {
            std::cerr << "error: cannot run '" << args[0] << "': "
                      << std::strerror(errno) << "\n";
            return 1;
        }
        if (rc != 0) {
            std::cerr << "error: " << args[0] << " failed (exit code " << rc << ")\n";
            return 1;
        }

        std::cout << "[bfc] wrote " << output << "\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
