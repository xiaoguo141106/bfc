## bfc beta 0.0.4 — ARM64

### 新增
- aarch64（ARM64）后端
  - --target aarch64-linux：树莓派 4/5、ARM 服务器、ARM 版 WSL 等
  - --target aarch64-macos：Apple Silicon
- --target auto 现在能识别 aarch64 宿主

### 修复
- 修复 AArch64 循环回边跳到循环体、导致循环条件不再判断的问题（此前会死循环并越界）
- Mach-O 的 tape 取址改用 @PAGE / @PAGEOFF 重定位

### 验证
- Linux ARM64：CI 在原生 arm64 runner 上构建并跑完整测试，28/28 通过
- macOS ARM64（Apple Silicon）：CI 在原生 arm64 runner 上通过
- Linux x86-64 与 Windows x64：本机 28/28

### 关于预编译产物
本版暂未附带预编译二进制。按版本号规划，从**第一个正式版**起，
每个正式版 Release 都会附带 Windows x64、Linux x86_64/arm64、macOS x86_64/arm64
以及 SHA256SUMS，详见 VERSIONING.md。

### CI
Linux / Linux-ARM64 / macOS / macOS-ARM64 / Windows 共五个任务，全部启用 -Werror。

### 链接
- 完整变更 https://github.com/xiaoguo141106/bfc/blob/main/CHANGELOG.md
- 版本号规划 https://github.com/xiaoguo141106/bfc/blob/main/VERSIONING.md
