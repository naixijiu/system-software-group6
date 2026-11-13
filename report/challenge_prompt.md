# Lab4 Challenge Prompt

## A（2513613 李佳凝）

## B（2512558 张爱迪）

## C（2513582 张子芊）

### Challenge 1：说明 local_intr_save/restore 如何开关中断

[PROMPT]

- 任务：结合 Lab4 的真实源码，完成 Challenge 1 的报告答案：说明 `local_intr_save(intr_flag); ... local_intr_restore(intr_flag);` 如何实现中断状态的保存、关闭和恢复。
- 操作要求：只进行源码分析并输出可直接写入实验报告的中文说明，不修改代码。必须逐层解释宏、内联函数、`intr_enable()/intr_disable()` 和 RISC-V `sstatus.SIE` 的关系，并用“原先开中断”和“原先关中断”两个执行场景说明行为。还要说明嵌套临界区为何不会由内层错误开启外层已经关闭的中断，以及这种机制与多核互斥锁的区别。
- 输出要求：给出源码调用链、两种状态的执行表、嵌套场景分析、作用范围、优点和限制。答案必须基于项目真实实现，不使用无法从源码验证的说法。

[RELY]

- `kern/sync/sync.h` 中：
  - `__intr_save()` 读取 `sstatus`，若 `SSTATUS_SIE` 为 1，则调用 `intr_disable()` 并返回 1；否则返回 0。
  - `__intr_restore(flag)` 仅在 `flag` 为 1 时调用 `intr_enable()`。
  - `local_intr_save(x)` 把 `__intr_save()` 的返回值保存到变量 `x`。
  - `local_intr_restore(x)` 调用 `__intr_restore(x)`。
- `kern/driver/intr.c` 中：
  - `intr_enable()` 使用 `set_csr(sstatus, SSTATUS_SIE)` 置位 SIE。
  - `intr_disable()` 使用 `clear_csr(sstatus, SSTATUS_SIE)` 清除 SIE。
- `libs/riscv.h` 定义 `SSTATUS_SIE = 0x00000002`。该位控制当前 hart 在 S 态是否响应可屏蔽中断。

[GUARANTEE]

- 明确 `intr_flag` 保存的是进入临界区前的中断状态，而不是退出时的当前状态。
- 明确原先开中断时：save 返回 1 并关闭中断，restore 根据 1 重新开启中断。
- 明确原先关中断时：save 返回 0 且保持关闭，restore 根据 0 不开启中断。
- 解释嵌套临界区中内层 save 得到 0，因此内层 restore 不会破坏外层的关中断状态。
- 说明它只影响当前 hart 的本地中断响应，不能阻止其他 hart 并发访问共享数据。
- 区分“防止当前 CPU 被中断处理程序打断”和“提供多核之间的互斥”。

[SPECIFICATION]

- **任务项：源码调用链说明**
  - Pre-Condition：读者已知临界区由一对 `local_intr_save/restore` 包围。
  - Post-Condition：读者能够从宏展开一直追踪到对 `sstatus.SIE` 的置位和清零。
- **任务项：状态保存与恢复**
  - Pre-Condition：进入临界区前 SIE 可能为 0 或 1。
  - Post-Condition：临界区执行期间中断保持关闭；退出后恢复到进入前状态，而不是无条件开启中断。
  - Case 1：SIE=1，`intr_flag=1`，进入时关闭，退出时重新开启。
  - Case 2：SIE=0，`intr_flag=0`，进入和退出后都保持关闭。
- **任务项：嵌套与边界**
  - Pre-Condition：外层临界区已经关闭中断，内层再次调用 save/restore。
  - Post-Condition：内层不会提前开启中断；答案同时指出该机制不提供跨 hart 的互斥和内存同步。
