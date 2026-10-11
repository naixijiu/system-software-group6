# Lab1 实验提示词（四段式规范版）

# 第一部分：按课程框架重构的最终提示词

## A 组员：构建、链接与内核加载

### A1. 修正 QEMU 启动与调试规则

[PROMPT]

**任务：** 检查 `code/Makefile` 中 `qemu`、`debug`、`gdb` 三个目标，解决 Lab1 已成功编译却无法进入 ucore 内核的问题，并保持 GDB 调试可用。

**操作要求：** 若具有仓库写权限，直接对真实 `code/Makefile` 做最小改动；否则给出准确补丁与文件行附近的修改指导，由使用者实际修改。不改变 `kern/`、`libs/`、`tools/kernel.ld` 的内核语义，保留已有规则。

**输出要求：** 说明启动失败原因、最终修改和验证命令；以实际 QEMU/OpenSBI 输出为准，不以编译成功替代启动成功。

[RELY]

以下内容来自本组 `lab1` 分支的源码及实验日志：

```makefile
GCCPREFIX := riscv64-unknown-elf-
QEMU := qemu-system-riscv64
GDB := $(GCCPREFIX)gdb
kernel = $(call totarget,kernel)
UCOREIMG := $(call totarget,ucore.img)
$(UCOREIMG): $(kernel)
	$(OBJCOPY) $(kernel) --strip-all -O binary $@
```

```ld
ENTRY(kern_entry)
BASE_ADDRESS = 0x80200000;
```

- `bin/kernel` 为 ELF；`bin/ucore.img` 为原始二进制镜像。
- 原 `-device loader,file=bin/ucore.img,addr=0x80200000` 启动方式下，OpenSBI `Domain0 Next Address = 0x0`，未打印内核输出。
- 手动用 `-bios default -kernel bin/kernel` 时，下一阶段地址变为 `0x80200000`，并打印 `(THU.CST) os is loading ...`。

[GUARANTEE]

**需修改/检查的构建目标（非 C 函数）：**

```makefile
qemu: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
debug: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
gdb:
```

- `qemu` 能正确运行内核。
- `debug` 沿用有效的加载方式并传递 `-s -S`；必须保证 Makefile 续行正确。
- `gdb` 能加载 `bin/kernel` 调试符号并连接 `localhost:1234`。成员间可按已安装工具选用交叉 GDB 或 `gdb-multiarch`。
- 无需新增函数或修改 `kern_entry`、`kern_init`。

[SPECIFICATION]

**目标：`qemu`**

**Pre-Condition：** 位于 `code/`，RISC-V 工具链和 QEMU 可执行，构建依赖正常。

**Post-Condition：** OpenSBI 能向 `0x80200000` 转交执行，内核打印指定启动字符串。

**Case 1：只启动 OpenSBI。** 检查 ELF 入口和 `make -n qemu`，区分镜像被加载与固件下一阶段入口被正确指定；使用经过验证的 ELF `-kernel $(kernel)` 方式修复。

**Case 2：内核正常打印。** 截图保存 `Next Address` 与内核启动信息。

**Requirements：** 保留现有内核构建流程，不修改 C/汇编以掩盖加载参数问题。

**目标：`debug` / `gdb`**

**Pre-Condition：** 两个终端处于 `code/`，调试端口可用。

**Post-Condition：** `make debug` 使用 `-s -S` 暂停 CPU，GDB 可在复位地址 `0x1000` 连接。

**Case 1：连接超时。** 检查 `-kernel $(kernel)` 行末续行反斜杠是否使 `-s -S` 真正传给 QEMU。

**Case 2：`No rule to make target 'gdb'`。** 先检查执行目录是否存在 Lab1 的 Makefile。

**Requirements：** 保存修改前后的命令、实际失败现象及成功验证，不能把假定输出写成实测。

**真实迭代与证据：** 修改加载方式后 `make qemu` 成功；随后补齐调试参数续行，GDB 成功连接。对应 `boot-success.png`、`gdb_reset_0x1000.jpg`。

---

### A2. 分析构建产物和内存布局

[PROMPT]

**任务：** 研读 `code/Makefile` 和 `code/tools/kernel.ld`，解释交叉编译、链接和镜像转换，以及内核入口、各 ELF 节和启动栈如何布局；用 `readelf`、`nm` 验证。

**操作要求：** 分析任务不修改源文件；严格使用实际宏、规则和符号，不引入并不存在的数据结构。

