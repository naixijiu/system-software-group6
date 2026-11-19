# Lab4 普通任务 Prompt

## A（2513613 李佳凝）


[PROMPT]

- 任务：使用 GPT-6 协助 A（2513613 李佳凝）恢复 Lab2，实现 page_insert、page_remove 及必要的 page_remove_pte，完成 Challenge 2 和报告。
- 操作要求：先读指导书、分工并检查 lab4 分支及 Git 状态；仅补全 code/kern/mm/pmm.c、default_pmm.c，参考已验证历史实现，简要解释设计。
- 输出要求：给出修改清单、最小 diff、真实测试和未解决问题；沿用现有三份报告文件。

[RELY]

- Windows + WSL Ubuntu，RISC-V GCC、Make、QEMU，Sv39 三级页表；使用项目实际页分配、引用计数和 TLB 接口。
- B 负责 proc.c；C 负责 trap.c、trapentry.S、sched.c、switch.S。最新进度需重新核对。

[GUARANTEE]

- 不覆盖队友代码和报告，不修改评分逻辑与断言；保留 lab 注释，A 的 YOUR CODE 改为 2513613。
- PTE、引用计数、TLB 保持一致；同页重映射净引用不变，删除空映射无副作用，分配失败不泄漏。
- 记录实际 Bug 与复测，不编造 PASS 或截图；展示结果后再决定提交，未经确认不 push。

[SPECIFICATION]

- **恢复 Lab2**：前置为有效内存信息和启动映射；正确初始化页描述符及空闲页，恢复 Best-Fit 分配、拆分和合并，保留原检查函数。
- **page_insert**：前置为有效页目录、已分配物理页及必要同步。成功返回 0，安装目标页及权限并刷新 TLB；新映射增加引用，同页替换仅更新权限，异页替换释放旧引用。失败返回 -E_NO_MEM，保留旧映射并回滚本次必要分配。
- **page_remove**：使用 get_pte(..., 0)；有效映射清除 PTE、减少引用，归零释放并刷新 TLB；不存在或重复删除安全返回。可复用 page_remove_pte。
- **测试**：执行构建与 QEMU，检查 check_alloc_page、check_pgdir、check_boot_pgdir、test_lab4_pmm_basic；三人汇合后 make grade，目标为三项 PASS、30/30，保留真实日志和失败归属。
- **Challenge 2 与报告**：解释 Sv32/Sv39/Sv48 和 get_pte 两段相似代码，讨论查找与分配拆分的利弊；填写函数原理、Spec/Prompt、实际排查、测试与 OS 原理联系。

## B（2512558 张爱迪）

### Prompt 1：实现功能模块二：进程创建及初始化功能

[PROMPT]

- 任务：在 kern/process/proc.c 中实现“进程创建及初始化”功能模块，共 6 个函数：`alloc_proc`、`proc_init`、`kernel_thread`、`do_fork`、`copy_thread`、`copy_mm`。它们一起把当前执行现场包装成第 0 个内核线程 idleproc，并创建第 1 个内核线程 initproc（入口为 init_main，参数为 "Hello world!!"）。本任务只修改 kern/process/proc.c。
- 操作要求：必须在项目中修改实际文件 kern/process/proc.c，不要只输出代码片段；保留文件中已有的 get_pid、`forkret`、`hash_proc`、`find_proc`、`setup_kstack`、`put_kstack`、`do_exit`、`init_main` 等实现，不要删除或改写；把文件末尾 //LAB4 function2 YOUR CODE 位置替换为上述 6 个函数的实现，保留 //LAB4 function2 注释，并把 YOUR CODE 改为 2512558；不要修改 kern/mm/pmm.c（A 负责）、`kern/schedule/sched.c`、`kern/process/switch.S` 及 trap 相关文件（C 负责）；不要修改测试文件；不要在函数外新增全局变量；允许新增 static 辅助函数，但不得改变 kern/process/proc.h 中已声明的公开接口。
- 输出要求：以本提示词 [RELY]、`[GUARANTEE]`、`[SPECIFICATION]` 与工程真实代码为准，不要引入与进程创建无关的功能。

