# Lab3 普通任务 Prompt

## C（2513582 张子芊）：Lab2 物理内存管理迁移与基线验证

### Prompt：迁移 Best-Fit 与物理页初始化
[PROMPT]
任务：在 lab3_new 中迁移 Lab2 已验证的物理内存管理代码，补全 kern/mm/best_fit_pmm.c 和 kern/mm/pmm.c 中的 Lab2 内容，形成 A、B 可以共同使用的 Lab3 基线。
操作要求：保留 Lab3 在 alloc_pages()、free_pages()、nr_free_pages() 中新增的中断保护；保留原有 lab2 注释和框架测试；不用 Lab2 整文件覆盖 Lab3 文件；不修改 Trap 模块和评分脚本。
输出要求：完成干净编译，记录真实的构建与启动结果；如果 Trap 占位尚未完成导致无法持续运行，必须明确记录为待 A、B 集成，不得伪造通过结果。
[RELY]
项目：lab3_new。
源代码：kern/mm/pmm.c、kern/mm/best_fit_pmm.c、kern/mm/pmm.h、kern/mm/best_fit_pmm.h、kern/driver/dtb.c、kern/sync/sync.h。
Lab2 来源：../lab2_new/kern/mm/pmm.c 和 ../lab2_new/kern/mm/best_fit_pmm.c。
已有接口：get_memory_base()、get_memory_size()、PADDR()、pa2page()、init_memmap()、local_intr_save()、local_intr_restore()、pmm_manager。
[GUARANTEE]
提供完整的 page_init()、Best-Fit 六个管理器接口和 best_fit_pmm_manager。
pmm_manager 选择 best_fit_pmm_manager，启动时可执行 check_alloc_page()。
不修改 basic_check()、best_fit_check()、tools/grade.sh、tools/inject.py 和 tools/test_lab3* 测试文件。
不删除 Lab3 已有的中断屏蔽与恢复逻辑。
[SPECIFICATION]
任务项：page_init
Pre-Condition：DTB 已由 dtb_init() 初始化，PMM 管理器已注册，物理内存从 DRAM_BASE 开始。
Post-Condition：正确设置 va_pa_offset、npage 和 pages；页描述符数组放在内核末尾之后；排除 OpenSBI、内核、描述符数组和对齐空间；通过 init_memmap() 登记剩余完整页。
Case 1：DTB 范围合法且内存充足时，输出物理内存和空闲页范围。
Case 2：DTB 范围无效、溢出、内核末尾不在映射区或没有完整空闲页时，使用 panic() 拒绝继续初始化。
任务项：Best-Fit
Pre-Condition：输入的初始空闲页连续且仍处于 Reserved 状态，分配和释放请求页数大于 0。
Post-Condition：空闲块按地址有序；分配时选择最小可用块并正确分裂；释放时同时处理前块、后块和双向合并；nr_free 与链表中空闲页总数一致。
Requirements：通过 -Werror 构建；出现失败时保留第一个真实错误，不得修改测试或删除断言来制造通过。

## A（2513613 李佳凝）：功能模块一·Trap 上下文恢复与入口

### Prompt 1：实现 RESTORE_ALL
[PROMPT]
任务：在 kern/trap/trapentry.S 的 `#lab3 function1 YOUR CODE` 位置实现 RESTORE_ALL，使其与现有 SAVE_ALL 及 struct trapframe 的 36 个寄存器槽位严格对称。
操作要求：只修改 kern/trap/trapentry.S；保留 `lab3 function1` 注释并将 `YOUR CODE` 替换为 `2513613`；不修改 SAVE_ALL、struct trapframe、trap() 签名和评分文件；不实现 Challenge。
输出要求：在项目中完成汇编实现，运行干净编译，记录代码生成结果、真实错误、修复方式和验证输出。
[RELY]
SAVE_ALL 在当前 sp 上分配 36*REGBYTES，保存 x0–x31 和 sstatus、sepc、sbadaddr、scause。
struct trapframe 的槽位顺序为 x0–x31、status(32)、epc(33)、badvaddr(34)、cause(35)。
SAVE_ALL 通过 sscratch 保留 Trap 发生前的 sp，并把该值写入槽位 2。
libs/riscv.h 定义 LOAD、STORE 和 REGBYTES。
[GUARANTEE]
从槽位 32、33 恢复 sstatus 和 sepc。
恢复 x1 及 x3–x31，不对 x0 做无意义恢复。
x2(sp) 必须最后从槽位 2 恢复，不得在仍需要用当前 sp 访问 Trap frame 时提前覆盖它。
不把 sbadaddr 和 scause 当作 Trap 返回必需 CSR 写回。
[SPECIFICATION]
任务项：RESTORE_ALL
Pre-Condition：sp 指向完整的 struct trapframe，其布局与 SAVE_ALL 完全一致，trap() 已完成处理。
Post-Condition：sstatus、sepc 和所有可恢复通用寄存器回到 Trap frame 记录值；sp 恢复为 Trap 前的值；后续 sret 可从 sepc 记录的逻辑返回点继续执行。
Case 1：普通寄存器通过对应槽位恢复。
Case 2：sp 使用槽位 2 的原值最后恢复。
Requirements：不自行假设新的栈布局；不增加额外栈帧；必须通过 -Werror 编译，并在 B 的异常处理合入后通过 Test 1 寄存器上下文检查。

