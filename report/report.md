# Lab1：最小可执行内核实验报告

## 一、实验目的

1. 熟悉 RISC-V 64 位操作系统内核的编译、链接与启动过程。
2. 掌握 RISC-V 交叉编译工具链、QEMU 模拟器及 GDB 调试工具的基本使用方法。
3. 理解 Makefile 的构建规则，以及链接脚本对内核内存布局的控制。
4. 通过实际运行和 ELF 文件分析，验证内核入口地址、程序节布局及启动栈的分配情况。

## 二、实验环境

| 项目 | 实验环境 |
|---|---|
| 宿主系统 | Windows + WSL Ubuntu |
| 目标架构 | RISC-V 64 位 |
| 交叉编译工具链 | SiFive RISC-V GNU Toolchain 10.2.0 |
| 模拟器 | QEMU（RISC-V virt 平台） |
| 固件 | OpenSBI v1.3 |
| 调试工具 | riscv64-unknown-elf-gdb |
| 版本管理 | Git + GitHub |

## 三、实验原理

### 3.1 RISC-V 内核启动流程

本实验使用 QEMU 模拟 RISC-V 64 位计算机，主要启动流程如下：

CPU 加电复位 → 执行复位地址 `0x1000` 处的引导代码 → 进入 OpenSBI 固件 → 跳转至 `0x80200000` → 执行 `kern_entry` → 初始化内核栈 → 进入 `kern_init()`。

其中，OpenSBI 固件提供底层硬件初始化及 SBI 服务，内核入口汇编代码负责完成进入 C 语言环境前的必要准备。

### 3.2 Makefile 编译构建流程

本实验使用 Makefile 管理内核的自动构建过程。执行 `make` 后，主要经历以下阶段：

**（1）交叉编译**

使用 `riscv64-unknown-elf-gcc` 编译 C 语言和汇编源文件，生成 RISC-V 目标文件。

由于宿主计算机使用 x86-64 架构，而目标程序需要在 RISC-V 64 位模拟环境中运行，因此必须使用交叉编译工具链。

Makefile 通过 `GCCPREFIX` 变量定义编译器前缀，并通过 `tools/function.mk` 中的辅助函数组织多个源文件的编译。

**（2）链接**

使用 `riscv64-unknown-elf-ld` 将目标文件链接为 `bin/kernel`。

关键链接命令包含：

```bash
$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
```

其中，`-T tools/kernel.ld` 指定链接脚本，控制内核程序节的布局及入口地址。

**（3）生成内核镜像**

使用：

```bash
riscv64-unknown-elf-objcopy bin/kernel --strip-all -O binary bin/ucore.img
```

将 ELF 格式的内核转换为原始二进制镜像。

其中，`bin/kernel` 保存 ELF 结构和调试相关信息，`bin/ucore.img` 则用于原始二进制加载方式。

### 3.3 kernel.ld 链接与内存布局

链接脚本 `tools/kernel.ld` 中的重要定义如下：

```ld
OUTPUT_ARCH(riscv)
ENTRY(kern_entry)
BASE_ADDRESS = 0x80200000;
```

`OUTPUT_ARCH` 指定目标架构，`ENTRY` 指定内核入口符号，`BASE_ADDRESS` 设置内核基地址。

链接脚本按照 `.text`、`.rodata`、`.data`、`.sdata` 和 `.bss` 的顺序组织程序内容，并使用 `ALIGN(0x1000)` 将数据区对齐到 4 KiB 边界。

在本实验的 QEMU 环境中，OpenSBI 固件位于 `0x80000000` 一带，内核选择从 `0x80200000` 开始，以满足课程使用的固件和内核布局约定。

## 四、实验过程与结果

### 4.1 内核编译

在 `code/` 目录下执行：

```bash
make
```

编译过程依次处理 `entry.S`、`init.c`、`stdio.c`、`console.c` 等源文件，并链接生成内核。

编译完成后，得到：

| 文件 | 大小 | 类型 |
|---|---|---|
| `bin/kernel` | 约 48 KB | ELF 可执行内核 |
| `bin/ucore.img` | 约 13 KB | 原始二进制内核镜像 |

编译过程中未出现错误，说明交叉编译工具链和基本构建规则能够正常工作。

### 4.2 内核启动及问题排查

最初执行 `make qemu` 时，OpenSBI v1.3 正常启动，但输出中显示：

```text
Domain0 Next Address : 0x0000000000000000
```

且没有出现内核预期的打印信息。

检查 Makefile 后发现，原有 QEMU 启动命令使用：