[RELY]

- 真实结构体定义（来自 proc.h / trap.h）：
struct proc_struct {
    enum proc_state state;   // PROC_UNINIT=0, PROC_SLEEPING=1, PROC_RUNNABLE=2, PROC_ZOMBIE=3
    int pid;
    int runs;
    uintptr_t kstack;
    volatile bool need_resched;
    struct proc_struct *parent;
    struct mm_struct *mm;
    struct context context;
    struct trapframe *tf;
    uintptr_t pgdir;         // 保存页表根节点的“物理地址”
    uint32_t flags;
    char name[PROC_NAME_LEN + 1];  // PROC_NAME_LEN = 15
    list_entry_t list_link;
    list_entry_t hash_link;
};

struct context {
    uintptr_t ra, sp, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
};

struct trapframe {
    struct pushregs gpr;   // 顺序：zero,ra,sp,gp,tp,t0,t1,t2,s0,s1,a0..a7,s2..s11,t3..t6
    uintptr_t status;      // 对应 sstatus
    uintptr_t epc;         // 对应 sepc
    uintptr_t badvaddr;
    uintptr_t cause;
};
- 可依赖的接口与符号（均为工程内真实存在的）：
  - kmalloc(size_t) / kfree(void*)
  - alloc_pages(size_t) / free_pages(struct Page *base, size_t)
  - page2kva(struct Page*) / kva2page(void*)
  - memset / memcpy
  - set_proc_name(proc, name) / get_proc_name(proc)
  - get_pid()：`static`，已实现，返回未使用的合法 PID
  - find_proc(pid)：已实现
  - hash_proc(proc)：`static`，已实现，按 pid 挂入 hash_list
  - wakeup_proc(proc)：位于 sched.c，把非 ZOMBIE 且非 RUNNABLE 的进程置为 PROC_RUNNABLE
  - list_init / list_add / list_next / le2proc（libs/list.h）
  - 全局量：`proc_list`、`hash_list[HASH_LIST_SIZE]`、`idleproc`、`initproc`、`current`、`nr_process`
  - bootstack、`bootstacktop`（入口已建好的启动栈）
  - boot_pgdir_pa（内核页表根节点的物理地址）
  - read_csr(sstatus)；`SSTATUS_SPP=0x100`、`SSTATUS_SPIE=0x20`、`SSTATUS_SIE=0x2`
  - kernel_thread_entry（entry.S 汇编入口，已定义）、`forkret`（proc.c static，已实现）、`forkrets`（trapentry.S）
  - CLONE_VM=0x100、`CLONE_THREAD=0x200`（pmm.h）
  - E_NO_MEM=4（error.h）
  - KSTACKPAGE=2、`KSTACKSIZE=KSTACKPAGE*PGSIZE`（memlayout.h）
  - 布局要点：`pushregs` 中的 s0、`s1`、`a0`、`sp` 字段用于构造新线程的首次运行现场

[GUARANTEE]

- 必须在最终代码中出现、且签名与声明完全一致的 6 个函数：
static struct proc_struct *alloc_proc(void);
void proc_init(void);
int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);
static void copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf);
static int copy_mm(uint32_t clone_flags, struct proc_struct *proc);
- copy_thread 与 copy_mm 必须为 static；不得新增/修改 proc.h 中的公开接口；如新增辅助函数，应为 static 且只在 proc.c 内部使用。

[SPECIFICATION]

