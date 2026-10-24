# 更新日志

[English](CHANGELOG_en-us.md) | **简体中文**

本项目处于 beta 阶段，版本号不保证严格遵循语义化版本。

## RC-1

- 版本号体系调整：正式版之前的预发布阶段统一用 RC-n（从 RC-1 起，不补零），
  取代 Beta；SnapShot 缩减为 1-3 个；取消 PR（预正式版）
- CI 与发布：macOS 构建迁移到 macOS 26 Tahoe 镜像（macos-26 / macos-26-intel），
  重新回到原生构建；固定部署目标（arm64 11.0、x86_64 10.15）
- 新增发布产物流水线 .github/workflows/release.yml：推送 RC-* 或 20* 标签时，
  自动构建五平台产物 + SHA256SUMS 并上传到 GitHub Release
- macOS x86-64 标记为弃用（本版仍支持并发布）

## beta 0.0.5

- 安全与健壮性
  - 解析器限制括号嵌套深度为 10000（kMaxNesting，编译期常量，不是命令行参数）；
    超出时编译失败并给出明确错误，而不是让编译器栈溢出（修复前约一万层嵌套即可崩溃）
- 工程
  - CI 新增 sanitizer 任务：AddressSanitizer + UndefinedBehaviorSanitizer 构建后跑完整测试，
    并断言 libasan/libubsan 确实被链接
  - Makefile 新增 sanitize / test-san 目标；测试运行器支持用 BFC= 指定被测二进制
  - 测试用例 28 → 30：新增 nest_deep_ok（1000 层嵌套应通过）与 nest_limit（超限应被拒绝）
  - 仓库开始同时托管在 GitHub 与 Codeberg，两站同步发布（每次发版两站都建 Release）
  - VERSIONING.md 明确：从第一个正式版起，Release 必须附带预编译产物
    （Windows x64、Linux x86_64/arm64、macOS x86_64/arm64 以及 SHA256SUMS）
- 版权信息统一为 "Xiaoguo and bfc contributors"（bfc.cpp 与 LICENSE-EXCEPTION.md）

## beta 0.0.4

- 新增 aarch64（ARM64）后端
  - --target aarch64-linux：树莓派 4/5、ARM 服务器、ARM 版 WSL 等
  - --target aarch64-macos：Apple Silicon（CI 已在原生 arm64 的 macos-latest 上跑通）
- aarch64-linux 已在 x86-64 Linux 上用交叉 g++ 加 qemu-aarch64 跑通全部 28 个用例
- --target auto 现在能识别 aarch64 宿主
- CI：Linux 与 Linux-ARM64 拆分为独立任务，ARM64 任务从本版起跑完整测试；
  macOS ARM64 已在原生 arm64 runner 上通过，故不再是实验性任务
- 修复了一些已知的 BUG：
  - 修复 AArch64 循环回边跳到循环体、导致循环条件不再判断的问题
  - 修复 AArch64 跨年/跨页取址在 Mach-O 上的重定位写法（@PAGE / @PAGEOFF）

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
