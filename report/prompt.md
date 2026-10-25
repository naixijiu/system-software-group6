# Lab2 普通任务 Prompt

## A（2513613 李佳凝）：功能模块一·任务一——物理内存探测与页描述符初始化

### Prompt：核对、完善并验证物理内存初始化
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
任务项：page_init
Pre-Condition：DTB 和 PMM 管理器已初始化；当前框架为以 DRAM_BASE 开始的单段 RAM。
Post-Condition：maxpa=ROUNDDOWN(mem_base+mem_size,PGSIZE)，npage=PPN(maxpa)；pages 在 end 后页对齐，数组长度为 npage-nbase；freemem 为数组末尾向上按页对齐的物理地址；通过 init_memmap(pa2page(freemem),(maxpa-freemem)/PGSIZE) 注册空闲页。
Case 1：合法且内存充足时，输出内存范围、空闲范围和页数。
Case 2：无效信息、未映射范围或内存不足时终止初始化；无完整空闲页时不得调用 init_memmap()。
Requirements：通过课程 Page Init、Best-Fit 检查并回归 SLUB；补充不同 RAM 大小验证。异常分支可在独立副本注入输入验证，不能更改正式代码或课程测试来制造通过。
## B（2512558 张爱迪）：功能模块二·任务一、任务二、任务三

### Prompt 1：实现 Best-Fit 分配器
[PROMPT]
在 kern/mm/best_fit_pmm.c 中补全标记为 //lab2 function2-2 YOUR CODE 的位置，实现完整的 Best-Fit 物理内存管理模块。必须在项目中实际修改该文件，不要只输出代码片段。保留文件现有的 basic_check() 和 best_fit_check() 原样，不得修改这两个测试函数，也不要修改 default_pmm.c 或其他无关文件。只有在确实缺少声明时才允许调整 best_fit_pmm.h。

[RELY]
当前框架提供：
struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; };
free_area_t { list_entry_t free_list; unsigned int nr_free; };
Page.property 表示空闲块头页记录的连续页数；
PageReserved、PageProperty、SetPageProperty、ClearPageProperty、set_page_ref；
list_init、list_add、list_add_before、list_del、list_next、list_prev、list_empty；
le2page(le, page_link)；
struct pmm_manager 的字段 name、init、init_memmap、alloc_pages、free_pages、
nr_free_pages、check；
best_fit_check() 直接使用 free_list 和 nr_free 两个名字，因此实现中必须提供与 default_pmm.c 风格一致的 free_area_t free_area 和对应宏。

[GUARANTEE]
必须交付以下函数或对象：
static void best_fit_init(void);
static void best_fit_init_memmap(struct Page *base, size_t n);
static struct Page *best_fit_alloc_pages(size_t n);
static void best_fit_free_pages(struct Page *base, size_t n);
static size_t best_fit_nr_free_pages(void);
const struct pmm_manager best_fit_pmm_manager;
同时保留 basic_check() 和 best_fit_check() 的原始内容，不得改动。

[SPECIFICATION]
best_fit_init：
Pre-Condition：管理器尚未初始化，free_area 可能是未定义状态。
Post-Condition：free_list 为空链表，nr_free 为 0。

best_fit_init_memmap：
Pre-Condition：base 是一段连续页的起始 Page，n > 0，相关页当前处于保留状态。
Post-Condition：这 n 页的 flags 和 property 被重置，引用计数为 0；base 成为唯一
空闲块头，property = n，设置 PageProperty；块按地址插入空闲链表，nr_free 增加 n。
Case 1：链表为空。
Case 2：新块应插入链表头、中间或尾部。
Case 3：n <= 0 或描述符状态不符合前置条件时，用 assert 指出错误。

