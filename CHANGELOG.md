# 更新日志

[English](CHANGELOG_en-us.md) | **简体中文**

本项目处于 beta 阶段，版本号不保证严格遵循语义化版本。

## beta 0.0.3

- 新增 --tape-size N：可配置 tape 字节数（默认 30000）
- 新增 --bounds-check：为数据指针生成边界检查，越界时以退出码 2 终止；
  并为 ±off 邻近访问预留 guard 区域，杜绝越界读写
- 新增 --compile-only（-c）：只生成目标文件 .o，不链接
- 安全：不再经 shell 调用工具链，改为参数向量直接 spawn（Windows 用
  _spawnvp，POSIX 用 fork + execvp），文件名与输出路径无法注入命令
- 错误信息更具体：区分「文件不存在」「权限不足」，并附 errno 文本
- 修复了一些已知的 BUG

## beta 0.0.2

- 支持了：Windows 8 / 8.1 / 10 / 11 (x64)、Linux (x86-64, glibc 与 musl)、
  macOS 10.15+ (x86-64)、FreeBSD / OpenBSD (x86-64)
- 新增运行时目标选择 --target，可在一种系统上为另一种系统生成汇编（交叉编译）：
  x86_64-windows / x86_64-linux / x86_64-freebsd / x86_64-macos
- 新增 --cc <命令> 指定汇编 / 链接器，--no-link（-S）只生成汇编，
  --targets 列出全部目标，--version 输出版本
- 新增跨平台构建与测试：Makefile、tests/run_tests.sh（POSIX sh）、
  GitHub Actions CI（ubuntu-latest / macos-13 / windows-latest）
- 修复了一些已知的 BUG：
  - 修复目标 ABI（符号前缀、参数寄存器、shadow space、-static）在编译 bfc
    时被写死、无法交叉编译的问题
  - 修复 macOS 目标 .align 语义有歧义的问题，改用 .p2align
  - 修复缺少版本号与目标列表的问题

不在支持范围：Windows XP（无 UCRT）、ARM64（Apple Silicon / 树莓派，需要
aarch64 后端）、DOS / z/OS / z/VSE / RTOS。

## beta 0.0.1

- 首个版本：Brainfuck -> x86-64 汇编编译器（C++17）
- 优化：元字符过滤、连续同向合并、相邻反向抵消、[-] / [+] 置零、
  [->+<] 等移动模式、扫描循环、通用乘法 / 复制 / 传输循环、嵌套循环整体折叠
- 26 个功能用例 + 2 个错误用例，与独立解释器逐字节比对
- 许可：AGPL-3.0-or-later，附编译产物输出例外