- alloc_proc：
  - Pre-condition：`pmm` 已初始化，`kmalloc` 可用。
  - Post-Condition：返回一个已分配的新 PCB，所有字段处于安全初始状态。
  - Case 1（成功）：`kmalloc` 成功时，`state=PROC_UNINIT`、`pid=-1`、`runs=0`、`kstack=0`、`need_resched=0`、`parent=NULL`、`mm=NULL`、`context` 全部清零、`tf=NULL`、`pgdir=boot_pgdir_pa`、`flags=0`、`name` 清零，并返回该 PCB 指针。
  - Case 2（失败）：`kmalloc` 返回 NULL 时，直接返回 NULL，不 panic。
- proc_init：
  - Pre-condition：`pmm` 与 vmm 已初始化，`bootstack` 可用。
  - Post-Condition：`proc_list` 与 hash_list 全部初始化；`idleproc` 为第 0 个线程：`pid=0`、`state=PROC_RUNNABLE`、`kstack=(uintptr_t)bootstack`、`need_resched=1`、`parent=NULL`、`mm=NULL`、`tf=NULL`、`runs=0`、`flags=0`、`context.ra=0`、`name="idle"，且 idleproc` 不挂入 proc_list 与 hash_list；`current=idleproc`；`nr_process` 已计入 idleproc；随后创建 initproc：`pid=1`、`state=PROC_RUNNABLE`、`parent=idleproc`、`mm=NULL`、`context.ra` 与 context.sp 均非 0、`name="init"，入口为 init_main`、参数为 "Hello world!!"。
  - Case 1：分配 idleproc 失败 → panic。
  - Case 2：`kernel_thread` 创建 initproc 失败（返回 pid<=0）→ panic。
  - Case 3：正常 → 满足上述 Post-Condition。
- kernel_thread：
  - Pre-condition：`fn` 为线程入口，`arg` 为传给 fn 的参数。
  - Post-Condition：在栈上构造一个清零的临时 trapframe，把首次运行现场编码进去，再以 clone_flags|CLONE_VM、`stack=0` 调用 do_fork，并返回 do_fork 的结果。
  - Case 1（正常）：`gpr.s0=fn`、`gpr.s1=arg`；`status` 取当前 sstatus 并置 SPP、置 SPIE、清 SIE；`epc=kernel_thread_entry`；返回新线程 pid 或负错误码。
- do_fork：
  - Pre-condition：`current` 有效（指向父线程）。
  - Post-Condition：依次完成“分配并初始化 PCB → parent=current → 分配内核栈 → copy_mm → copy_thread → 关中断临界区内分配 pid、`hash_proc`、`list_add` 入 proc_list、`nr_process++` → wakeup_proc 置 RUNNABLE”，并返回新 pid。
  - Case 1：`alloc_proc` 失败 → 返回 -E_NO_MEM，无资源泄漏。
  - Case 2：内核栈分配失败 → 释放已分配的 PCB，返回 -E_NO_MEM。
  - Case 3：`copy_mm` 失败 → 释放内核栈和 PCB，返回错误。
  - Case 4：全部成功 → 新进程处于 RUNNABLE、已登记进 proc_list 与 hash_list、`pid` 唯一，返回 pid。
  - Requirements：`pid` 分配、链表挂接、`nr_process++` 必须在关闭中断的临界区内完成，避免中途被打断导致链表与 pid 状态不一致。
- copy_thread：
  - Pre-condition：`proc->kstack` 已分配好（KSTACKSIZE 字节可用），`tf` 指向要复制的 trapframe。
  - Post-Condition：在新进程内核栈顶端放置 trapframe 副本并令 proc->tf 指向它；`proc->tf` 的 gpr.a0=0（子进程返回值 0）；`gpr.sp` 在 esp 非 0 时用 esp，否则指向 proc->tf；`proc->context.ra=forkret`、`proc->context.sp=proc->tf`，使新线程首次切换后经 forkret → forkrets → __trapret → kernel_thread_entry 运行。
  - Case 1（正常）：按上述完成副本与 context 设置。