best_fit_alloc_pages：
Pre-Condition：n > 0。
Post-Condition：在所有 property >= n 的空闲块中选择 property 最小的块；若大小相同，
选择链表中地址较小的块。移除被选块；若 property > n，则在 base + n 处建立剩余空闲块，设置 property 和 PageProperty，并按地址插入链表；清除被分配块头页的PageProperty；nr_free 减少 n；返回被分配块的首个 Page。
Case 1：n 大于 nr_free，返回 NULL，链表状态不变。
Case 2：没有 property >= n 的块，返回 NULL，链表状态不变。
Case 3：刚好相等，整块分配，不产生剩余块。
Case 4：大于 n，分裂并产生剩余块。

best_fit_free_pages：
Pre-Condition：base 是已分配连续页的首个 Page，n > 0，相关页不是保留页且不是
空闲块头。
Post-Condition：这 n 页恢复为空闲状态，base.property = n，设置 PageProperty；
按地址插入链表；如果前一个空闲块与 base 首尾相接则合并，如果后一个空闲块与当前块首尾相接则合并；合并时只保留一个块头，清除被吸收块头的 PageProperty；
nr_free 增加 n。
Case 1：链表为空。
Case 2：插入链表头、中间或尾部。
Case 3：只与前块合并。
Case 4：只与后块合并。
Case 5：前后都可合并，最终形成一个连续块。

best_fit_nr_free_pages：
Pre-Condition：管理器已初始化。
Post-Condition：返回当前空闲页总数 nr_free。

best_fit_pmm_manager：
name 使用 "best_fit_pmm_manager"，各字段分别绑定上述函数和 best_fit_check。
Requirements：不修改 basic_check 和 best_fit_check；不使用 malloc；保持空闲链表
按地址有序；实现必须能通过 -Werror 编译；选择策略必须真的是“最小可用块”，不能退化成 First-Fit。
### Prompt 2：切换 Best-Fit 并运行验证
[PROMPT]
在本地工作副本中把 kern/mm/pmm.c 的 init_pmm_manager() 从 default_pmm_manager临时切换到 best_fit_pmm_manager，运行 Best-Fit 的构建和测试。只做本地验证，不要把这个临时切换当作最终合并结果。依次执行 make clean、make qemu 和 make test，保存完整日志；如果没有出现预期输出，定位并报告失败点，不要修改 basic_check 或best_fit_check。

[RELY]
pmm.c 已经包含 best_fit_pmm.h；best_fit_pmm_manager 已在任务 05 中定义。
make qemu 会通过 pmm_init() 调用 init_pmm_manager()、page_init() 和check_alloc_page()；make test 会使用 -D ucore_test 重新编译，使 best_fit_check()打印 grading 分数。

[GUARANTEE]
交付 lab2-B/02-best-fit/qemu.log、test.log、validation.md 和本地切换的 pmm.cdiff 或 patch。validation.md 必须记录命令、预期输出和实际输出。

[SPECIFICATION]
Pre-Condition：任务 05 已完成；A 的 page_init() 已能提供有效的 pages、npage 和
初始空闲区，或者 A 已提供可运行的基线。
Post-Condition：make qemu 输出 memory management: best_fit_pmm_manager 和
check_alloc_page() succeeded!；make test 输出从 grading: 1 / 6 points 一直到grading: 6 / 6 points，没有断言失败。
Case 1：编译失败，保存第一个真实编译错误，不继续运行。
Case 2：编译成功但启动时断言失败，保存断言位置和当时的调用阶段。
Case 3：只拿到 1-5 分，记录缺失的测试项和对应的 free/alloc 行为。
Case 4：全部通过，保留 QEMU 和测试日志，并准备回滚本地切换。
Requirements：不改测试函数；不把临时切换和最终 pmm.c 合并混淆；最终提交前
要能够恢复或交给 A 处理。
### Prompt 3：记录并修复 Best-Fit 或 Buddy 的 bug
[PROMPT]
根据用户提供的失败日志或复现命令，定位当前实现中的 bug，做最小修复，并形成可提交的 bug 记录。必须先复现问题，再说明根因，再修改相关代码；不得通过删除断言、修改测试函数或跳过测试来“修复”问题。修复后必须用同一组命令重新验证。
本任务只处理一个明确的 bug，不要顺手重构无关代码。

