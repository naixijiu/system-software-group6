# Lab 1 提示词记录（A、B、C 组员合并版）

本文件汇总当前已提交的 A、B、C 组员 AI 协作记录。A 部分保留原始短提示词及上下文；B 部分将阅读源码、核对符号表与反汇编过程中的提问按顺序归并；C 部分将连续对话按 Vibe Coding 的实际迭代过程科学归并为“目标—依据—约束—验收”式提示词。归并文本用于准确表达用户如何逐步引导 AI，不声称是每次聊天的逐字转录。AI 结论均使用源码、命令输出或截图进行人工核验。

## A 组员：环境、构建、链接与项目管理

### A01｜确认课程任务与实验环境

```text
我负责三人小组中的 A 部分，主要完成 Lab1 的环境与构建、Makefile 和 kernel.ld 分析、GitHub 项目管理。实验需要使用 RISC-V、QEMU 和 GDB，我使用 Windows WSL Ubuntu。请结合实验指导书，告诉我应当先准备哪些工具、如何验证环境，以及与组员的工作如何衔接。
```

实际过程：先明确三人共同完成基础实验、各自深入模块的分工，再安装 RISC-V GNU 工具链和 QEMU。本人承担编译链接分析、启动排错、报告整合与版本管理。

### A02｜排查 RISC-V 工具链路径问题

```text
我根据实验指导书推荐的 SiFive 下载链接获取了 RISC-V 工具链。在 Ubuntu 中解压时提示目标目录不存在；之后在 /opt/riscv 下能够找到 riscv64-unknown-elf-gcc，但执行 gcc --version 仍提示 command not found。请根据终端输出区分解压路径与 PATH 配置的问题，并告诉我怎样修改 ~/.bashrc 和检查安装是否成功。
```

实际过程：先区分 `~/riscv` 和 `/opt/riscv`，再发现 GCC 实际位于 `/opt/riscv/` 下多一层的工具链目录。将正确的 `bin` 路径加入 `PATH`，执行 `source ~/.bashrc` 后，GCC 与 GDB 的版本检查通过。

### A03｜准备 Lab1 源码和小组 GitHub 分支

```text
我已经拿到课程提供的 lab1 代码，小组仓库是 https://github.com/naixijiu/system-software-group6 。老师要求每次实验对应一个 labx 分支，并在分支中放置 code/ 和 report/；report/ 需要 report.md、prompt.md 与 images/。我使用 WSL Ubuntu，请指导我创建 lab1 分支，整理目录，并确认代码应放在哪里才可以正常执行 make。
```

实际过程：本地克隆仓库、创建 `lab1` 分支、建立 `code/` 和 `report/`；将实验源码置于 `code/`。检查 `pwd`、`git branch`、`ls` 后确认目录正确。

### A04｜确认 Makefile 构建产物并定位启动失败

```text
我在 code/ 目录运行 make，终端依次输出多个 + cc、+ ld bin/kernel，并通过 objcopy 生成 bin/ucore.img，构建没有报错。但 make qemu 只显示 OpenSBI，尚未看到 (THU.CST) os is loading ...；日志中的 Domain0 Next Address 是 0x0。请分析编译、链接、加载与执行的区别，并指导我用 readelf 和 make -n qemu 排查问题。
```

实际过程：`bin/kernel` 与 `bin/ucore.img` 均已生成；`readelf -h` 显示 ELF 入口地址为 `0x80200000`，而原 Makefile 使用 `-device loader,file=bin/ucore.img,addr=0x80200000`。说明 ELF 入口正确并不能单独保证 QEMU/OpenSBI 会跳转到该入口。

### A05｜验证修正后的 QEMU 加载方式并修改 Makefile

```text
我把 QEMU 启动方式改为 -bios default -kernel bin/kernel 后，OpenSBI 输出的 Domain0 Next Address 变为 0x80200000，并成功打印 (THU.CST) os is loading ...。请根据我贴出的 Makefile qemu、debug、gdb 目标，解释其中的依赖与参数，指导我做最小修改，并检查修正是否影响后续 GDB 调试。
```