```bash
-device loader,file=bin/ucore.img,addr=0x80200000
```

该参数指定了镜像的加载位置，但当前环境未能通过此方式正确设置固件移交控制权时的内核入口。

随后尝试：

```bash
qemu-system-riscv64 \
    -machine virt \
    -nographic \
    -bios default \
    -kernel bin/kernel
```

本次启动成功，OpenSBI 显示：

```text
Domain0 Next Address : 0x0000000080200000
Domain0 Next Mode    : S-mode
```

内核输出：

```text
(THU.CST) os is loading ...
```

因此，对 Makefile 的 `qemu` 和 `debug` 目标进行了相应调整，并重新验证运行和 GDB 连接功能。

这次排查表明：编译成功、链接成功与内核真正开始执行是不同阶段，需要分别验证。

![内核启动成功](images/boot-success.png)

### 4.3 ELF 节布局验证

使用以下命令查看 ELF 节头表：

```bash
riscv64-unknown-elf-readelf -S bin/kernel
```

实验结果显示，内核 ELF 文件包含 18 个节头表项。主要节的信息如下：

| 节名称 | 起始地址 | 大小 |
|---|---|---|
| `.text` | `0x80200000` | `0x4c8` |
| `.rodata` | `0x802004c8` | `0x270` |
| `.data` | `0x80201000` | `0x2000` |
| `.sdata` | `0x80203000` | `0x8` |

`.text` 节从 `0x80200000` 开始，与链接脚本的内核基地址一致。

`.rodata` 节结束地址为：

`0x802004c8 + 0x270 = 0x80200738`

由于链接脚本使用 `ALIGN(0x1000)`，后续 `.data` 节从 `0x80201000` 开始，验证了数据区的对齐规则。

此外，ELF 文件中包含 `.debug_info`、`.debug_line` 等调试信息节，为 GDB 调试提供支持。

![ELF节布局](images/elf-sections.png)

### 4.4 内核符号地址验证

使用以下命令查看关键符号：

```bash
riscv64-unknown-elf-nm -n bin/kernel | grep -E 'kern_entry|kern_init|edata|end|bootstack'
```

得到：

| 符号 | 地址 |
|---|---|
| `kern_entry` | `0x80200000` |
| `kern_init` | `0x8020000a` |
| `bootstack` | `0x80201000` |
| `bootstacktop` | `0x80203000` |
| `edata` | `0x80203008` |
| `end` | `0x80203008` |

其中，`kern_entry` 与链接脚本指定的内核入口一致，`kern_init` 紧随入口汇编代码之后。

启动栈大小为：

`0x80203000 - 0x80201000 = 0x2000 = 8192 B`

即 8 KiB。

由于 RISC-V 栈向低地址增长，启动时将 SP 初始化为 `bootstacktop`，使后续函数能够正常使用栈空间。

此外，`edata` 与 `end` 地址相同，说明当前构建产物中两者之间没有需要额外清零的 BSS 空间。

![内核符号地址](images/kernel-symbols.png)

## 五、实验练习

### 5.1 练习一：理解内核入口操作

（由 B 组员补充）

### 5.2 练习二：使用 GDB 验证启动流程

（由 C 组员补充）

## 六、实验总结

### 6.1 主要知识点

本实验涉及交叉编译、ELF 文件结构、链接脚本、内核入口、启动栈以及固件与操作系统之间的控制权交接。

通过实际编译和启动，理解了 Makefile 负责组织构建流程、链接脚本负责确定地址布局、QEMU 与 OpenSBI 配合完成内核启动的基本关系。

### 6.2 与操作系统理论知识的联系

交叉编译和链接属于操作系统构建的基础环节，内核启动体现了计算机从底层固件进入操作系统的过程。

本实验还初步涉及栈空间、特权模式和内存布局等概念，为后续进程管理、虚拟内存与中断处理实验奠定基础。

### 6.3 本实验尚未涉及的知识

当前内核仅实现最基础的启动及输出功能，尚未完整实现物理内存分配、虚拟内存管理、进程调度、并发同步和文件系统等操作系统核心机制。这些功能将在后续实验中逐步学习和实现。

### 6.4 实验心得

通过本次实验，认识到操作系统内核开发不仅需要正确编写源代码，还需要保证编译工具链、链接地址、启动固件和模拟器配置之间相互匹配。

特别是在排查内核无法正常启动的问题时，通过对比 ELF 入口地址、QEMU 启动参数和 OpenSBI 输出，逐步确定了问题所在，进一步理解了内核从构建到执行的全过程。