[RELY]
失败日志路径、复现命令、相关源码文件和测试函数由用户提供。Best-Fit 相关文件是kern/mm/best_fit_pmm.c；Buddy 相关文件是 kern/mm/buddy_system_pmm.c 及其测试文件。
测试函数 basic_check、best_fit_check 是框架测试，不得修改。

[GUARANTEE]
交付 lab2-B/bugs/bug-XX.md、修复后的源码 diff、修复前日志和修复后日志。
bug-XX.md 必须包含现象、复现步骤、根因、修改点、影响范围、验证命令和验证结果。

[SPECIFICATION]
Pre-Condition：失败现象可重复出现，已有原始日志或可重新生成日志。
Post-Condition：同一复现命令在修复后通过；相关测试函数保持原样；bug 记录足以让
另一位同学复现和复核。
Case 1：编译错误，记录错误类型和最小修改。
Case 2：运行时断言失败，记录断言条件、调用阶段和对应的 Page/空闲链表状态。
Case 3：Best-Fit 选错块，说明是选择条件、分裂还是链表维护导致的。
Case 4：释放后没有正确合并，说明前块、后块或双向合并中的哪一个路径出错。
Requirements：修复范围最小；不引入无关功能；保留修复前后的对照证据；如果一次
修复引入新问题，也要把迭代过程记录在同一份 bug 文档中。
### Prompt 4：实现 Buddy System 模块
[PROMPT]
在项目中新增 kern/mm/buddy_system_pmm.c 和 kern/mm/buddy_system_pmm.h，从零实现Buddy System 物理内存管理器，并注册为 buddy_system_pmm_manager。必须在项目中实际创建和修改文件，不要只输出代码片段。不要修改 basic_check()、best_fit_check() 或其他成员的代码；如果需要临时切换 pmm.c 做本地验证，必须单独说明并在最终状态恢复。
新文件放在 kern/mm 下，利用 Makefile 已有的目录通配规则自动编译。

[RELY]
可使用 struct Page 的 ref、flags、property、page_link；free_area_t 的 free_list和 nr_free；list_init、list_add、list_del、list_next、list_prev、list_empty；
le2page、PageReserved、PageProperty、SetPageProperty、ClearPageProperty、set_page_ref；以及 pmm_manager 的六个接口。不能使用 malloc；页描述符由内核提前分配。struct Page.property 可用来记录块大小或阶数，但必须在整个模块中保持一致。

[GUARANTEE]
必须交付：
static void buddy_init(void);
static void buddy_init_memmap(struct Page *base, size_t n);
static struct Page *buddy_alloc_pages(size_t n);
static void buddy_free_pages(struct Page *base, size_t n);
static size_t buddy_nr_free_pages(void);
static void buddy_check(void);
const struct pmm_manager buddy_system_pmm_manager;
允许新增 static 辅助函数，但不得改变公开接口。

[SPECIFICATION]
buddy_init：
Pre-Condition：模块尚未初始化。
Post-Condition：所有阶的空闲链表为空，所有计数为 0。

buddy_init_memmap：
Pre-Condition：base 是连续页起始，n > 0，相关描述符可用。
Post-Condition：n 被拆成若干个最大的 2 的幂次块；每个块头满足对齐和阶数一致；
块按设计进入对应阶链表；空闲页计数为 n。
Case 1：n 本身就是 2 的幂次。
Case 2：n 不是 2 的幂次，拆成多个块。
Case 3：块头描述符或起始地址不符合要求时，用 assert 报错。