**输出要求：** 提供编译流程、主要地址表、链接脚本逐项解释和可复现的验证命令。

[RELY]

```makefile
CC := $(GCCPREFIX)gcc
LD := $(GCCPREFIX)ld
OBJCOPY := $(GCCPREFIX)objcopy
include tools/function.mk
$(kernel): $(KOBJS)
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
$(UCOREIMG): $(kernel)
	$(OBJCOPY) $(kernel) --strip-all -O binary $@
```

```ld
OUTPUT_ARCH(riscv)
ENTRY(kern_entry)
BASE_ADDRESS = 0x80200000;
SECTIONS {
  . = BASE_ADDRESS;
  .text : { *(.text.kern_entry .text .stub .text.* .gnu.linkonce.t.*) }
  .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  . = ALIGN(0x1000);
  .data : { *(.data) *(.data.*) }
  .sdata : { *(.sdata) *(.sdata.*) }
  PROVIDE(edata = .);
  .bss : { *(.bss) *(.bss.*) *(.sbss*) }
  PROVIDE(end = .);
}
```

`entry.S` 使用 `.space KSTACKSIZE` 在 `bootstack` 和 `bootstacktop` 间保留启动栈。

[GUARANTEE]

无新增函数。**需核对的既有符号：** `kern_entry`、`kern_init`、`bootstack`、`bootstacktop`、`edata`、`end`。

**交付物：** 构建流程解释，ELF 节布局表、栈大小计算和对应截图。

[SPECIFICATION]

**任务：ELF 布局与符号验证**

**Pre-Condition：** 在 `code/` 下成功构建 `bin/kernel`。

**Post-Condition：** 能用实测文件结构说明入口地址、节布局和栈大小与链接脚本一致。

**Case 1：ELF 地址。** 使用 `riscv64-unknown-elf-readelf -h bin/kernel` 和 `-S`，核对入口和 `.text = 0x80200000`、`.rodata = 0x802004c8`、`.data = 0x80201000`、`.sdata = 0x80203000`；解释 `ALIGN(0x1000)`。

**Case 2：符号及启动栈。** 使用 `riscv64-unknown-elf-nm -n bin/kernel`，核对 `kern_init = 0x8020000a`、`bootstack = 0x80201000`、`bootstacktop = 0x80203000`，从而得出 `KSTACKSIZE = 0x2000 = 8192 B`。核对 `edata = end = 0x80203008`。

**Case 3：文件不存在。** 先检查相对路径和当前目录，避免将路径错误误判为编译器错误。

**Requirements：** 区分 ELF 文件偏移、链接地址、加载地址和 CPU 运行时 PC；对当前没有独立 `.bss` 节的观察不作跨版本推断。

**验证证据：** `elf-sections.png`、`kernel-symbols.png`。

---

## B 组员：内核启动与练习一

### B1. 分析 entry.S 两条汇编指令和 kern_init

[PROMPT]

**任务：** 根据 `code/kern/init/entry.S`、`code/kern/init/init.c` 完成练习一：解释 `la sp, bootstacktop`、`tail kern_init` 的操作与目的，并以 objdump 反汇编核验。

**操作要求：** 保留源码，仅做阅读和验证；区分汇编伪指令与实际机器指令，不凭想象编造执行结果。

**输出要求：** 分别说明两条指令含义、必要性、与 C 初始化函数的衔接，提供符号地址和反汇编证据。

[RELY]

```asm
#include <mmu.h>
#include <memlayout.h>
.section .text,"ax",%progbits
.globl kern_entry
kern_entry:
    la sp, bootstacktop
    tail kern_init
.section .data
.align PGSHIFT
.global bootstack
bootstack:
    .space KSTACKSIZE
.global bootstacktop
bootstacktop:
```

```c
int kern_init(void) __attribute__((noreturn));
int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    const char *message = "(THU.CST) os is loading ...
";
    cprintf("%s

", message);
    while (1) ;
}
```

`edata/end` 由 `kernel.ld` 提供；`memset` 和 `cprintf` 为工程内的可用函数。

[GUARANTEE]

**无需新增或修改函数。必须分析的已有接口/符号：**

```c
int kern_init(void) __attribute__((noreturn));
```

```asm
kern_entry:
    la sp, bootstacktop
    tail kern_init
```

**交付物：** 练习一书面解答、8 KiB 启动栈说明、两份反汇编证据。

[SPECIFICATION]

**目标：`kern_entry`**

