# 版本号规划

[English](VERSIONING_en-us.md) | **简体中文**

本项目处于正式版之前的预发布阶段。下列规则自本文发布起生效；**所有已发布的标签不再改动**。

## 阶段总览

| 阶段 | 命名 | 用途 | 数量 |
|------|------|------|------|
| 预发布（正式版之前） | RC-n | 功能冻结、只修问题；序号从 1 起，不补零 | 不限 |
| 正式版 | YYYY-K | 对外发布，按季节 | 每年 4 个 |
| 正式版修复 | YYYY-K-F | 仅重大问题 | 按需 |
| 快照 | YYYY-K-SnapShot-n | 发布前的日常快照 | 1-3 |

- 已取消：Beta（由 RC 取代）与 PR（预正式版）。
- YYYY = 目标正式版年份；K = 季节编号；F = 修复号；n = 序号（不补零）。
- 示例：RC-1、RC-2、RC-3 ……

## 正式版：按季节，每年 4 个

| K | 季节 | 发布窗口 |
|---|------|----------|
| 1 | 春 | 3-5 月 |
| 2 | 夏 | 6-8 月 |
| 3 | 秋 | 9-11 月 |
| 4 | 冬 | 12 月 |

- K 由季节决定，某个季节未发布也不会顺延补位。
- **K 每年重置为 1**：冬季 YYYY-4 之后，下一个正式版是 (YYYY+1)-1。
- 示例：2026-1（春）、2026-2（夏）、2026-3（秋）、2026-4（冬）、
  2027-1（春）、2027-2（夏）……

## 正式版修复：YYYY-K-F

- 只有当某个正式版出现重大问题才发布，不引入新功能。
- 示例：2026-2 出重大问题 -> 2026-2-1；再出 -> 2026-2-2。

## 正式版 Release 必须附带预编译产物

从第一个正式版起，每个正式版在 GitHub 与 Codeberg 两个站点的 Release **必须**附上下表全部
预编译二进制。RC 与快照版不强制，但建议从 RC 阶段就开始附带，提前验证打包流程。

### 产物矩阵

| 文件 | 对应 --target | 构建位置 | 工具链 |
|------|---------------|----------|--------|
| bfc-<版本>-Windows-x64.exe | x86_64-windows | 本机 / CI | MinGW-w64 UCRT，g++ -static |
| bfc-<版本>-Linux-x86_64.tar.gz | x86_64-linux | 本机 / CI | g++ -static |
| bfc-<版本>-Linux-arm64.tar.gz | aarch64-linux | CI（ubuntu-24.04-arm） | 原生 g++ -static |
| bfc-<版本>-macOS-x86_64.tar.gz | x86_64-macos | CI（macos-26-intel） | clang |
| bfc-<版本>-macOS-arm64.tar.gz | aarch64-macos | CI（macos-26） | clang |
| SHA256SUMS | — | CI / 本机 | sha256sum |

- <版本> 就是标签本身：标签 RC-1 对应 bfc-RC-1-Windows-x64.exe，
  正式版标签 2026-1 对应 bfc-2026-1-Windows-x64.exe。
- 架构名用 uname -m 的写法：x86_64、arm64。
- Linux 与 macOS 用 .tar.gz 而不是裸二进制：从 Release 直接下载裸文件会丢执行位。
  包内为 bfc、LICENSE、LICENSE-EXCEPTION.md。
- macOS 不能 -static（系统只提供动态 libSystem，属系统组件，不计入第三方依赖）。
- macOS 产物固定部署目标：arm64 为 11.0，x86_64 为 10.15。
- macOS x86-64 已标记弃用，本版仍提供；后续版本可能移除。
- 每个产物必须先在对应平台跑通完整测试（30/30）才允许上传。

### 自动化

.github/workflows/release.yml 在推送 RC-* 或 20* 标签时自动：

1. 在五个 runner 上构建：Linux x86_64、Linux arm64、macOS x86_64、macOS arm64、Windows x64；
2. 校验源码里的 kVersion 与标签一致（不一致直接失败）；
3. 按上面的命名打包（Linux/macOS 为 tar.gz，含 LICENSE 与 LICENSE-EXCEPTION.md）；
4. 生成 SHA256SUMS；
5. 上传到该标签的 GitHub Release（Release 不存在则自动创建，RC 标记 prerelease）。

workflow_dispatch 只构建、不发版，用于验证流水线。

### 发布步骤

1. 确认 kVersion 已改成标签名并提交。
2. 打注解标签并推送：origin 配了 GitHub 与 Codeberg 两个 push URL，一次推送同时到两处。
3. release.yml 自动构建五平台产物并上传到 GitHub Release。
4. 在 Codeberg 建同名 Release 并上传同一批产物 + SHA256SUMS（或用令牌自动上传）。
5. Release 说明里写清每个文件对应的 --target 与最低系统要求。

## 快照循环：每个正式版发布之前

以「正在准备 2026-3（秋）」为例，依次发布：

1. SnapShot：2026-3-SnapShot-1 … 最多 3 个
2. 正式版：2026-3
3. 若出重大问题：2026-3-1

跨年时目标版本号跟随新年：2026-4（冬）发布后，快照从 2027-1 开始。

## 预发布阶段（当前）

- 命名 RC-n，序号从 1 起递增，不补零（RC-1、RC-2、RC-3 ……）。
- 取代原 Beta（beta-0.0.N）与按版本号的 RC（YYYY-K-RC-n）。
- 允许问题修复与必要改动；进入功能冻结后只修问题。
- 已发布的 beta-0.0.1 … beta-0.0.5 保留不动。
- 出口条件：核心功能齐全、CI 全绿、无已知高危缺陷、CLI 冻结。

## 版本号常量

bfc.cpp 中的 kVersion 必须与最新标签一致：

- 预发布：RC-n（例如 RC-1）
- 正式版：YYYY-K
- 修复版：YYYY-K-F

## 历史

beta-0.0.1 … beta-0.0.5 已发布，按既定规则保留：不改名、不重写。