buddy_alloc_pages：
Pre-Condition：n > 0，模块已初始化。
Post-Condition：将 n 向上取整到块大小；选择一个可满足要求的最小阶块；若块大于请求，
逐级分裂并把剩余伙伴块放回较低阶链表；被分配块头的 PageProperty 被清除；返回块首 Page；空闲页计数减少实际分配的块大小。
Case 1：n 为 0、超过可管理上限或没有可用块，返回 NULL，状态不变。
Case 2：找到同阶块，直接分配。
Case 3：找到更大块，需要逐级分裂。
Case 4：需要跨多个阶寻找可用块。

buddy_free_pages：
Pre-Condition：base 是有效分配块，n 与块大小一致，且未被重复释放。
Post-Condition：块被放回正确阶；如果伙伴块同阶且空闲，则逐级合并；合并后只保留
一个块头，清除被吸收块头的 PageProperty；空闲页计数增加释放的块大小。
Case 1：伙伴不空闲。
Case 2：伙伴空闲并合并一次。
Case 3：连续多次合并到更高阶。
Case 4：重复释放、非法地址或 n 不匹配时用 assert 阻止破坏状态。

buddy_nr_free_pages：返回所有阶空闲页总数。
buddy_check：至少检查空闲链表结构、块对齐、阶数与块大小一致、计数一致；后续测试可以扩展为完整的行为测试。
buddy_system_pmm_manager：name 为 "buddy_system_pmm_manager"，六个字段绑定对应函数。
Requirements：实现必须能通过 -Werror；不能改变 Best-Fit 的测试函数；不能依赖
未定义的全局变量；所有辅助状态初始化和计数更新必须一致。
### Prompt 5：为 Buddy System 设计测试
[PROMPT]
为已生成的 Buddy System 实现设计并编写测试。测试文件放在 kern/mm 下，例如kern/mm/buddy_system_test.c/.h，入口函数命名为 buddy_system_check()。不要创建kern/test 目录，不要修改 basic_check() 或 best_fit_check()，不要通过跳过断言来让测试通过。测试必须覆盖分配、分裂、合并、碎片状态、耗尽、失败请求和完全恢复，并在结束时恢复到初始空闲状态。

[RELY]
Buddy 模块提供 buddy_alloc_pages、buddy_free_pages、buddy_nr_free_pages 和buddy_check；pmm.h 提供 alloc_pages、free_pages、nr_free_pages 宏/接口；
struct Page 提供 property、flags 和 page_link。测试可以使用 assert、cprintf、
PGSIZE、page2pa/pa2page 和链表辅助函数。

[GUARANTEE]
交付 buddy_system_test.c/.h，至少包含以下测试：
1. buddy_test_power_of_two；
2. buddy_test_large_block_split；
3. buddy_test_buddy_merge；
4. buddy_test_mixed_orders；
5. buddy_test_exhaustion；
6. buddy_test_unsatisfiable_request；
7. buddy_test_full_release_restore。
并提供统一入口 buddy_system_check()，全部通过时输出：
buddy_system_check(): PASS

[SPECIFICATION]
Pre-Condition：Buddy 已初始化，当前测试环境有可分配的初始空闲页。
Post-Condition：七类测试全部执行；每次测试结束后释放自己分配的页；最终空闲页数
和空闲链表恢复到测试前状态；控制台输出明确的 PASS 行。

Case 1：2 的幂次页面块，分别分配 1、2、4、8 页，验证返回非空、地址对齐、空闲页数
按实际块大小减少。
Case 2：大块逐级分裂，先分配大块，再分配小块，验证分裂后的伙伴块确实进入较低阶
链表，且请求仍能得到满足。
Case 3：伙伴块释放后的逐级合并，先分配两个伙伴块，再按不同顺序释放，验证最终
合并成一个更高阶块。
Case 4：不同阶空闲块同时存在，交错分配和释放多种大小，验证不会重叠、不会丢失
空闲页、计数始终一致。
Case 5：内存耗尽，持续分配直到返回 NULL，验证每次返回的块互不重叠，计数最终为 0，
并能够逐块释放。
Case 6：无法满足的分配请求，请求超过当前空闲量或超过模块上限，验证返回 NULL 且
空闲页数不变。
Case 7：全部释放后恢复初始状态，重新检查初始空闲页数、空闲链表和各阶块结构；
最后调用 buddy_check()。