实际过程：用成功的直接启动命令确认故障出在当前环境使用的镜像加载方式，随后修改 `qemu` 和 `debug` 目标并再次运行。曾经在用户主目录执行 `make gdb` 导致 `No rule to make target 'gdb'`，切换到 `code/` 目录后连接正常。后续 C 组员还修正了 `debug` 目标的续行反斜杠并适配其 GDB 工具。

### A06｜深入理解 Makefile 和 kernel.ld

```text
现在内核已经能运行、GDB 也能连接。请结合本次 Lab1 的实际 Makefile 和 tools/kernel.ld，逐段讲解：C 和汇编文件怎样编译成 .o，目标文件怎样链接成 ELF，objcopy 为什么还要生成 ucore.img，链接脚本如何规定 kern_entry、.text、.rodata、.data 以及为什么基地址使用 0x80200000。希望分析能用于我负责的实验报告，而不是只给出结论。
```

实际过程：学习了 `GCCPREFIX`、`CFLAGS`、`LDFLAGS`、`tools/function.mk`、`-T tools/kernel.ld`、`ENTRY(kern_entry)`、`BASE_ADDRESS` 与 `ALIGN(0x1000)` 等内容；区分链接地址、镜像加载地址和 CPU 实际执行入口。

### A07｜使用 readelf 和 nm 验证内存布局及启动栈

```text
我已经成功生成 bin/kernel。请帮我分析 readelf -S 与 nm -n 的真实输出，核对 .text、.rodata、.data、.sdata 的起始地址和大小；检查 kern_entry、kern_init、bootstack、bootstacktop、edata、end 等符号，计算内核启动栈大小，并说明这些结果如何验证 kernel.ld。若命令提示找不到文件，也请先检查当前工作目录。
```

实际过程：起初在仓库根目录执行 `readelf -S bin/kernel` 导致文件找不到，进入 `code/` 后验证成功。节地址显示 `.text=0x80200000`、`.rodata=0x802004c8`、`.data=0x80201000`、`.sdata=0x80203000`。符号表显示 `kern_entry=0x80200000`、`kern_init=0x8020000a`、`bootstack=0x80201000`、`bootstacktop=0x80203000`，故启动栈为 `0x2000=8192` 字节；`edata=end=0x80203008`。

### A08｜整理 Git 提交、截图及报告交付物

```text
现在 A 部分已经完成，请按照老师要求帮我检查 Git 工作区：lab1 分支是否正确，code/ 是否包含完整源代码，report/ 是否包含 report.md、prompt.md 和 images/，哪些编译产物需要通过 .gitignore 排除。实验报告中我准备使用内核成功启动、ELF 节表和 nm 符号表三张实际截图；请说明图片路径、Markdown 引用、Git 提交和推送前的检查方式，并协助整理我负责的报告部分。
```

实际过程：使用 `git status --short` 和 `git add -n` 检查未跟踪文件与待添加文件；通过 `.gitignore` 排除生成的编译产物；将三张截图放入 `report/images/`。在 HTTPS 推送时遇到 GitHub 不支持账户密码验证的问题，按提示改用个人访问令牌完成身份验证。最后将 A 部分与 B、C 后续提交的内容合并。

### A 部分经验总结

本次使用 AI 的核心方式是**先实际操作，再提供完整终端结果，要求分析原因，最后使用命令重新验证**。关键故障包括工具链 PATH 错误、命令执行目录错误、QEMU 加载方式与当前固件行为不匹配。真正证明问题解决的是 `gcc --version`、`make`、`make qemu`、`readelf`、`nm` 和 GDB 的实际输出，而不是 AI 给出的文字结论。

## B 组员：内核启动原理与练习一

> 下面是按实际过程归并的提示词，保留了先看代码、跑命令、再把结果拿给 AI 核对的节奏。

### B01｜首先对齐一下交付物

```text
我负责 B：内核启动原理和练习一。先带我看下 entry.S 和 init.c 这两份文件分别是干嘛的。
```

这一轮先对齐范围，没有直接让它写答案。

### B02｜先把环境搭起来