- copy_mm：
  - Pre-condition：当前阶段只创建内核线程，`current->mm` 为 NULL。
  - Post-Condition：不复制任何地址空间，`proc->mm` 保持 NULL，返回 0，函数形态保持完整以便后续实验扩展。
  - Case 1：`current->mm` 为 NULL → 返回 0。
  - Case 2：`current->mm` 非 NULL → assert 失败（当前阶段不应发生）。

### Prompt 2：实现功能模块三的cpu_idle()与proc_run()

[PROMPT]

- 任务：在 kern/process/proc.c 中实现进程调度执行链里属于 proc.c 的 2 个函数：`cpu_idle` 与 proc_run。它们与 C 负责的 schedule()（sched.c）和 switch_to()（switch.S）配合，让 idleproc 主动让出 CPU、让 initproc 真正接管 CPU 运行。本任务只修改 kern/process/proc.c。
- 操作要求：必须在项目中修改实际文件 kern/process/proc.c，不要只输出代码片段；保留已有代码；把文件末尾 //LAB4 function3 YOUR CODE 位置替换为上述 2 个函数的实现，保留 //LAB4 function3 注释，并把 YOUR CODE 改为 2512558；不要修改 kern/schedule/sched.c、`kern/process/switch.S`、`kern/trap/trap.c`、`kern/trap/trapentry.S`（C 负责）；不要修改 kern/mm/pmm.c（A 负责）；不要修改测试文件；不要在函数外新增全局变量。
- 输出要求：以本提示词 [RELY]、`[GUARANTEE]`、`[SPECIFICATION]` 与工程真实代码为准，不要引入与进程切换无关的功能。

[RELY]

- 可依赖的接口与符号（均为工程内真实存在的）：
  - 全局量：`current`、`idleproc`、`initproc`、`proc_list`（proc_list 中不含 idleproc）
  - schedule()：`sched.c` 中实现（C 负责），本任务只负责调用
  - switch_to(struct context *from, struct context *to)：`proc.c` 顶部已有 extern 声明，实现在 switch.S
  - local_intr_save(intr_flag) / local_intr_restore(intr_flag)（sync.h）
  - write_csr(satp, val)（riscv.h）
  - SATP_MODE_SV39=8、`RISCV_PGSHIFT=12`、`SATP64_MODE=0xF000000000000000`
  - proc->pgdir：保存页表根节点的物理地址（内核线程均为 boot_pgdir_pa）
  - flush_tlb()（pmm.h，执行 sfence.vma）
  - 参考：`kern/mm/pmm.c` 的 enable_paging() 是 Sv39 写 satp 的正确写法；`libs/riscv.h` 的 lsatp() 是 Sv32 遗留，本任务不可用
  - struct proc_struct 与 struct context 的真实布局见任务 1 的 [RELY]

[GUARANTEE]

- 必须在最终代码中出现、且签名与声明完全一致的 2 个函数：
void cpu_idle(void);            // proc.h 中已带 __attribute__((noreturn))
void proc_run(struct proc_struct *proc);
- 不得新增/修改 proc.h 中的公开接口；如新增辅助函数，应为 static 且只在 proc.c 内部使用。

[SPECIFICATION]

- cpu_idle：
  - Pre-condition：系统初始化完成，`current` 有效，`schedule()` 可用。
  - Post-Condition：永不返回；循环中当 current->need_resched 为真时调用 schedule()，由调度器切换到其他线程。
  - Case 1：`need_resched==0` → 继续循环等待。
  - Case 2：`need_resched!=0` → 调用 schedule()（会经 proc_run 完成切换）。
  - Requirements：函数不得返回；不要在循环里引入会导致死锁的关中断逻辑。
