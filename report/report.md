# 实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 2：物理内存与页表 |
| **小组成员** | 2513613-李佳凝、2512558-张爱迪、2513582-张子芊 |
| **完成日期** | 2026-10-23 |

### 小组分工

练习分工如下：

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2513613-李佳凝（A） | 物理内存初始化 `page_init()`、硬件可用物理内存范围 Challenge、最终整合 |
| 2512558-张爱迪（B） | First-Fit 分析、Best-Fit 实现、Buddy System |
| 2513582-张子芊（C） | QEMU/GDB 页表查询调试、SLUB Challenge |

实验报告由三名成员分别完成本人负责的部分，A 负责最终合并与格式检查。本稿已经完成 C 负责的内容，并为 A、B 保留待填写位置。

---

## 一、实验目的

本实验的主要目的是：

1. 理解物理内存探测、页描述符初始化以及物理页分配器之间的关系。
2. 掌握 First-Fit、Best-Fit 等连续物理页分配算法的基本思想和实现方式。
3. 理解 RISC-V Sv39 地址翻译、页表项权限、超级页以及 TLB 的作用。
4. 使用双 GDB 调试 QEMU 4.1.1，观察模拟硬件如何把 uCore 虚拟地址翻译为物理地址。
5. 在物理页分配器之上实现简化 SLUB，形成“页分配—小对象分配”的两层内存管理结构。
6. 按任务目标、功能边界和可验证结果设计 Prompt，并依据真实代码、输出和截图完成验证。

---

## 二、实验环境

### 2.1 基础环境

三名成员分别填写本人实际使用的实验环境；路径等任务上下文写入对应 Prompt，不在基础环境中重复堆列。

| 成员 | 操作系统与开发环境 | 编译、运行和调试工具 |
|------|--------------------|----------------------|
| 2513613-李佳凝（A） | 待 A 填写 | 待 A 填写 |
| 2512558-张爱迪（B） | 待 B 填写 | 待 B 填写 |
| 2513582-张子芊（C） | Windows + WSL Ubuntu 22.04，RISC-V 64 位 uCore Lab2 | `riscv64-unknown-elf` 工具链、QEMU 4.1.1 调试构建、host GDB、`gdb-multiarch` |

### 2.2 AI 工具

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2513613-李佳凝（A） | 待 A 填写 | 待 A 填写 | 物理内存初始化与整合 |
| 2512558-张爱迪（B） | 待 B 填写 | 待 B 填写 | 物理页分配算法 |
| 2513582-张子芊（C） | Codex 桌面应用 | GPT-5 | 按四段式设计 Prompt，辅助源码定位、GDB 调试、SLUB 实现与报告整理 |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

本章围绕“内核如何认识、管理并使用内存”展开。首先从设备树取得物理内存范围，排除 OpenSBI、内核和页描述符数组占用的区域；随后把剩余完整页面交给物理内存管理器，并实现连续物理页的分配与回收；在此基础上，内核启用 Sv39 页表，把高虚拟地址映射到实际物理内存；最后可以继续在页级分配器之上构建 SLUB，为内核提供小对象分配能力。

### 3.2 功能的逐步实现

1. **物理内存初始化**：先确定可管理内存边界并建立 `Page` 描述符，否则后续页分配器没有可靠的资源视图。
2. **连续物理页分配**：在已经登记的空闲页上实现 First-Fit、Best-Fit 和 Buddy System，提供 `alloc_pages()`/`free_pages()` 基础能力。
3. **虚拟地址翻译验证**：页表启用后，通过 QEMU 源码和双 GDB 观察 TLB miss、Sv39 查表、权限检查和 TLB 填充。
4. **SLUB 小对象分配**：以物理页分配器为后端，把一个页面划分为多个对象，实现更细粒度的内核内存分配。
5. **统一测试与回归**：完成干净构建、内核运行、SLUB 自检和课程评分，确认各模块合并后仍保持正确。

---

## 四、实验内容与实现

### 功能模块一·任务一：物理内存探测与页描述符初始化

**负责人：** 2513613-李佳凝（A）

> 待 A 填写：模块功能、涉及函数、实际完成内容、测试结果，以及硬件可用物理内存范围 Challenge；对应 Prompt 写入 `prompt.md` 或 `challenge_prompt.md`。

---

### 功能模块二：物理内存分配（任务一、任务二、任务三）

**负责人：** 2512558-张爱迪（B）

> 待 B 填写：任务一 First-Fit 分析、任务二 Best-Fit 实现与改进空间、任务三 Buddy System、实际完成内容和测试结果；对应 Prompt 写入 `prompt.md`。