```text
我在 Windows 上装了 WSL 的 Ubuntu。这实验要装哪些东西？riscv 的编译器和 qemu 我都没有。
```

按建议装了 riscv64-linux-gnu 工具链、qemu-system-misc、gdb-multiarch 和 make。

### B03｜编译一直报错

```text
环境弄好了，代码放到 ~/lab1，make 之后一堆 Permission denied，这是怎么回事？
```

原因是复制文件时把只读权限也带过来了，`chmod -R u+w` 之后就正常编译了。

### B04｜先跑一下，看项目通不通

```text
编译过了，bin/kernel 和 ucore.img 都生成了。接下来怎么运行？怎么判断跑通？
```

用 `make qemu` 启动，OpenSBI 之后打印出 `(THU.CST) os is loading ...` 就算通了；QEMU 会占着终端，按 `Ctrl+A` 松开再按 `X` 退出。

### B05｜entry.S 这两行看不懂

```text
entry.S 里就两行，la sp, bootstacktop 和 tail kern_init，讲一下是什么意思？
```

它解释了 sp 是栈指针、bootstacktop 是内核栈的栈顶，tail 是直接跳过去、不回来的那种跳转。

### B05｜kern_init 里写了什么

```text
kern_init 里就 memset、cprintf 和死循环三行。edata 和 end 是哪来的？cprintf 是标准库的吗？
```

edata/end 是链接脚本给的段边界，清的是 .bss；内核没有标准库，cprintf 是仓库里自己写的。

### B06｜对着反汇编确认一下

```text
确认一下kern_entry 和 kern_init 那几行能跟源码对上吗
```

核对结果：la 拆成 auipc+mv，tail 是一条 j，kern_init 里 memset、cprintf 的顺序和源码一致。

### B07｜把启动流程串一下

```text
从加电到进 kern_init，是不是 0x1000 → OpenSBI → 0x80200000 → kern_init 这么走？为啥不能换个地址？
```

它讲了这是固件→引导→内核的三级接力，内核的地址被链接脚本写死了，所以必须在 0x80200000。

### B08｜整理成报告

```text
这是本次任务的仓库链接，请在A、C的基础上补充B部分的内容，包括需要补充的实验原理、部分截图、以及练习一的部分
```

最终，整理完report.md,prompt.md相应部分。

## C 组员：GDB 调试与练习二（根据真实对话归并）

> 下面不是重新虚构的一条“完美提示词”，而是把连续对话中重复的终端输出和追问适度合并。保留了本人先操作、贴现象、再根据反馈继续排错的过程。

### C01｜先把课程任务和交付要求讲清楚

```text
我在学习计算机系统软件基础（操作系统）这门课，接下来小组三个人要一起完成实验 1。我会把实验指导书、实验代码和助教的提交要求发给你，请继续帮我完成。

最终一个分支对应一次实验，包含 code 和 report；report 里要有 report.md、prompt.md 和 images，每个人还要讲各自负责的部分。
```

这一轮先让 AI 理解课程背景、目录要求和答辩目标，没有直接让它生成答案。

### C02｜根据我机器上的实际状态准备环境

```text
告诉我如何把可运行的 Ubuntu/WSL 环境准备好。

wsl --list --verbose 显示 Ubuntu-22.04、WSL 2，但是 wsl --update 返回 403。我现在还能不能继续做实验？
```

根据反馈，我没有继续纠结在线更新，而是先使用已经能运行的 Ubuntu 22.04 安装工具并编译项目。这样避免为了非必要的系统升级耽误实验。

### C03｜先跑起来，再根据输出判断下一步

```text
我执行 make qemu 后已经看到 OpenSBI，最后显示：
(THU.CST) os is loading ...

现在这样，然后呢？要退出 QEMU 吗，还是直接在终端修改？
```

AI 提醒我 QEMU 正占用当前终端，需要用 `Ctrl+A`、`X` 退出，修改代码或调试时再开第二个 Ubuntu 终端。我随后保存了第一张内核运行截图。

### C04｜GDB 连不上时持续提供现场信息