**Pre-Condition：** 固件已移交控制权，PC 指向 `0x80200000`，内核镜像及栈存储已准备好。

**Post-Condition：** `sp` 设为 `bootstacktop`，随后跳到 `kern_init`；无需返回汇编入口。

**Case 1：`la sp, bootstacktop`。** 将内核栈高地址边界装入 `sp`，满足 C 函数执行时的栈要求。

**Case 2：`tail kern_init`。** 不保留新的返回地址，直接转入 C 入口；比较 `tail` 和 `call`。

**Requirements：** 将伪指令与编译得到的 `auipc`/其他指令对应；引用 `kern_entry = 0x80200000`、`kern_init = 0x8020000a`、`bootstacktop = 0x80203000`。

**目标：`kern_init`**

**Pre-Condition：** 内核栈已建立。

**Post-Condition：** BSS 清零，打印字符串，并停在无限循环中。

**Case 1：本次构建 `end - edata = 0`。** 没有需要额外清零的字节，仍按原代码调用 `memset`。

**Requirements：** 解释 `cprintf` 非标准库函数以及内核经 SBI 输出字符的关系。

**验证证据：** `entry_disasm.png`、`kerninit_disasm.png`。

---

## C 组员：GDB 调试与练习二

### C1. 调试 CPU 复位至进入 kern_init 的全过程

[PROMPT]

**任务：** 使用 QEMU GDB Stub 和 GDB 观察从复位 `PC = 0x1000` 到 `kern_entry = 0x80200000` 再进入 `kern_init = 0x8020000a` 的启动链，完成练习二。

**操作要求：** 以实际寄存器和反汇编结果为证据，不修改 `kern/` 源码；如需修复调试连接问题，仅最小调整 Makefile 参数或选择已安装的 GDB。

**输出要求：** 两终端调试命令、六条复位指令作用、断点与 SP 变化表、关键截图、异常排查结果。

[RELY]

来自最终仓库 `code/Makefile` 的调试目标：

```makefile
debug: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
	$(V)$(QEMU) 		-machine virt 		-nographic 		-bios default 		-kernel $(kernel) 		-s -S

gdb:
	$(GDB)     -ex 'file bin/kernel'     -ex 'set arch riscv:rv64'     -ex 'target remote localhost:1234'
```

`kern_entry` 中仅有 `la sp, bootstacktop` 与 `tail kern_init`。C 的真实 GDB 记录：复位 `PC = 0x1000`，内核入口时 `SP = 0x80017ee0`，执行栈初始化后 `SP = 0x80203000`。

[GUARANTEE]

**无需新增函数。需完成的调试检查点：**

```text
PC: 0x1000（复位跳板）
PC: 0x80200000（内核入口）
SP: 0x80017ee0 -> 0x80203000（切换内核栈）
PC: 0x8020000a（kern_init）
```

**交付物：** 六条复位汇编解释、GDB 操作记录、PC/SP 状态变化和两张截图。

[SPECIFICATION]

**目标：复位跳板分析**

**Pre-Condition：** QEMU 通过 `make debug` 带 `-s -S` 启动并暂停，GDB 加载 ELF 符号后连接 `localhost:1234`。

**Post-Condition：** 确认 PC 在 `0x1000`，能够读出前六条指令并解释进入 OpenSBI 的动作。

**Case 1：正常连接。** 执行 `info registers pc sp a0 a1 a2` 和 `x/6i $pc`；分析 `auipc`、`addi`、`csrr`、`ld`、`ld`、`jr`。

**Case 2：连接超时。** 检查 QEMU 是否实际启用 `-s -S`，核对 Makefile 的反斜杠续行和目标端口；如交叉 GDB 不可用则尝试 `gdb-multiarch`。

**目标：内核入口与栈切换**

**Pre-Condition：** 已连接 QEMU，能够设置内核入口断点。

**Post-Condition：** 断点在 `0x80200000` 命中；单步后 SP 变为 `0x80203000`，PC 跳到 `0x8020000a`。

**Case 1：到达内核入口。** `break *0x80200000`、`continue`、`disassemble /r kern_entry`、`p/x &bootstacktop`，记录进入时 PC/SP。

**Case 2：单步验证。** 用 `si` 观察 SP 变化与跳转，并与 B 的静态反汇编结果互相验证。

**Requirements：** 不混用不同组员运行环境对应的寄存器值；保存实际 GDB 截图。

**验收证据：** `gdb_reset_0x1000.jpg`、`gdb_kern_entry_0x80200000.jpg`。