---

### 分支任务：GDB 调试页表查询过程

**负责人：** 2513582-张子芊（C）

#### 模块功能描述

本部分对应指导书中的“分支任务：GDB 调试页表查询过程”。调试对象不是 uCore 中的普通 C 函数，而是“正在模拟 uCore 的 QEMU”。通过带调试信息的 QEMU 4.1.1、host GDB 和 guest GDB，观察虚拟 CPU 发生取指访问时，QEMU 如何查询软件 TLB、在 miss 后遍历 Sv39 页表、形成物理地址并填充 TLB。

**主要涉及的 QEMU 函数：**

```c
static int get_physical_address(CPURISCVState *env, hwaddr *physical,
                                int *prot, target_ulong addr,
                                int access_type, int mmu_idx);

bool riscv_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                        MMUAccessType access_type, int mmu_idx,
                        bool probe, uintptr_t retaddr);

void tlb_set_page(CPUState *cpu, target_ulong vaddr,
                  hwaddr paddr, int prot,
                  int mmu_idx, target_ulong size);
```

#### 实际完成内容

1. 在独立目录完成 QEMU 4.1.1 的调试构建，并确认生成文件带有调试符号、未被 strip，没有执行 `make install`。
2. 定位 QEMU 的 TLB miss、RISC-V 地址翻译和 TLB 填充代码，建立 host GDB 与 guest GDB 调试链路；guest GDB 连接前设置 `set remotetimeout unlimited`。
3. 对内核入口附近的真实取指地址设置条件断点，通过调用栈排除调试器辅助查询，单步记录 `satp`、VPN、PTE、权限、物理页地址和 `tlb_set_page()` 参数。
4. 根据实际报错调整工具和命令：用 `grep` 代替缺失的 `rg`，使用 `-kernel` 启动 uCore，并把 GDB 命令逐行输入以避免 `malformed linespec`。

![QEMU 4.1.1 调试符号验证](images/qemu-4.1.1-debug-symbols.png)

本任务使用的 Prompt 单独保存在 `prompt.md`，报告以下内容只记录完成的操作和观察结果。

#### 1. QEMU 源码调用路径与 Sv39 单步翻译（对应调试要求 1、2）

QEMU 的 TCG 访存辅助代码先按虚拟页和 MMU 索引查询软件 TLB。标签不匹配时进入慢路径 `tlb_fill()`，RISC-V CPU 类注册的回调继续进入 `riscv_cpu_tlb_fill()`，再调用 `get_physical_address()`。

`get_physical_address()` 的关键分支包括：

1. M 模式直接访问或未启用 MMU时执行恒等映射；
2. 从 `satp` 取得分页模式与根页表 PPN；
3. 根据 Sv32/Sv39/Sv48/Sv57 选择层数、索引宽度和 PTE 大小；
4. 循环计算 `pte_addr = base + idx * ptesize` 并读取 PTE；
5. 检查 `V/R/W/X`、访问权限、A/D 位和超级页对齐；
6. 非叶子项更新下一级页表基址，叶子项形成物理页地址；
7. 翻译成功后由 `tlb_set_page()` 写入 QEMU 软件 TLB，失败则设置相应异常。

为了减少无关中断，使用内核入口附近必然发生的取指地址作为目标：

```text
address     = 0xffffffffc02000d8
access_type = MMU_INST_FETCH
mmu_idx     = 1
```

调用栈实测为：

```text
#0 get_physical_address
#1 riscv_cpu_tlb_fill
#2 tlb_fill
```

断点处的 `satp` 为：

```text
env->satp = 0x8000000000080205
```

最高 4 位 `8` 表示 Sv39；根 PPN 为 `0x80205`，根页表物理地址为 `0x80205000`。目标虚拟地址拆分为：

| 字段 | 数值 |
|------|------|
| `VPN[2]` | `511` |
| `VPN[1]` | `1` |
| `VPN[0]` | `0` |
| 页内偏移 | `0xd8` |

第一轮页表遍历实测：

```text
i        = 0
pte_addr = 0x80205ff8
pte      = 0x200000cf
ppn      = 0x80000
```

手工计算 `0x80205000 + 511 × 8 = 0x80205ff8`，与 QEMU 一致。PTE 低位 `0xcf` 表示 `V/R/W/X/A/D` 置位，`U/G` 未置位。根级即遇到叶子项，因此本次使用 1 GiB 超级页，不再读取后两级页表。

![Sv39 根级 PTE 的实际读取结果](images/sv39-pte-walk.png)

