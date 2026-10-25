# Lab2 Challenge Prompt

## A（2513613 李佳凝）：功能模块一·Challenge——硬件可用物理内存范围

### Prompt：硬件可用物理内存范围（思考题）
[PROMPT]
任务：回答 OS 无法提前知道硬件可用物理内存范围时，可以通过哪些方式获取；结合当前实验写入报告，无需实现新模块。

[RELY]
本实验由启动流程传递 DTB 地址，dtb_init() 解析 memory 节点，get_memory_base()/get_memory_size() 向 page_init() 提供 RAM 范围。

[GUARANTEE]
说明 DTB、BIOS E820、UEFI GetMemoryMap 和引导程序传递内存映射；区分硬件 RAM 与排除固件、内核及保留区后的可分配页面，说明盲目读写探测的风险。

[SPECIFICATION]
Pre-Condition：依据真实工程和官方资料。
Post-Condition：给出各方式的适用平台、信息来源、OS 使用方法和引用。
Requirements：标明本实验仅使用 DTB；其他方法是资料分析，不声称完成实现或实测。
## B（2512558 张爱迪）

B 无单独 Challenge。

## C（2513582 张子芊）：功能模块二·Challenge——任意大小的内存单元 SLUB 分配算法

### Prompt 1：实现并接入简化 SLUB
[PROMPT]
任务：在 /mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_new 中新增 kern/mm/slub.c 和 kern/mm/slub.h，实现教学版简化 SLUB，并在 kern/init/init.c 中接入现有物理页分配器。SLUB 以 4 KiB 物理页为后备存储，在页内划分固定大小对象，实现按大小类分配、释放、链表状态转换和空 slab 回收。

操作要求：必须修改项目中的实际文件，而不是只展示代码。保留现有 PMM、First-Fit、Best-Fit 和课程测试逻辑；不得使用宿主机 malloc/free，不得用固定全局对象数组代替真实 slab 页。允许新增 static 辅助函数、模块内宏和静态变量，不改变现有公开 PMM 接口。

输出要求：直接进行文件操作，并使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 中的真实工程信息。完成后分别编译 slub.o 和完整内核；不得通过删除断言、修改课程测试或伪造输出掩盖实现问题。

[RELY]
相关文件kern/mm/pmm.hkern/mm/memlayout.hkern/mm/mmu.hlibs/list.hkern/init/init.c

宏定义与全局状态
#define PGSIZE 4096
extern uint64_t va_pa_offset;

页级分配与地址转换
#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)
struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);
static inline uintptr_t page2pa(struct Page *page);
static inline struct Page *pa2page(uintptr_t pa);

可用链表操作
void list_init(list_entry_t *elm);
void list_add(list_entry_t *listelm, list_entry_t *elm);
void list_del(list_entry_t *listelm);
bool list_empty(list_entry_t *list);
list_entry_t *list_next(list_entry_t *listelm);

初始化顺序kern_init() 先调用 pmm_init()，SLUB 初始化必须位于其后，因为后备页来自 alloc_page()。

[GUARANTEE]
必须新增并实现的公开接口：
void slub_init(void);
void *slub_alloc(size_t size);
void slub_free(void *ptr);
你可以根据需要添加：
static 大小类查找、slab 创建、对象边界与重复释放检查函数；
slab 页、大小类 cache 和 freelist 的内部数据结构；
模块内部 magic、大小类数量和对象大小数组。

[SPECIFICATION]
函数：slub_init
Pre-Condition:
pmm_init() 已完成，alloc_page() 和 free_page() 可用。

Post-Condition:
建立 8、16、32、64、128、256、512、1024、2048 字节九个大小类；每个 cache 的 partial、full 链表为空且状态一致。

Case 1:
首次调用时初始化全部 cache。

Case 2:
重复调用时保持已有状态，不重复初始化或破坏已存在的 slab。

函数：slub_alloc
Pre-Condition:
参数表示所需对象的最小字节数；SLUB 已初始化或函数能够安全触发一次初始化。

Post-Condition:
成功时返回最小可容纳大小类中的一个对象首地址；对象与元数据不重叠，地址至少按 8 字节对齐，计数、freelist 和链表状态一致。

Case 1:
size == 0 或 size > 2048 时返回 NULL，不消耗物理页。

