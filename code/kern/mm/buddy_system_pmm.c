#include <pmm.h>
#include <list.h>
#include <buddy_system_pmm.h>

/* Buddy System physical memory manager.
 *
 * Free memory is organized in power-of-two orders: an order-k free block is
 * exactly 2^k pages. Each order has its own free list; a free block is
 * represented by its head Page, whose `property` records the block size (2^k)
 * and whose PG_property flag marks it as a free-block head.
 *
 * The buddy of a page with physical page number `ppn` at order k is obtained by
 * flipping bit k of `ppn` (ppn ^ (1 << k)). For this relation to be correct,
 * an order-k block must always start at a page number aligned to 2^k.
 */

#define BUDDY_MAX_ORDER 20

static free_area_t buddy_area[BUDDY_MAX_ORDER + 1];
static size_t buddy_nr_free;

static size_t
buddy_floor_log2(size_t n) {
    size_t k = 0;
    while (n > 1) {
        n >>= 1;
        k++;
    }
    return k;
}

static size_t
buddy_ceil_log2(size_t n) {
    size_t k = 0;
    size_t p = 1;
    while (p < n) {
        p <<= 1;
        k++;
    }
    return k;
}

static bool
buddy_is_power_of_2(size_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

/* Largest order k such that ppn is aligned to 2^k (i.e. trailing zero bits). */
static size_t
buddy_ppn_align_order(size_t ppn) {
    size_t k = 0;
    while (k < BUDDY_MAX_ORDER && (ppn & 1) == 0) {
        ppn >>= 1;
        k++;
    }
    return k;
}

static struct Page *
buddy_get_buddy(struct Page *page, size_t order) {
    size_t ppn = page2ppn(page);
    size_t buddy_ppn = ppn ^ (1UL << order);
    if (buddy_ppn < nbase || buddy_ppn >= npage) {
        return NULL;
    }
    return &pages[buddy_ppn - nbase];
}

static void
buddy_init(void) {
    for (size_t k = 0; k <= BUDDY_MAX_ORDER; k++) {
        list_init(&buddy_area[k].free_list);
        buddy_area[k].nr_free = 0;
    }
    buddy_nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }

    struct Page *pos = base;
    size_t rem = n;
    while (rem > 0) {
        size_t ppn = page2ppn(pos);
        size_t align = buddy_ppn_align_order(ppn);
        size_t fit = buddy_floor_log2(rem);
        size_t k = (align < fit) ? align : fit;
        assert(k <= BUDDY_MAX_ORDER);

        pos->property = (1UL << k);
        SetPageProperty(pos);
        list_add(&buddy_area[k].free_list, &pos->page_link);
        buddy_area[k].nr_free += (1UL << k);

        pos += (1UL << k);
        rem -= (1UL << k);
    }
    buddy_nr_free = n;
}

static struct Page *
buddy_alloc_pages(size_t n) {
    if (n == 0) {
        return NULL;
    }
    size_t K = buddy_ceil_log2(n);
    if (K > BUDDY_MAX_ORDER) {
        return NULL;
    }

    /* find the smallest order >= K that has a free block */
    size_t j = K;
    while (j <= BUDDY_MAX_ORDER && list_empty(&buddy_area[j].free_list)) {
        j++;
    }
    if (j > BUDDY_MAX_ORDER) {
        return NULL;
    }

    struct Page *page = le2page(list_next(&buddy_area[j].free_list), page_link);
    list_del(&page->page_link);
    buddy_area[j].nr_free -= (1UL << j);

    /* split the block down to order K; each upper half becomes a free buddy */
    while (j > K) {
        j--;
        struct Page *buddy = page + (1UL << j);
        buddy->property = (1UL << j);
        SetPageProperty(buddy);
        list_add(&buddy_area[j].free_list, &buddy->page_link);
        buddy_area[j].nr_free += (1UL << j);
        page->property = (1UL << j);
    }

    ClearPageProperty(page);
    buddy_nr_free -= (1UL << K);
    return page;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    assert(buddy_is_power_of_2(n));
    size_t k = buddy_floor_log2(n);
    assert(k <= BUDDY_MAX_ORDER);
    assert(base->property == n);
    assert((page2ppn(base) % n) == 0);

    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }

    base->property = n;
    SetPageProperty(base);
    list_add(&buddy_area[k].free_list, &base->page_link);
    buddy_area[k].nr_free += n;
    buddy_nr_free += n;

    /* repeatedly merge with a free same-order buddy */
    while (k < BUDDY_MAX_ORDER) {
        struct Page *buddy = buddy_get_buddy(base, k);
        if (buddy == NULL || !PageProperty(buddy) ||
            buddy->property != (1UL << k)) {
            break;
        }
        list_del(&buddy->page_link);
        list_del(&base->page_link);
        buddy_area[k].nr_free -= (1UL << (k + 1));
        ClearPageProperty(buddy);
        ClearPageProperty(base);
        if (buddy < base) {
            base = buddy;
        }
        k++;
        base->property = (1UL << k);
        SetPageProperty(base);
        list_add(&buddy_area[k].free_list, &base->page_link);
        buddy_area[k].nr_free += (1UL << k);
    }
}

static size_t
buddy_nr_free_pages(void) {
    return buddy_nr_free;
}

void
buddy_check(void) {
    size_t total = 0;
    for (size_t k = 0; k <= BUDDY_MAX_ORDER; k++) {
        list_entry_t *le = &buddy_area[k].free_list;
        while ((le = list_next(le)) != &buddy_area[k].free_list) {
            struct Page *p = le2page(le, page_link);
            assert(PageProperty(p));
            assert(p->property == (1UL << k));
            assert((page2ppn(p) % (1UL << k)) == 0);
            total += (1UL << k);
        }
    }
    assert(total == buddy_nr_free);
}

const struct pmm_manager buddy_system_pmm_manager = {
    .name = "buddy_system_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