```text
我在 GDB 中执行 target remote localhost:1234，但是 Connection timed out。

另一个终端执行 make debug 后能看到 OpenSBI 和 os is loading。下面是 Makefile 的 debug 部分：

debug: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
        $(V)$(QEMU) \
                -machine virt \
                -nographic \
                -bios default \
                -kernel $(UCOREIMG)
                -s -S

请根据这些实际现象继续排查。
```

这次不是让 AI 猜原因，而是把超时信息、QEMU 现象和 Makefile 片段逐步补全。最终发现 `-kernel` 行末缺少反斜杠，导致 `-s -S` 没有传给 QEMU；修正后 GDB 成功停在 `0x1000`。Ubuntu 中没有指定的交叉 GDB 时，改用 `gdb-multiarch`。

### C05｜确认本人负责 C，再按分工收窄任务

```text
我刚才说错了，我负责 C 部分。

请帮助我理解并优秀完成 GDB 调试与练习二，并且最后能通过助教要求的答辩。我要观察 CPU 从复位地址 0x1000 到内核入口 0x80200000，分析最初执行的指令，观察 PC、SP 等寄存器变化，整理调试过程、结果、截图和问题分析。
```

这一轮明确只深入 C 的任务。B 尚未提交，因此最终报告中练习一继续沿用 A 原报告的“由 B 组员补充”占位。

### C06｜把真实 GDB 结果继续交给 AI 分析

```text
GDB 连接后，pc 是 0x1000，sp、a0、a1、a2 都是 0。x/6i $pc 显示：
auipc、addi、csrr、ld、ld、jr。

然后我在 0x80200000 设置断点并继续运行。断点命中 kern_entry，pc=0x80200000，sp=0x80017ee0，a0=0，a1=0x87000000，a2=7。

请告诉我接下来怎样单步验证 la sp, bootstacktop 和 tail kern_init，并解释这些数值说明了什么。
```

按照给出的命令继续执行后，我查到 `bootstack = 0x80201000`、`bootstacktop = 0x80203000`；单步执行使 `sp` 变为 `0x80203000`，随后 PC 进入 `0x8020000a <kern_init>`。这些值来自本机 GDB，而不是 AI 生成的示例。

### C07｜在 A 的实际成果上合并，而不是重写整份报告

```text
这是 A 组员已经上传到 GitHub 的 prompt.md、report.md 和三张 images。我将继续在他的工作之上完成我的 C 部分；如果 code 也需要修改，请明确告诉我修改了什么。

请保留 A 已完成的编译、链接和截图内容，只加入我的练习二、GDB 调试过程、截图与答辩准备。B 还没有提交，B 部分继续使用 A 原来的占位。
```

最终合并时保留了 A 的数据和三张截图，加入了 C 的两张 GDB 截图与运行截图。代码方面，A 和 C 都只修改了 `Makefile`：A 调整内核加载方式，C 补齐调试命令续行并适配 `gdb-multiarch`；内核 C/汇编源码没有修改。

## 后续记录规则

每次继续使用 AI 时追加以下信息：

1. 原始提示词；
2. 使用的工具与模型；
3. AI 输出是否直接采用；
4. 若出现编译、运行或理解偏差，记录错误现象和下一次提示词如何修正；
5. 最终结论由哪条命令、代码或截图验证。

## 合并后的关键迭代记录

1. 首次使用 `-device loader` 时 OpenSBI 的下一阶段地址为 0，改用 `-kernel` 后内核成功运行。
2. 首次 `make debug` 时因 Makefile 续行反斜杠缺失，`-s -S` 未生效；修正后 PC 成功停在 `0x1000`。
3. 使用 `gdb-multiarch` 代替环境中不存在的 `riscv64-unknown-elf-gdb`。
4. 最终以两张实际 GDB 截图验证复位跳板、内核入口和 SP 切换。
5. A、C 使用的 OpenSBI 与 GDB 版本不同，因此报告分别标注各自环境，不把版本差异误写成实验矛盾。
6. 最终结论以源码、`readelf`/`nm` 输出、GDB 寄存器和反汇编结果共同验证；AI 文本不作为唯一证据。