### Prompt 2：实现 __alltraps 入口
[PROMPT]
任务：在 kern/trap/trapentry.S 的第二个 `#lab3 function1 YOUR CODE` 位置实现 __alltraps 到 C 语言 trap(struct trapframe *tf) 的调用链。
操作要求：保持 SAVE_ALL、__trapret 和 sret 框架不变；遵循 RISC-V ABI；保留 `lab3 function1` 注释并将 `YOUR CODE` 替换为 `2513613`；不实现或启用 Challenge。
输出要求：实际修改源码并编译；与 B 的 handler 合入后执行 Test 1–Test 3，保留真实日志。
[RELY]
__alltraps 入口已执行 SAVE_ALL，此时 sp 就是 struct trapframe *。
trap() 声明为 `void trap(struct trapframe *tf)`，RISC-V 第一个参数通过 a0 传递。
trap() 返回后需要进入 __trapret，RESTORE_ALL 仍依赖 sp 指向原 Trap frame。
[GUARANTEE]
将当前 sp 传给 a0 并调用 trap()。
不在调用前后破坏 Trap frame，不额外改变 sp。
trap() 返回后正确到达 __trapret，形成 SAVE_ALL→trap()→RESTORE_ALL→sret 闭环。
[SPECIFICATION]
任务项：__alltraps
Pre-Condition：处理器已转入 S 态，stvec 指向 __alltraps，SAVE_ALL 成功建立 Trap frame。
Post-Condition：trap() 收到正确的 struct trapframe *；函数返回后 sp 仍指向该 Trap frame；控制流进入 __trapret。
Case 1：断点异常进入 handler 后能够返回 ebreak 后的指令。
Case 2：非法指令异常返回后不重复触发。
Case 3：持续时钟中断可多次进入和返回，不损坏上下文。
Requirements：不把 Trap frame 拷贝到新结构；不修改 trap() 签名；联合 B 的实现后 Test 1 输出 `[Test 1] Trap entry and breakpoint return: PASS`。

## B（2512558 张爱迪）：功能模块二·中断与异常处理

