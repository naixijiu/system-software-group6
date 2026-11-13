# Lab4 普通任务 Prompt

## A（2513613 李佳凝）

## B（2512558 张爱迪）

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