- proc_run：
  - Pre-condition：`proc` 是调度器选出的目标线程，`proc->pgdir` 是其页表根节点物理地址；`proc` 可能与 current 相同。
  - Post-Condition：若 proc!=current，在关闭中断的临界区内完成 current=proc，用 Sv39 方式写 satp，刷新 TLB，再调用 switch_to(&prev->context, &proc->context) 完成现场切换，之后恢复中断；若 proc==current 则直接返回。
  - Case 1：`proc==current` → 直接返回，不切换。
  - Case 2：`proc!=current` → current 更新为 proc；`satp` 按 SATP_MODE_SV39 编码写入（SATP_MODE_SV39<<60 | proc->pgdir>>RISCV_PGSHIFT），不使用 Sv32 遗留写法；`flush_tlb()`；调用 switch_to 完成上下文切换；返回（即再次被调度回来）时 current 已为切换前的目标线程。
  - Requirements：切换过程必须屏蔽中断，保证 current、页表和实际执行现场一致；本函数只做切换，不参与调度策略选择。

## C（2513582 张子芊）

### Prompt 1：迁移已验证的 Lab3 Trap 实现

[PROMPT]

- 任务：把本组已验证的 Lab3 Trap 实现迁移到 Lab4，补全 `lab4_new/kern/trap/trapentry.S` 和 `lab4_new/kern/trap/trap.c` 中标有 `lab3` 的缺失代码。
- 操作要求：以 `.github-lab3-audit/code/kern/trap/trapentry.S` 和 `.github-lab3-audit/code/kern/trap/trap.c` 为已验证来源，只迁移目标文件缺失部分，不得整文件覆盖。保留 Lab4 新增的 `forkrets`、现有 `SAVE_ALL`、`idt_init()`、`trap()`、`vmm.h` 依赖和其他非占位代码；保留原有 Lab3 注释及实现者学号；不得修改评分文件。
- 输出要求：给出两个目标文件的最小补丁和静态核对结果；完整构建受 A 的功能模块一阻塞时必须如实说明，不得伪造测试通过。

[RELY]

- `struct trapframe` 包含 32 个通用寄存器槽和 `status`、`epc`、`badvaddr`、`cause`，共 36 个 `REGBYTES` 槽。`SAVE_ALL` 把原始 `sp` 保存到槽 2，把 `sstatus`、`sepc`、`stval`、`scause` 保存到槽 32 至 35。
- 已验证实现中，`RESTORE_ALL` 先恢复 `sstatus`、`sepc`，再恢复通用寄存器，最后恢复原始 `sp`；`__alltraps` 在 `SAVE_ALL` 后令 `a0 = sp` 并调用 `trap`。
- 已验证 `trap.c` 实现了 `interrupt_handler()` 和 `exception_handler()`，依赖 `sbi.h`、`SHUTDOWN_AFTER`、`clock_set_next_event()`、`ticks`、中断号和异常号定义。

[GUARANTEE]

- `RESTORE_ALL` 正确恢复 CSR、通用寄存器和原始 `sp`。
- `__alltraps` 把当前 Trap frame 地址传给 `trap()`。
- 保留 `forkrets -> __trapret -> RESTORE_ALL -> sret` 路径。
- 中断处理覆盖时钟中断、软件中断和未知中断。
- 异常处理覆盖非法指令、断点和未知异常，并按 16/32 位指令长度更新 `epc`。
- 不覆盖 Lab4 已有代码，不修改 A、B 文件和评分文件。

[SPECIFICATION]

- **任务项：迁移 `RESTORE_ALL` 和 `__alltraps`**
  - Pre-Condition：`sp` 指向当前 `SAVE_ALL` 生成的完整 Trap frame。
  - Post-Condition：`trap(tf)` 收到正确参数；返回时 CSR、通用寄存器和 `sp` 恢复；控制流通过 `sret` 返回。
  - Requirements：不得在其他槽尚未加载时提前覆盖作为基址的 `sp`。