### Prompt 1：实现 interrupt_handler()
[PROMPT]
任务：在 kern/trap/trap.c 的 `//lab3 function2 YOUR CODE` 位置实现 `void interrupt_handler(struct trapframe *tf)`，处理 S 态时钟中断、S 态软件中断和未知中断。
操作要求：只修改 kern/trap/trap.c；保留 `lab3 function2` 注释并将 `YOUR CODE` 替换为 `2512558`；不修改 trap_dispatch()、trap()、时钟驱动和评分文件；不实现 Challenge。
输出要求：完成源码实现与干净编译，保留 AI 生成结果、真实 bug、修复过程及 Test 2、Test 3 日志。
[RELY]
trap_dispatch() 在 tf->cause 最高位为 1 时调用 interrupt_handler()。
libs/riscv.h 定义 IRQ_S_SOFT=1、IRQ_S_TIMER=5；clock.h 提供 ticks 和 clock_set_next_event()；sbi.h 提供 sbi_shutdown()。当前 trap.c 尚未包含 sbi.h，调用 sbi_shutdown() 时必须在该文件顶部添加 `#include <sbi.h>`。
trap.c 已定义 TICK_NUM=100 和 print_ticks()；SHUTDOWN_AFTER 的具体数值必须使用课程网站最新骨架或助教明确给出的值。如果当前材料没有给出，先报告缺失的约束并向 C 确认，不得自行假定。
评分关键字包括 `Supervisor software interrupt`、`Unknown interrupt: cause 15`、`100 ticks` 和 `ticks=200`。
[GUARANTEE]
正确去除 tf->cause 的中断标志位并按中断号分流。
IRQ_S_TIMER 每次先设置下一次时钟事件，再使 ticks 增加 1；每到 TICK_NUM 的整数倍调用 print_ticks()；累计输出达到 SHUTDOWN_AFTER 后调用 sbi_shutdown()。
IRQ_S_SOFT 输出 `Supervisor software interrupt`。
未知中断输出 `Unknown interrupt: cause %d`，并调用 print_trapframe(tf)。
[SPECIFICATION]
任务项：interrupt_handler
Pre-Condition：tf 指向完整 Trap frame。正常 trap_dispatch() 路径中 tf->cause 最高位为 1；评分测试也会直接向 interrupt_handler() 传入不带最高位标志的裸中断号，如 cause=15。实现必须正确取得低位中断原因，不得依赖最高位一定存在。
Post-Condition：已知中断执行规定处理，未知中断输出原因和 Trap frame，控制流可返回统一 Trap 恢复路径。
Case 1：IRQ_S_TIMER 连续触发时，ticks 递增且下一次事件始终被重新设置。
Case 2：IRQ_S_SOFT 仅执行类型提示。
Case 3：中断号 15 输出指定关键字并打印 Trap frame。
Requirements：按助教口径，中断是异步事件，逻辑返回到被打断指令完成后的 PC+4；本 handler 不得再手工对 tf->epc 加 4，否则会额外跳过一条指令。通过 Test 2 的两个 fallback 关键字和 Test 3 的持续时钟中断检查。

### Prompt 2：实现 exception_handler()
[PROMPT]
任务：在 kern/trap/trap.c 的 `//lab3 function2 YOUR CODE` 位置实现 `void exception_handler(struct trapframe *tf)`，处理非法指令、断点和未知异常，并按实际指令长度更新 tf->epc。
操作要求：只修改 kern/trap/trap.c；保持函数签名和 trap_dispatch() 不变；保留 `lab3 function2` 注释并将 `YOUR CODE` 替换为 `2512558`；不修改或规避评分测试。如果 interrupt_handler() 已经完成，必须完整保留其现有实现，本任务只新增或修正 exception_handler()，不得重写已验证的中断处理代码。
输出要求：完成源码实现，运行干净编译，与 A 的 Trap 入口合入后保留 Test 1、Test 2 真实输出和 bug 修复记录。
[RELY]
tf->cause 在异常路径不含中断最高位；CAUSE_ILLEGAL_INSTRUCTION=2，CAUSE_BREAKPOINT=3。
tf->epc 记录触发异常的指令地址。RISC-V 指令低两位为 0b11 时是 32 位指令，否则是 16 位压缩指令。
未知异常只要求调用 print_trapframe(tf) 兜底；Test 2 会传入 cause=20 并检查 `cause    0x00000014`。
评分关键字包括 `Breakpoint at`、`Illegal instruction at` 和 `[Test 2] Illegal instruction return: PASS`。
[GUARANTEE]
CAUSE_ILLEGAL_INSTRUCTION 先使用 `cprintf("Illegal instruction at 0x%08x\n", tf->epc);` 输出原 epc，再按触发指令的 2/4 字节长度增加 tf->epc。
CAUSE_BREAKPOINT 先使用 `cprintf("Breakpoint at 0x%08x\n", tf->epc);` 输出原 epc，再按 ebreak/c.ebreak 的实际 2/4 字节长度增加 tf->epc。
其他异常不伪造恢复行为，仅调用 print_trapframe(tf)。
[SPECIFICATION]
任务项：exception_handler
Pre-Condition：tf 指向完整 Trap frame，tf->cause 为异常原因，tf->epc 指向触发异常的指令。
Post-Condition：非法指令和断点输出规定信息，tf->epc 指向触发指令之后；未知异常的 Trap frame 被完整输出。
Case 1：16 位非法指令或 c.ebreak 使 epc 增加 2。
Case 2：32 位非法指令或 ebreak 使 epc 增加 4。
Case 3：cause=20 使用 print_trapframe(tf) 输出兜底信息。
Requirements：按助教口径，需要重试的可恢复异常保留 epc=PC，无需重试的异常返回 PC+4，无法恢复的异常不应伪造返回；本实验只对非法指令和断点实现跳过触发指令的行为。不得把所有异常统一写成 epc+=4。