Requirements：测试之间不能共享未清理的分配；不能依赖固定物理地址；不要修改
Best-Fit 的测试；所有失败都要保留断言位置和当前分配/释放步骤。
## C（2513582 张子芊）：分支任务——GDB 调试页表查询过程

### Prompt 1：准备调试版 QEMU 并定位地址翻译源码
[PROMPT]
任务：基于 /home/jicheon/oslab-tools/qemu-4.1.1，在独立目录构建带调试信息的 qemu-system-riscv64，验证版本和符号状态，并定位软件 TLB miss、Sv39 页表遍历和 TLB 填充代码。

操作要求：不修改 QEMU 源码，不执行 make install。由实验者运行命令；每次先解释用途，再等待真实输出。函数位置必须以 QEMU 4.1.1 源码为准。

输出要求：给出构建与符号验证结果，以及关键文件、函数和最短调用关系。警告、错误和未验证内容应明确区分。

[RELY]
QEMU 源码：/home/jicheon/oslab-tools/qemu-4.1.1构建目录：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug目标程序：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug/riscv64-softmmu/qemu-system-riscv64构建目标：riscv64-softmmu

相关源码：
target/riscv/cpu_helper.caccel/tcg/cputlb.c

关键函数：
tlb_fill()riscv_cpu_tlb_fill()get_physical_address()tlb_set_page()

[GUARANTEE]
生成独立的 riscv64-softmmu/qemu-system-riscv64；
验证版本为 4.1.1，文件包含调试信息且未 strip；
定位 TLB 查询与 miss、RISC-V 地址翻译和 TLB 填充位置；
说明关键函数在一次地址翻译中的调用关系。

[SPECIFICATION]
任务项：调试版 QEMU
Pre-Condition:
QEMU 4.1.1 源码和构建依赖可用。

Post-Condition:
构建退出状态为 0，目标程序可执行且能被 GDB 解析源码符号。

Requirements:
构建产物保留在独立目录，不覆盖系统 QEMU。

任务项：地址翻译源码
Pre-Condition:
QEMU 版本已经确认，能够搜索并查看源码。

Post-Condition:
找到软件 TLB miss、Sv39 查表、权限检查、物理地址形成和 TLB 填充代码，并整理调用关系。
### Prompt 2：建立 host GDB 与 guest GDB 调试链路
[PROMPT]
任务：在 /mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_new 建立双 GDB 链路：host GDB 调试 QEMU 进程，guest GDB 加载 RISC-V uCore 内核并连接 QEMU GDB stub。

操作要求：实验者在两个终端分别输入命令。每次说明终端编号、命令目的和成功标志，再等待真实输出。host 与 guest 的程序、架构和符号文件不得混用。

输出要求：给出两个终端的启动顺序和命令，确认 QEMU 源码断点可设置、guest GDB 可连接，并说明两边的暂停与继续关系。

[RELY]
项目目录：/mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_newguest 符号文件：bin/kernelguest 镜像：bin/ucore.img

host GDB：/usr/bin/gdbguest GDB：/usr/bin/gdb-multiarch调试版 QEMU：/home/jicheon/oslab-tools/qemu-4.1.1/build-debug/riscv64-softmmu/qemu-system-riscv64

QEMU 参数：
-machine virt -nographic -bios default -kernel bin/ucore.img -s -S

guest GDB 设置：
set architecture riscv:rv64set remotetimeout unlimitedfile bin/kerneltarget remote localhost:1234

[GUARANTEE]
host GDB 正确加载调试版 QEMU 和宿主源码符号；
QEMU 使用 -s -S 打开 GDB stub 并暂停虚拟 CPU；
guest GDB 加载 bin/kernel 并连接 localhost:1234；
能分别控制 QEMU 进程和虚拟 CPU 的暂停与继续。