Case 2:
partial 链表中存在空闲对象时，从对应 slab 的 freelist 分配；slab 变满后从 partial 移到 full。

Case 3:
没有可用 partial slab 时调用 alloc_page() 创建后备页，在页首保存元数据，将其余空间切分为对象并建立 freelist；物理页不足时返回 NULL，不得破坏已有状态。

函数：slub_free
Pre-Condition:
ptr 为 NULL，或为先前由 slub_alloc() 返回且尚未释放的对象首地址。

Post-Condition:
非空合法对象被放回所属 slab 的 freelist；使用计数和 partial/full 状态正确更新；slab 完全空闲时，其后备页归还 PMM。

Case 1:
ptr == NULL 时不执行操作。

Case 2:
从 full slab 释放第一个对象时，将该 slab 移回 partial。

Case 3:
释放最后一个在用对象时，将 slab 从 cache 移除，并通过 free_page() 归还后备页。

Case 4:
地址不属于 SLUB、大小类非法、地址不是对象边界、重复释放或计数损坏时调用 panic()，不得静默破坏分配器状态。

Requirements:
slab 页首元数据不能作为普通对象返回。
已分配对象不能同时出现在 freelist 中。
同一 slab 只能位于 partial 或 full 链表之一。
### Prompt 2：设计并实现 SLUB 自检
[PROMPT]
任务：为 /mnt/c/Users/chann/Documents/ChatGPT/lab2/lab2_new/kern/mm/slub.c 中的简化 SLUB 编写确定性的内核自检函数，并在 kern/init/init.c 中于 pmm_init() 和 slub_init() 之后调用。测试需要验证不同大小对象的分配与释放、状态转换和空 slab 回收，同时保持课程原有物理页管理测试不变。

操作要求：必须修改项目中的实际文件。测试只能通过公开 SLUB 接口和现有 PMM 统计接口观察行为，不得修改 SLUB 核心算法迎合断言，不得修改或删除课程原有 page_init、First-Fit、Best-Fit 测试。会触发 panic() 的破坏性非法释放不放入正常启动路径。

输出要求：直接实现自检和最小接入修改，使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION]。只有所有断言通过后才能输出 slub_check: passed；依次执行单文件编译、完整构建、QEMU 运行和课程评分，并区分预期结果与真实结果。

[RELY]
被测公开接口
void slub_init(void);
void *slub_alloc(size_t size);
void slub_free(void *ptr);

可用测试与统计接口
size_t nr_free_pages(void);
void cprintf(const char *fmt, ...);
assert(expression);

已实现的行为约束支持的大小类为 8、16、32、64、128、256、512、1024、2048 字节；零大小和超过 2048 字节的请求失败；空 slab 必须归还 PMM；非法地址、非对象边界和重复释放由 slub_free() 调用 panic()。

[GUARANTEE]
必须新增并实现的测试接口：
void slub_check(void);
必须完成的接入：
pmm_init();
slub_init();
slub_check();
可以添加仅供 slub_check() 使用的局部数组和静态测试数据，不新增与测试无关的公开接口。

[SPECIFICATION]
函数：slub_check
Pre-Condition:
PMM 和 SLUB 已完成初始化，课程原有 check_alloc_page() 已能够通过。

Post-Condition:
所有确定性断言通过；测试申请的对象全部释放；nr_free_pages() 恢复测试前数值；最后输出 slub_check: passed。

Case 1:
分别申请 1、8、9、31、32、33、127、1024、2048 字节，验证请求成功、至少 8 字节对齐且同时存活的对象地址不同。

Case 2:
申请 0 和 2049 字节，验证返回 NULL 且不消耗物理页。

Case 3:
对两个 32 字节对象写入不同数据模式，验证对象内容互不污染。

Case 4:
在同一 slab 中保留一个存活对象，释放另一个对象后再次申请同类对象，验证 freelist 能够复用已释放对象。

Case 5:
分配足够多的 32 字节对象跨越多个 slab 页，确认空闲页数量下降；全部释放后确认空闲页数量恢复。

Requirements:
每个测试阶段都清理自身对象，避免前一阶段影响后一阶段。
非法地址和重复释放的防护通过源码检查说明，不在正常启动自检中故意触发 panic()。
自检通过不能替代课程评分；完整合并版本仍需执行 make grade。
