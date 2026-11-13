# 实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| **实验名称** | Lab 4：进程管理 |
| **小组成员** | 2513613-李佳凝、2512558-张爱迪、2513582-张子芊 |
| **完成日期** | 2026-11-12 |

### 小组分工

| 成员 | 负责的练习/模块 |
|---|---|
| 2513613-李佳凝 | 功能模块一：虚拟内存页操作；Challenge 2；最终完整评分 |
| 2512558-张爱迪 | 功能模块二：进程创建及初始化；功能模块三中的 `cpu_idle()`、`proc_run()` |
| 2513582-张子芊 | 迁移 Lab3 Trap 代码；功能模块三中的 `schedule()`、`switch_to()`；Challenge 1 |

实验报告由三人边实现边填写本人负责部分。当前文件已填写 C 的实际工作，A、B 在同一文件中继续补充各自模块。

---

## 一、实验目的

1. 理解内核线程从创建、登记到调度运行的完整过程。
2. 掌握进程控制块、Trap frame、上下文和内核栈之间的关系。
3. 实现轮转调度和 RISC-V 被调用者保存寄存器的上下文切换。
4. 理解通过保存和恢复 `sstatus.SIE` 保护内核临界区的机制。

---

## 二、实验环境

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|---|---|---|---|
| 2513613-李佳凝 | 待 A 填写 | 待 A 填写 | |
| 2512558-张爱迪 | 待 B 填写 | 待 B 填写 | |
| 2513582-张子芊 | Codex 桌面应用 | GPT-5 | Windows 工作区；使用 WSL 中的 RISC-V GCC 与 QEMU 工具链做编译检查 |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

待 A 补充。

### 3.2 功能的逐步实现

待 A 补充。

---

## 四、实验内容与实现

### 功能模块一：虚拟内存页操作

**负责人：** 2513613-李佳凝

待 A 补充。

---

### 功能模块二：进程创建及初始化

**负责人：** 2512558-张爱迪

待 B 补充。

---

### 功能模块三：进程调度与上下文切换

**负责人：** 2512558-张爱迪、2513582-张子芊

#### C 负责的函数

```c
void schedule(void);
void switch_to(struct context *from, struct context *to);
```

#### Lab3 Trap 代码迁移

C 将本组已经验证过的 Lab3 Trap 实现迁入 Lab4：

- 在 `kern/trap/trapentry.S` 中补全 `RESTORE_ALL` 和 `__alltraps`；
- 保留 Lab4 新增的 `forkrets`，没有整文件覆盖；
- 在 `kern/trap/trap.c` 中恢复 `interrupt_handler()` 和 `exception_handler()`；
- 保留原 Lab3 实现者学号，C 只负责迁移和兼容性核对。

`RESTORE_ALL` 先恢复 `sstatus` 和 `sepc`，再按 Trap frame 槽位恢复通用寄存器，最后恢复原始 `sp`。`__alltraps` 将当前 `sp` 作为 Trap frame 指针放入 `a0`，再调用 `trap()`。新内核线程第一次运行时，B 设置的 `context.ra = forkret` 会进入 `forkrets -> __trapret -> RESTORE_ALL -> sret` 恢复链。

#### `schedule()`

`schedule()` 在保存并关闭本地中断后清除 `current->need_resched`。当前进程是 `idleproc` 时从 `proc_list` 表头开始查找，否则从当前进程的后继开始查找。遍历允许越过表头继续，只选择 `PROC_RUNNABLE`；没有普通可运行进程时回退到 `idleproc`。

选中目标后增加 `next->runs`。当 `next != current` 时调用 B 实现的 `proc_run(next)`，最后按进入函数前保存的标志恢复中断状态。

#### `switch_to()`

`switch_to()` 使用 `a0` 指向旧 context、`a1` 指向新 context。它按照 `struct context` 的字段顺序保存旧进程的 `ra`、`sp`、`s0-s11`，再从新进程 context 的相同偏移恢复这些寄存器，最后执行 `ret` 跳转到恢复后的 `ra`。

汇编使用 `STORE`、`LOAD` 和 `REGBYTES` 表示寄存器宽度及偏移，没有硬编码 RV64 的 8 字节槽宽。

#### Spec 和 Prompt

C 的普通任务 Prompt 位于 `report/prompt.md` 的 C 章节：

