# Contributing to bfc

感谢你对 bfc 的关注。

## 许可证

bfc 以 GNU Affero General Public License v3.0 或更新版本 (AGPL-3.0-or-later)
发布。提交贡献即表示你同意你的贡献也按该条款授权。见 LICENSE。

## 开发者原产地证书 (DCO)

本项目只要求 DCO，不需要签署额外的贡献者许可协议。
每个提交都必须带 DCO 签署：

    git commit -s -m "your message"

会追加一行：

    Signed-off-by: Your Name <you@example.com>

请使用真实姓名与可联系的邮箱。以下 DCO 1.1 全文对你有约束力。

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

## 报告 bug

请在 issue 里提供：

* 最小 BF 源文件（.bf），能复现问题即可
* 使用的 bfc 命令，以及完整输出
* 期望行为与实际行为
* 平台（操作系统 / 架构）与 g++ 版本（g++ --version）

如果是安全问题，请不要开公开 issue，改见 SECURITY.md。

## 开发约定

* C++17，无第三方依赖，要求 g++ -std=c++17 -Wall -Wextra 零警告。
* 保持公开结构：struct Op，class BFCompiler { parse, optimize, generate }。
* 提交 PR 前运行完整测试：

      powershell -File tests\run_tests.ps1

* 每个行为变更都要在 tests/bf 下新增用例，并更新 tests/run_tests.ps1
  里的期望输出表。
