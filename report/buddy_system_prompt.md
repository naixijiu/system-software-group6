# Lab2 Buddy_system Prompt

## B（2512558 张爱迪）:该部分为B负责的功能模块二任务三的Buddy_system设计的两个prompt

### Prompt 1：实现Buddy_system模块

````text
[PROMPT]
在项目中新增 kern/mm/buddy_system_pmm.c 和 kern/mm/buddy_system_pmm.h，从零实现
Buddy System 物理内存管理器，并注册为 buddy_system_pmm_manager。必须在项目中实际
创建和修改文件，不要只输出代码片段。不要修改 basic_check()、best_fit_check() 或
其他成员的代码；如果需要临时切换 pmm.c 做本地验证，必须单独说明并在最终状态恢复。
新文件放在 kern/mm 下，利用 Makefile 已有的目录通配规则自动编译。

[RELY]
可使用 struct Page 的 ref、flags、property、page_link；free_area_t 的 free_list
和 nr_free；list_init、list_add、list_del、list_next、list_prev、list_empty；
le2page、PageReserved、PageProperty、SetPageProperty、ClearPageProperty、
set_page_ref；以及 pmm_manager 的六个接口。不能使用 malloc；页描述符由内核提前
分配。struct Page.property 可用来记录块大小或阶数，但必须在整个模块中保持一致。

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
逐级分裂并把剩余伙伴块放回较低阶链表；被分配块头的 PageProperty 被清除；返回块
首 Page；空闲页计数减少实际分配的块大小。
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
buddy_check：至少检查空闲链表结构、块对齐、阶数与块大小一致、计数一致；后续测试
可以扩展为完整的行为测试。
buddy_system_pmm_manager：name 为 "buddy_system_pmm_manager"，六个字段绑定对应函数。
Requirements：实现必须能通过 -Werror；不能改变 Best-Fit 的测试函数；不能依赖
未定义的全局变量；所有辅助状态初始化和计数更新必须一致。
````

### Prompt 2：为Buddy_system设计测试

````text
[PROMPT]
为已生成的 Buddy System 实现设计并编写测试。测试文件放在 kern/mm 下，例如
kern/mm/buddy_system_test.c/.h，入口函数命名为 buddy_system_check()。不要创建
kern/test 目录，不要修改 basic_check() 或 best_fit_check()，不要通过跳过断言来
让测试通过。测试必须覆盖分配、分裂、合并、碎片状态、耗尽、失败请求和完全恢复，
并在结束时恢复到初始空闲状态。

[RELY]
Buddy 模块提供 buddy_alloc_pages、buddy_free_pages、buddy_nr_free_pages 和
buddy_check；pmm.h 提供 alloc_pages、free_pages、nr_free_pages 宏/接口；
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
````