QEMU 返回的页级映射为：

```text
address         = 0xffffffffc02000d8
pa              = 0x80200000
address & 0xfff = 0xd8
prot            = 0x7
mmu_idx         = 1
```

最终物理字节地址为 `0x80200000 + 0xd8 = 0x802000d8`，`prot=0x7` 表示读、写、执行权限。

![翻译结果写入 QEMU 软件 TLB](images/tlb-set-page.png)

#### 2. TLB、Bare 模式与真实 CPU 的区别（对应调试要求 3、4）

在 `accel/tcg/cputlb.c` 中可以定位 TCG 软件 TLB 的 miss 慢路径；本次动态调试实际命中了 `tlb_fill()`、`riscv_cpu_tlb_fill()`、页表遍历以及 `tlb_set_page()`。这证明目标映射在 miss 后被计算并写入软件 TLB。本次没有单独记录同一页后续访问的 fast-path hit，因此不把“第二次访问更快”写成实测结论。

QEMU 源码中，M 模式直接访问或未启用 MMU 时令 `physical = addr`，不遍历页表；Sv39 模式则使用 `satp` 和 VPN 逐级读取 PTE。本次对 Sv39 完成了动态调试；Bare 仅完成源码分支分析，没有把 guest GDB 的辅助内存查询冒充成真实 CPU 访存。

真实 CPU TLB 是处理器内部硬件结构，与流水线、权限检查和异常机制配合；QEMU TLB 是宿主机内存中的软件结构，用于缓存 guest 虚拟页的翻译。两者逻辑上都利用局部性并需要在页表变化后失效，但实现载体、命中代价和并发机制不同。

#### 3. 调试记录与 AI 协作（对应调试要求 5、6）

- 调试器为了读取 guest 内存也会请求地址翻译，所以“断点命中”本身不能证明发生了 CPU 访存，必须检查调用栈。
- Sv39 虽称三级页表，本次根级 PTE 已是超级页叶子，因此循环只执行一轮。
- `pa=0x80200000` 是页基址，必须加上 `0xd8` 页内偏移才得到最终字节地址。
- `set remotetimeout unlimited` 解决的是双 GDB 暂停关系导致的远程等待问题，而不是页表算法错误。

关键修复包括用 `grep` 替代缺失的 `rg`、把 `-device loader` 改为 `-kernel`、用调用栈排除调试器辅助访问、逐行执行 GDB 命令，以及设置无限远程等待时间。任务 Prompt 记录在 `prompt.md`。

---

### 功能模块二·扩展任务 Challenge：任意大小的内存单元 SLUB 分配算法

**负责人：** 2513582-张子芊（C）

#### 模块功能描述

SLUB 在现有页分配器上构建两层架构：第一层通过 `alloc_page()`/`free_page()` 管理 4 KiB 后备页；第二层把每个后备页切分为固定大小对象，对外提供小对象分配与释放接口。

**实现和接入的函数：**

```c
void slub_init(void);
void *slub_alloc(size_t size);
void slub_free(void *ptr);
void slub_check(void);
```

实现文件为 `kern/mm/slub.c`、`kern/mm/slub.h`，并在 `pmm_init()` 完成后调用 `slub_init()` 和 `slub_check()`。

#### 实际完成内容

1. 阅读 PMM、页描述符、链表和内核初始化接口，新增 `kern/mm/slub.c`、`kern/mm/slub.h`，并在 `pmm_init()` 之后接入 SLUB 初始化与自检。
2. 实现九个大小类以及 slab 页内元数据、对象 freelist、partial/full 链表，保证对象使用最小可容纳的大小类。
3. 完成分配、释放和状态转换：满 slab 进入 full，释放后回到 partial，完全空闲时归还后备页；同时检查非法地址、非对象边界和重复释放。
4. 修正当前内核布尔常量兼容问题后完成编译，并用 `slub_check()` 验证边界大小、对齐、对象隔离、对象复用、跨页分配和空页回收。

本 Challenge 的“实现 Prompt”和“测试 Prompt”单独保存在 `challenge_prompt.md`。

#### 与 Linux SLUB 的区别

本实现保留“按大小类组织 cache—以页为 slab—页内管理对象—维护满页和可分配页—空页回收”的主体思想，但省略真实 Linux SLUB 的 per-CPU cache、NUMA、并发锁、批量操作、红区、对象构造函数和复杂调试机制，因此适合教学展示，不等同于生产内核分配器。

---

## 五、测试与验证

### 5.1 A：物理内存初始化与硬件内存探测