[SPECIFICATION]
任务项：host GDB
Pre-Condition:
调试版 QEMU 和 bin/ucore.img 已生成。

Post-Condition:
QEMU 在 host GDB 中启动，地址翻译函数可设置源码断点。

任务项：guest GDB
Pre-Condition:
QEMU GDB stub 已监听，虚拟 CPU 处于暂停状态。

Post-Condition:
guest GDB 使用 RISC-V 64 位架构和内核符号连接成功。

Requirements:
在 target remote 前执行 set remotetimeout unlimited。
### Prompt 3：单步验证 Sv39 页表翻译与 TLB 填充
[PROMPT]
任务：在双 GDB 链路中，以内核取指地址 0xffffffffc02000d8 为目标，观察一次由 TLB miss 触发的 Sv39 翻译，记录页表遍历、权限、物理地址和 tlb_set_page() 参数，并完成指导书要求的 TLB 分析。

操作要求：实验者逐条执行 GDB 命令。使用条件断点减少无关中断；每一步先解释观察对象，再等待真实输出。只保留支撑报告结论的必要截图。

输出要求：根据真实调用栈和变量完成虚拟地址到物理地址的手工核对；区分真实访存、调试器辅助查询、源码分析和未验证内容。

[RELY]
目标虚拟地址：0xffffffffc02000d8访问类型：MMU_INST_FETCH架构与分页模式：RISC-V 64 位、Sv39

bool riscv_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
MMUAccessType access_type, int mmu_idx,
bool probe, uintptr_t retaddr);

static int get_physical_address(CPURISCVState *env, hwaddr *physical,
int *prot, target_ulong addr,int access_type, int mmu_idx);

void tlb_set_page(CPUState *cpu, target_ulong vaddr,
hwaddr paddr, int prot,int mmu_idx, target_ulong size);

Sv39 中页内偏移为 12 位，三级 VPN 各为 9 位，PTE 为 8 字节；satp.MODE=8，根页表地址为 satp.PPN << 12。

[GUARANTEE]
捕获 tlb_fill()、riscv_cpu_tlb_fill()、get_physical_address() 的真实调用关系；
记录 address、access_type、mmu_idx、satp、VPN 和页内偏移；
记录实际经过层级的 PTE 地址、PTE 值、PPN、权限和叶子层级；
在 tlb_set_page() 处确认虚拟页、物理页、权限和 MMU 索引；
计算最终物理字节地址，并说明 Bare/Sv39、软件 TLB/硬件 TLB 的区别。

[SPECIFICATION]
任务项：真实 TLB miss
Pre-Condition:
双 GDB 已连接，条件断点限定到目标地址。

Post-Condition:
调用栈经 tlb_fill() 进入 riscv_cpu_tlb_fill()，并记录访问类型和 MMU 索引。

Case 1:
若调用栈经过 riscv_cpu_get_phys_page_debug() 或 cpu_memory_rw_debug()，标记为调试器辅助查询并继续寻找真实访存。

任务项：get_physical_address
Pre-Condition:
已从真实 TLB miss 路径进入函数。

Post-Condition:
根据 satp、VPN 和实际读取的 PTE 得到物理页基址与权限。

Case 1:
非叶子 PTE 更新下一级页表基址并继续遍历。

Case 2:
高层合法叶子 PTE 按超级页规则形成物理页基址。

Requirements:
检查 V/R/W/X/U/G/A/D 位和超级页对齐；物理页基址与页内偏移分别记录。

任务项：tlb_set_page
Pre-Condition:
页表翻译成功。

Post-Condition:
确认映射参数被写入 QEMU 软件 TLB。

Requirements:
未实际观察的 Bare 路径或 TLB fast-path hit 必须标注为源码分析或未验证。