- **任务项：迁移中断处理**
  - Pre-Condition：`tf` 指向完整 Trap frame，`cause` 可能包含最高位中断标志。
  - Post-Condition：已知中断完成处理，未知中断输出原因和 Trap frame。
  - Case 1：时钟中断设置下一次事件并增加 `ticks`。
  - Case 2：软件中断输出提示。
- **任务项：迁移异常处理**
  - Pre-Condition：`tf->epc` 指向触发异常的指令。
  - Post-Condition：非法指令和断点越过 2 或 4 字节的触发指令，未知异常打印 Trap frame。

### Prompt 2：实现功能模块三的 schedule() 和 switch_to()

[PROMPT]

- 任务：完成 Lab4 功能模块三中 C 负责的两个函数：在 `lab4_new/kern/schedule/sched.c` 实现 `schedule()`，在 `lab4_new/kern/process/switch.S` 实现 `switch_to()`。
- 操作要求：只修改上述两个文件；保留 `LAB4 function3` 注释并把 `YOUR CODE` 替换为 `2513582`；不得修改 B 的 `proc.c`、公共头文件、测试文件或评分脚本。`schedule()` 使用现有进程链表和本地中断保护接口；`switch_to()` 使用 `STORE`、`LOAD`、`REGBYTES`，不得硬编码 RV64 字节数。
- 输出要求：给出两个文件的最小补丁，说明调度遍历、idle 保底、`runs` 计数、中断恢复以及 14 个上下文字段偏移；不得把预期输出写成实测结果。

[RELY]

- `proc_list` 是环形双向链表；`idleproc` 不作为普通节点加入该链表。`struct proc_struct` 包含 `state`、`runs`、`need_resched`、`list_link`；`proc_run()` 已由 B 实现，但未增加 `runs`。
- 当前进程为 `idleproc` 时从链表头开始查找，否则从当前进程后继开始；只选择 `PROC_RUNNABLE`；找不到普通可运行进程时使用 `idleproc`。
- `struct context` 依次包含 `ra`、`sp`、`s0-s11` 共 14 个 `uintptr_t` 字段；`a0` 指向 `from`，`a1` 指向 `to`。B 的 `copy_thread()` 已把新线程 `context.ra` 设为 `forkret`。
- `test_lab4_schedule()` 创建两个反复主动调度的内核线程，要求二者都至少运行一次；评分关键字包括 `[Test Schedule] PASS` 和 initproc 的规定输出。

[GUARANTEE]

- `schedule()` 在中断保护区内清除 `current->need_resched`。
- 从正确位置环形遍历，只选择 `PROC_RUNNABLE`，无普通候选时回退到 `idleproc`。
- 选中目标后增加 `next->runs`，仅在 `next != current` 时调用 `proc_run(next)`。
- 所有路径恢复进入函数前的中断状态。
- `switch_to()` 按真实字段顺序保存旧上下文并恢复新上下文的 `ra`、`sp`、`s0-s11`。
- `switch_to()` 使用 `0 * REGBYTES` 至 `13 * REGBYTES` 的对应偏移，最后以恢复后的 `ra` 返回。
- 不修改 A、B 的代码和评分文件。

[SPECIFICATION]

- **任务项：`schedule`**
  - Pre-Condition：`current`、`idleproc` 和 `proc_list` 已完成初始化。
  - Post-Condition：完成一次合法调度选择；若选中其他进程则调用 `proc_run()`；原中断状态得到恢复。
  - Case 1：当前为 idle 时从链表头查找。
  - Case 2：当前非 idle 时从当前节点后继查找并允许越过表头。
  - Case 3：没有普通可运行进程时选择 idle。
- **任务项：`switch_to`**
  - Pre-Condition：`a0` 指向可写的旧 context，`a1` 指向完整的新 context。
  - Post-Condition：旧进程现场已保存，CPU 已装载新进程现场，并从新 `ra` 继续执行。
  - Case 1：新线程第一次运行时进入 `forkret`。
  - Case 2：旧线程再次被调度时从上次切出位置继续执行。