> 待 A 补充本人负责部分的测试方法、关键输出和必要截图。

### 5.2 B：First-Fit、Best-Fit 与 Buddy System

> 待 B 补充本人负责部分的测试方法、关键输出和必要截图。

### 5.3 C：QEMU/GDB 调试与 SLUB Challenge

#### 干净构建

合并 A、B、C 代码后执行 `make clean` 和并行构建，结果为：

```text
Build status: 0
bin/kernel: ELF 64-bit LSB executable, UCB RISC-V,
            with debug_info, not stripped
```

#### 内核运行与 SLUB 自检

内核成功进入物理内存管理与 SLUB 检查：

```text
check_alloc_page() succeeded!
satp virtual address: 0xffffffffc0205000
satp physical address: 0x0000000080205000
slub_check: begin
slub_check: passed
```

![SLUB 自检通过](images/slub-check-passed.png)

### 5.4 合并版本课程评分

评分脚本在临时副本中注入课程测试，并使用当前环境已经验证的 `-kernel bin/ucore.img` 启动方式。最终结果：

```text
check page init: OK
check best fit:  OK
Total Score: 20/20
Grade status: 0
```

![Lab2 基础任务最终评分](images/grade-20-20.png)

该结果说明当前合并版本的页面初始化和 Best-Fit 通过课程基础测试；SLUB Challenge 由独立的 `slub_check()` 验证。

---

## 六、实验总结与收获

### 6.1 对操作系统的理解

本实验中重要的知识点包括：

1. **物理页管理与小对象管理分层**：PMM 解决按页分配，SLUB 在页内继续切分对象；两者对应不同粒度，不能相互替代。
2. **Sv39 多级页表**：虚拟地址包含多级 VPN 与页内偏移，页表项决定继续查表还是形成叶子映射；超级页允许在高层提前结束遍历。
3. **TLB 与局部性**：TLB 缓存近期页级映射，避免每次访存都读取多级页表；页表更新后必须考虑失效与一致性。
4. **权限与异常**：地址存在映射并不等于允许访问，PTE 权限、访问类型、特权级和 PMP 都可能使翻译失败。
5. **模拟器与硬件的对应关系**：QEMU 用 C 代码和宿主机数据结构模拟 MMU/TLB，使原本不可见的硬件过程能够被源码级调试。

OS 原理中重要但本实验没有完整覆盖的内容包括：多进程独立地址空间、ASID 与上下文切换、TLB shootdown、多核一致性、换页、页面置换、写时复制以及真实内核 SLUB 的并发和 NUMA 优化。

### 6.2 AI 协作开发的经验

本组在使用 AI 协作完成实验时，总结出以下经验：

1. **先按指导书划分任务和边界**：先明确 A、B、C 各自负责的功能模块、Challenge 和报告内容，再为每个可以独立实现、独立验证的任务编写 Prompt，能够减少多人同时修改同一模块造成的冲突。
2. **上下文必须来自真实工程**：Prompt 中的宏定义、数据结构、函数签名、文件位置和可调用接口应从当前代码中核对，不能根据其他版本的 uCore 或 QEMU 凭印象填写。错误的接口信息会直接导致编译失败或调试位置错误。
3. **实现与测试形成闭环**：对于 Best-Fit、Buddy System、SLUB 等编程任务，不仅要描述需要生成的实现，还要明确接口契约、边界情况和测试方法。实现完成后依次进行单文件编译、完整构建、内核运行和课程评分，不能只以代码看起来合理作为完成标准。
4. **根据反馈修正，而不是掩盖问题**：编译错误、运行异常和断点未命中都应先定位原因，再调整代码或 Prompt；不能通过删除断言、修改既有测试或直接填写预期输出来制造通过结果。
5. **区分动态证据、源码分析和推断**：终端输出、调用栈和变量值属于动态证据；没有实际命中的路径只能写为源码分析。报告中应明确这种区别，使结论能够被其他成员和助教复查。
6. **统一合并规范**：每名成员负责填写自己的环境、实现说明、Prompt 和测试结果，最终合并时统一标题、图片路径、接口名称和术语，并重新执行完整测试，确认单独正确的模块组合后仍能正常工作。

## 待合并检查

- [ ] A 完成物理内存初始化、Challenge、测试结果和对应 Prompt。
- [ ] B 完成 First-Fit、Best-Fit、Buddy System、测试结果和对应 Prompt。
- [x] C 完成 QEMU 页表调试、SLUB Challenge、测试、截图和 Prompt。
- [ ] 三人统一检查所有测试、图片相对路径和 Git 提交内容。