1. 迁移已验证的 Lab3 Trap 实现；
2. 实现功能模块三的 `schedule()` 和 `switch_to()`。

两份 Prompt 均使用 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 四段结构。第一份限制为迁移已验证代码并保留 `forkrets`；第二份明确进程链表规则、`runs` 计数、上下文字段顺序和寄存器偏移。

#### 实现过程中的问题

完整构建在 A 负责的 `kern/mm/default_pmm.c` 处因缺少 `best_fit_pmm.h` 停止。为验证 C 的实现，单独编译 `trap.c`、`trapentry.S`、`sched.c`、`switch.S` 及 B 的 `proc.c`，均成功生成目标文件。反汇编进一步确认 `switch_to` 按偏移 0 至 104 字节保存和恢复 14 个上下文字段并以 `ret` 返回。完整运行测试等待 A 的代码进入工程后进行。

---

### Challenge 1：`local_intr_save/restore` 如何开关中断

**负责人：** 2513582-张子芊

`local_intr_save(intr_flag)` 展开后调用 `__intr_save()`。该函数读取 `sstatus` 的 `SSTATUS_SIE` 位：如果进入临界区前 SIE 为 1，就调用 `intr_disable()` 清除 SIE，并返回 1；如果原先已经关中断，则不重复操作并返回 0。因此 `intr_flag` 保存的是进入临界区前的中断状态。

`local_intr_restore(intr_flag)` 调用 `__intr_restore()`。它只在 `intr_flag` 为 1 时调用 `intr_enable()` 重新置位 SIE；当 `intr_flag` 为 0 时保持关中断。其行为如下：

| 进入前 SIE | save 后 | `intr_flag` | restore 后 |
|---|---|---|---|
| 1 | SIE 清零 | 1 | SIE 恢复为 1 |
| 0 | 仍为 0 | 0 | 仍为 0 |

该语义能够支持嵌套临界区：外层关闭中断后，内层再次执行 save 时看到 SIE=0，因此内层得到 `intr_flag=0`；内层 restore 不会提前开启中断，最终由外层 restore 恢复原状态。

该机制只关闭当前 hart 的本地中断响应，防止当前 CPU 被中断处理程序打断；它不能阻止其他 hart 同时访问共享数据，因此不能替代多核自旋锁或其他跨 CPU 同步机制。

---

## 五、测试与验证

### C 的实际验证记录

| 验证项 | 实际结果 |
|---|---|
| `kern/trap/trap.c` 编译 | 通过 |
| `kern/trap/trapentry.S` 编译 | 通过 |
| `kern/schedule/sched.c` 单独编译 | 通过 |
| `kern/process/switch.S` 单独编译 | 通过 |
| B 的 `kern/process/proc.c` 接口编译 | 通过 |
| `switch.o` 反汇编偏移检查 | 通过，14 个字段与 `struct context` 一致 |
| `schedule`、Trap handler 符号检查 | 通过 |
| 完整 `make` | 在 A 的 `default_pmm.c` 处失败：缺少 `best_fit_pmm.h` |
| `make qemu` | 未执行，完整工程尚未构建成功 |
| `make grade` | 未执行，等待 A 的功能模块一 |

上述结果仅证明 C 的文件能够编译，不能替代 `[Test Schedule] PASS`。完整工程可编译后，还需运行调度测试和最终评分。

---

## 六、实验总结与收获

### 对操作系统的理解

C 负责部分体现了调度策略和上下文切换机制的分层：`schedule()` 决定“运行谁”，`proc_run()` 负责切换当前进程和地址空间，`switch_to()` 只负责“如何切换寄存器现场”。这种分层使调度策略与体系结构相关汇编相互独立。

Trap frame 与 context 的用途不同：Trap frame 保存异常或中断边界上的完整处理器状态；context 只保存函数调用约定要求被调用者保持的寄存器，用于内核线程之间的主动切换。减少保存范围可以降低上下文切换开销。

### AI 协作开发的经验

提示词中的 `[RELY]` 必须引用真实结构体布局、现有接口和测试条件；汇编任务尤其需要明确字段顺序与寄存器偏移。验证时应区分“单文件编译通过”“完整工程构建通过”和“运行测试通过”，不能用前一种证据替代后一种结论。
