# Lab2 普通任务 Prompt

## A（2513613 李佳凝）：功能模块一·任务一——物理内存探测与页描述符初始化

### Prompt：核对、完善并验证物理内存初始化

```text
[PROMPT]
任务：以仓库最新 lab2 分支为基础，按指导书完成功能模块一·任务一。检查已有 page_init()，正确部分保留，仅修复必要内容，不用旧版 lab2_new 覆盖 B、C 的成果。
操作要求：保留 Best-Fit 管理器选择、分配算法、SLUB 接入和课程测试；保留 lab2 注释，将 A 负责位置的 YOUR CODE 替换为 2513613。本次只修改和验证本地副本，不提交或推送 GitHub。
输出要求：完成干净构建、QEMU 运行、make grade，记录真实输出；补充 A 的报告、内存布局、地址关系和实际 bug/修复过程，未验证内容明确标注。

[RELY]
工程：code/；主要文件：kern/mm/pmm.c、pmm.h、memlayout.h、mmu.h，kern/driver/dtb.c、dtb.h，kern/init/entry.S、init.c，tools/test_lab2.c。
接口：get_memory_base()、get_memory_size()、PADDR()、pa2page()、init_memmap()、panic()、cprintf()。
PGSIZE=4096；nbase=DRAM_BASE/PGSIZE；va_pa_offset=PHYSICAL_MEMORY_OFFSET。
dtb_init() 先于 pmm_init()；当前 PMM 为 best_fit_pmm_manager。

[GUARANTEE]
正确设置 va_pa_offset、npage、pages；检查无效 DTB 范围、溢出、启动映射边界和内存不足。
描述符数组在 end 后按页对齐，包含 npage-nbase 项；初始化各字段并先将所有页面标为保留。
排除 OpenSBI、内核、描述符数组和对齐空间，只登记完整空闲页。
保持地址转换、Best-Fit 和 SLUB 行为正确，不修改测试或伪造结果。

[SPECIFICATION]
## page_init
Pre-Condition：DTB 和 PMM 管理器已初始化；当前框架为以 DRAM_BASE 开始的单段 RAM。
Post-Condition：maxpa=ROUNDDOWN(mem_base+mem_size,PGSIZE)，npage=PPN(maxpa)；pages 在 end 后页对齐，数组长度为 npage-nbase；freemem 为数组末尾向上按页对齐的物理地址；通过 init_memmap(pa2page(freemem),(maxpa-freemem)/PGSIZE) 注册空闲页。
Case 1：合法且内存充足时，输出内存范围、空闲范围和页数。
Case 2：无效信息、未映射范围或内存不足时终止初始化；无完整空闲页时不得调用 init_memmap()。
Requirements：通过课程 Page Init、Best-Fit 检查并回归 SLUB；补充不同 RAM 大小验证。异常分支可在独立副本注入输入验证，不能更改正式代码或课程测试来制造通过。
```

本次执行记录：基线为 `6cc74ad`，已有实现来自合并分支；本次补充启动直接映射上界检查和 A 的学号注释。验证环境及真实结果见 `report.md` 和本地留存的验证记录。这是本次实际使用的整理版 Prompt，不冒充已有代码最初生成时的 Prompt。

## B（2512558 张爱迪）：功能模块二·任务一、任务二、任务三

> 待 B 按 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 补充 First-Fit、Best-Fit 和 Buddy System 对应 Prompt。

## C（2513582 张子芊）：分支任务——GDB 调试页表查询过程

### Prompt 1：准备调试版 QEMU 并定位地址翻译源码

````text
[PROMPT]
**任务**：基于 `/home/jicheon/oslab-tools/qemu-4.1.1`，在独立目录构建带调试信息的 `qemu-system-riscv64`，验证版本和符号状态，并定位软件 TLB miss、Sv39 页表遍历和 TLB 填充代码。

**操作要求**：不修改 QEMU 源码，不执行 `make install`。由实验者运行命令；每次先解释用途，再等待真实输出。函数位置必须以 QEMU 4.1.1 源码为准。

**输出要求**：给出构建与符号验证结果，以及关键文件、函数和最短调用关系。警告、错误和未验证内容应明确区分。

[RELY]
QEMU 源码：/home/jicheon/oslab-tools/qemu-4.1.1
构建目录：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug
目标程序：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug/riscv64-softmmu/qemu-system-riscv64
构建目标：riscv64-softmmu

相关源码：
- target/riscv/cpu_helper.c
- accel/tcg/cputlb.c

关键函数：
- tlb_fill()
- riscv_cpu_tlb_fill()
- get_physical_address()
- tlb_set_page()

[GUARANTEE]
- 生成独立的 `riscv64-softmmu/qemu-system-riscv64`；
- 验证版本为 4.1.1，文件包含调试信息且未 strip；
- 定位 TLB 查询与 miss、RISC-V 地址翻译和 TLB 填充位置；
- 说明关键函数在一次地址翻译中的调用关系。

[SPECIFICATION]
## 调试版 QEMU
**Pre-Condition**:
- QEMU 4.1.1 源码和构建依赖可用。

**Post-Condition**:
- 构建退出状态为 0，目标程序可执行且能被 GDB 解析源码符号。

**Requirements**:
- 构建产物保留在独立目录，不覆盖系统 QEMU。

## 地址翻译源码
**Pre-Condition**:
- QEMU 版本已经确认，能够搜索并查看源码。

**Post-Condition**:
- 找到软件 TLB miss、Sv39 查表、权限检查、物理地址形成和 TLB 填充代码，并整理调用关系。
````

