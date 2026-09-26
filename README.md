# bfc - Brainfuck -> x86-64 汇编编译器

[![License](https://img.shields.io/github/license/xiaoguo141106/bfc?style=flat-square&color=111111)](./LICENSE)
[![Stars](https://img.shields.io/github/stars/xiaoguo141106/bfc?style=flat-square&logo=github&color=111111)](https://github.com/xiaoguo141106/bfc/stargazers)
[![Forks](https://img.shields.io/github/forks/xiaoguo141106/bfc?style=flat-square&logo=github&color=111111)](https://github.com/xiaoguo141106/bfc/forks)

[English](README_en-us.md) | **简体中文**

C++17 编写。把 Brainfuck 编译成 x86-64 汇编（AT&T 语法），再驱动
g++ 汇编链接为只依赖操作系统运行时的独立可执行文件。

## 构建与使用

    g++ -std=c++17 -O2 -static -o bfc.exe bfc.cpp
    bfc.exe <input.bf> [-o output.exe]

* 中间产物：输入同名 .s（hello.bf -> hello.s）
* 最终产物：默认输入同名可执行文件，可用 -o 覆盖
* 内部执行：g++ -O2 -static -o "output" "input.s"

## 代码结构

    struct Op { char kind; int value; };
    class BFCompiler { void parse(); void optimize(); void generate(); };

parse 过滤元字符、配平校验、连续同向合并、相邻反向抵消；
optimize 做循环抽象解释；generate 按 Op.kind 输出汇编。

Op.kind：普通 BF 指令，以及
    Z  置零            A B C D  [->+<]/[->-<]/[-<+>]/[-<->]
    S  扫描循环        M        通用传输循环（transfers_ 下标）

## 优化

1. 元字符过滤。
2. 连续同向合并，相邻反向抵消并重新合并（+++--+ -> addb $2）。
3. [-] 与 [+] 折叠为 movb $0。
4. 六 token 移动模式 A/B/C/D。
5. 扫描循环 [<] / [>]（任意步长）：进入前判零 + 先移动再判零。
6. 通用乘法/复制/传输循环：任意偏移、任意系数、多目标
   （[->+++<] 用 imull，[->+>+<<] 复制）。
7. 嵌套循环整体折叠：自内向外求摘要，内层摘要代入外层分析。

## 循环分析

对每个循环体做抽象解释，每个数据单元记录
(valKnown,val) 与 (deltaKnown,delta)：值是否恒为常量、每轮增量是否恒为常量。
遇到嵌套循环先递归求其摘要再施加：控制值已知则精确施加，未知则污染目标，
clear 置 0，scan 放弃外层。

接受条件：指针净位移 0；控制单元每轮恰为 -1；其它被动过的单元要么常量累加，
要么被重置为常量。生成时累加用 imull/addb，reset 用 movb，控制单元置 0。

reset 型单元只在“循环体至少执行过一次”时才改变，因此所有 reset（无论来自
第几层嵌套）共用同一个 testb/je (V!=0) 守卫即可，语义完备。无法证明时保守
退回真实循环，不改变语义。

## x86-64 与跨平台

* r12 = tape 基址，r13 = 数据指针；tape 30000 字节在 .bss。
* 入口 rsp = 8 (mod 16)，push rbp/r12/r13 后为 0；Windows 再 sub $32 shadow space，
  调用点始终 16 字节对齐。
* 平台分支在编译期选择：Windows x64（无前缀，shadow space，-static）、
  Linux/*BSD（无前缀，-static）、macOS（下划线前缀，不加 -static）。

## 错误处理

* 文件打不开 -> 报错退出，码 1。
* [ / ] 不匹配 -> 报错退出，码 1，不产生可执行文件。

## 测试

    powershell -File tests\run_tests.ps1

26 个功能用例 + 2 个错误用例，全部与独立解释器逐字节比对（当前 28 passed, 0 failed）。
源文件在 tests/bf，生成的汇编分类存放到 tests/asm，详见 tests/README.md。
另以随机差分测试覆盖了通用传输循环与多层 reset 嵌套。g++ -Wall -Wextra 无警告。

## 许可证

本项目采用 GNU Affero General Public License v3.0 或更新版本
(AGPL-3.0-or-later)。SPDX 标识符：AGPL-3.0-or-later。

* 许可证全文：LICENSE
* 编译产物附加许可：LICENSE-EXCEPTION.md
* 贡献指南（DCO 签署、报告 bug）：CONTRIBUTING.md
* 安全策略（漏洞私密上报）：SECURITY.md

### 编译产物的许可

AGPL 本身没有编译器"输出例外"。为避免下游对自己编译出的产物产生许可顾虑，
本项目以 AGPLv3 第 7 条附加许可的形式，明确允许把 bfc 从你自己的 Brainfuck
源码生成的汇编 / 目标文件 / 可执行文件，按你选择的条款使用和分发，详见
LICENSE-EXCEPTION.md。该例外不适用于 bfc 编译器本身。

### 第三方组件

* libstdc++ / libgcc（-static 静态链入）：GPL-3.0-or-later WITH
  GCC-exception-3.1，明确允许链接与再分发。
* MinGW-w64 运行时与头文件：宽松许可（Public Domain / BSD / ZPL 类）。
* UCRT 的 api-ms-win-crt-* 与 KERNEL32：Windows 系统组件，不是随附库。
* bfc 在运行时调用外部 g++，仅为进程调用，不构成链接。

