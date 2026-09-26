# bfc 测试套件

## 运行

    powershell -File tests\run_tests.ps1   # Windows
    sh tests/run_tests.sh                 # Linux / macOS / BSD
    make test                             # 两者皆可

脚本会：编译 tests/bf 下每个 .bf，把生成的 .s 归入 tests/asm，
运行可执行文件并与内置期望值逐字节比较，校验括号错误被拒绝，
最后删除临时目录 tests/bin。全部通过时退出码为 0。

## 目录

    tests/
      bf/             BF 源文件（26 个用例 + 2 个错误用例 + echo 输入）
      asm/            生成的汇编（与源文件同名；已 git 忽略，由脚本生成）
      run_tests.ps1   一键编译 + 运行 + 校验

## 用例与期望输出

| 用例 | 覆盖点 | 期望输出(hex) |
|------|--------|---------------|
| A | 65 个 '+' 合并 | 41 |
| move | [->+<] 移动模式 | 41 |
| hello | 综合；输出 Hello World! 换行 | 48 65 6C 6C 6F 20 57 6F 72 6C 64 21 0A |
| run_merge | 连续同向合并 (10 个 '>') | 03 |
| multiply | [->+++<] 乘法循环 | 0C |
| copy | [->+>+<<] 复制循环 | 05 05 |
| move_b / move_c / move_d | [->-<] / [-<+>] / [-<->] | 41 |
| scan_right / scan_left / scan_guard | [>] / [<] 扫描与进入前判零 | 01 |
| clear_plus | [+] 置零 | 05 |
| cancel_add | +- 抵消后重合并 | 02 |
| cancel_move | >< 抵消 | 01 |
| dots | 连续 '.' 不被合并 | 01 01 02 02 |
| nested_fold | 嵌套整体折叠 | 40 |
| nested_kept | 无法证明常量时保留真实循环 | 10 |
| nested_guard | 单层 reset 守卫 | 07 |
| nested_reset | 两层嵌套 + reset 折叠 | 00 00 00 00 00 08 00 |
| nested_deep | 三层嵌套 + 两层 reset 整体折叠 | 00 00 00 00 00 00 18 |
| nested_v0 | 外层 V==0 时 reset 单元保持不变 | 00 00 00 00 07 05 00 |
| wrap255 / wrap300 | 8 位回绕 | FF / 2C |
| comment | 非 BF 字符过滤 | 41 |
| echo | ',' 输入回显 | 5A |
| err_open / err_close | '[' / ']' 不匹配应被拒绝（退出码非 0） | - |

## 关于多层 reset 嵌套

外层循环 L 的摘要里，reset 型单元表示“只要 L 的循环体执行过一次，
该单元就会被重置为某个常量”。循环体执行与否只取决于 L 的控制单元 V 是否为 0，
与 reset 来自第几层嵌套无关：内层循环如果能折叠进 L 的摘要，它的控制值必然是
编译期已知的非零常量，因此内层一定会在每次 L 迭代中执行。

所以“所有 reset 共用一个 testb/je (V!=0) 守卫”就是正确且完备的，
不存在“内层 reset 需要单独守卫”的情况。

nested_deep 是三层嵌套、两层 reset 的最小可折叠例子：

    [->[-]++[->[-]++[->++<]<]<]

它整体折叠为：

    addb    $3, (%r13)          ; V = 3
    movzbl  (%r13), %eax
    testb   %al, %al
    je      .L0_end
    imull   $8, %eax, %edx
    addb    %dl, 3(%r13)
    movb    $0, 1(%r13)
    movb    $0, 2(%r13)
    movb    $0, (%r13)
    .L0_end:

V==0 时整块跳过，三个 reset 都不会执行，与解释器一致。

## 许可

本目录下所有文件随 bfc 一并按 AGPL-3.0-or-later 授权，除非另有说明。
tests/asm 中由 bfc 生成的汇编适用 LICENSE-EXCEPTION.md 的输出附加许可。

## hello.bf 来源

tests/bf/hello.bf 是 Brainfuck 语言中广为流传的经典 Hello World 程序，
属于该语言最常见的示例代码，原始作者与出处已不可考。此处仅作为编译器的
回归测试输入使用，按公共领域（public domain / 事实性 trivia）处理。
若权利人认为不妥，请提交 issue，我们会移除或替换该文件。