### Prompt 2：建立 host GDB 与 guest GDB 调试链路

````text
[PROMPT]
**任务**：在 `/mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_new` 建立双 GDB 链路：host GDB 调试 QEMU 进程，guest GDB 加载 RISC-V uCore 内核并连接 QEMU GDB stub。

**操作要求**：实验者在两个终端分别输入命令。每次说明终端编号、命令目的和成功标志，再等待真实输出。host 与 guest 的程序、架构和符号文件不得混用。

**输出要求**：给出两个终端的启动顺序和命令，确认 QEMU 源码断点可设置、guest GDB 可连接，并说明两边的暂停与继续关系。

[RELY]
项目目录：/mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_new
guest 符号文件：bin/kernel
guest 镜像：bin/ucore.img

host GDB：/usr/bin/gdb
guest GDB：/usr/bin/gdb-multiarch
调试版 QEMU：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug/riscv64-softmmu/qemu-system-riscv64

QEMU 参数：
-machine virt -nographic -bios default -kernel bin/ucore.img -s -S

guest GDB 设置：
set architecture riscv:rv64
set remotetimeout unlimited
file bin/kernel
target remote localhost:1234

[GUARANTEE]
- host GDB 正确加载调试版 QEMU 和宿主源码符号；
- QEMU 使用 `-s -S` 打开 GDB stub 并暂停虚拟 CPU；
- guest GDB 加载 `bin/kernel` 并连接 `localhost:1234`；
- 能分别控制 QEMU 进程和虚拟 CPU 的暂停与继续。

[SPECIFICATION]
## host GDB
**Pre-Condition**:
- 调试版 QEMU 和 `bin/ucore.img` 已生成。

**Post-Condition**:
- QEMU 在 host GDB 中启动，地址翻译函数可设置源码断点。

## guest GDB
**Pre-Condition**:
- QEMU GDB stub 已监听，虚拟 CPU 处于暂停状态。

**Post-Condition**:
- guest GDB 使用 RISC-V 64 位架构和内核符号连接成功。

**Requirements**:
- 在 `target remote` 前执行 `set remotetimeout unlimited`。
````

### Prompt 3：单步验证 Sv39 页表翻译与 TLB 填充

````text
[PROMPT]
**任务**：在双 GDB 链路中，以内核取指地址 `0xffffffffc02000d8` 为目标，观察一次由 TLB miss 触发的 Sv39 翻译，记录页表遍历、权限、物理地址和 `tlb_set_page()` 参数，并完成指导书要求的 TLB 分析。

**操作要求**：实验者逐条执行 GDB 命令。使用条件断点减少无关中断；每一步先解释观察对象，再等待真实输出。只保留支撑报告结论的必要截图。

**输出要求**：根据真实调用栈和变量完成虚拟地址到物理地址的手工核对；区分真实访存、调试器辅助查询、源码分析和未验证内容。

[RELY]
目标虚拟地址：0xffffffffc02000d8
访问类型：MMU_INST_FETCH
架构与分页模式：RISC-V 64 位、Sv39

bool riscv_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                        MMUAccessType access_type, int mmu_idx,
                        bool probe, uintptr_t retaddr);

static int get_physical_address(CPURISCVState *env, hwaddr *physical,
                                int *prot, target_ulong addr,
                                int access_type, int mmu_idx);

void tlb_set_page(CPUState *cpu, target_ulong vaddr,
                  hwaddr paddr, int prot,
                  int mmu_idx, target_ulong size);

Sv39 中页内偏移为 12 位，三级 VPN 各为 9 位，PTE 为 8 字节；`satp.MODE=8`，根页表地址为 `satp.PPN << 12`。

[GUARANTEE]
- 捕获 `tlb_fill()`、`riscv_cpu_tlb_fill()`、`get_physical_address()` 的真实调用关系；
- 记录 `address`、`access_type`、`mmu_idx`、`satp`、VPN 和页内偏移；
- 记录实际经过层级的 PTE 地址、PTE 值、PPN、权限和叶子层级；
- 在 `tlb_set_page()` 处确认虚拟页、物理页、权限和 MMU 索引；
- 计算最终物理字节地址，并说明 Bare/Sv39、软件 TLB/硬件 TLB 的区别。

[SPECIFICATION]
## 真实 TLB miss
**Pre-Condition**:
- 双 GDB 已连接，条件断点限定到目标地址。

**Post-Condition**:
- 调用栈经 `tlb_fill()` 进入 `riscv_cpu_tlb_fill()`，并记录访问类型和 MMU 索引。

**Case 1**:
- 若调用栈经过 `riscv_cpu_get_phys_page_debug()` 或 `cpu_memory_rw_debug()`，标记为调试器辅助查询并继续寻找真实访存。

## get_physical_address
**Pre-Condition**:
- 已从真实 TLB miss 路径进入函数。

**Post-Condition**:
- 根据 `satp`、VPN 和实际读取的 PTE 得到物理页基址与权限。

**Case 1**:
- 非叶子 PTE 更新下一级页表基址并继续遍历。

**Case 2**:
- 高层合法叶子 PTE 按超级页规则形成物理页基址。

**Requirements**:
- 检查 `V/R/W/X/U/G/A/D` 位和超级页对齐；物理页基址与页内偏移分别记录。

## tlb_set_page
**Pre-Condition**:
- 页表翻译成功。

**Post-Condition**:
- 确认映射参数被写入 QEMU 软件 TLB。

**Requirements**:
- 未实际观察的 Bare 路径或 TLB fast-path hit 必须标注为源码分析或未验证。
````
