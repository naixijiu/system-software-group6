#include <pmm.h>
#include <buddy_system_pmm.h>
#include <buddy_system_test.h>
#include <string.h>
#include <stdio.h>

/* Behavioral test suite for the Buddy System manager.
 *
 * Every test records the free-page count before it runs, frees everything it
 * allocated, and asserts the count is restored. At the end the structural
 * self-check buddy_check() verifies the free lists are back to their initial
 * state.
 */

#define BUDDY_TEST_MAX_PAGES (1 << 15)  /* 128 MiB / 4 KiB, upper bound */

static struct Page *buddy_allocated[BUDDY_TEST_MAX_PAGES];
static uint8_t buddy_seen[BUDDY_TEST_MAX_PAGES / 8];

static bool
buddy_overlap(struct Page *a, size_t na, struct Page *b, size_t nb) {
    size_t a0 = page2ppn(a), a1 = a0 + na;
    size_t b0 = page2ppn(b), b1 = b0 + nb;
    return !(a1 <= b0 || b1 <= a0);
}

static struct Page *
buddy_find_buddy(struct Page *p, size_t order) {
    size_t ppn = page2ppn(p);
    size_t bppn = ppn ^ (1UL << order);
    if (bppn < nbase || bppn >= npage) {
        return NULL;
    }
    return &pages[bppn - nbase];
}

/* Allocate 1, 2, 4, 8 pages; verify alignment and exact size accounting. */
static void
buddy_test_power_of_two(void) {
    size_t before = nr_free_pages();
    size_t sizes[4] = {1, 2, 4, 8};
    struct Page *ps[4];
    size_t used = 0;
    for (int i = 0; i < 4; i++) {
        ps[i] = alloc_pages(sizes[i]);
        assert(ps[i] != NULL);
        assert(ps[i]->property == sizes[i]);
        assert(!PageProperty(ps[i]));
        assert(page2ppn(ps[i]) % sizes[i] == 0);
        used += sizes[i];
        assert(nr_free_pages() == before - used);
    }
    for (int i = 3; i >= 0; i--) {
        free_pages(ps[i], sizes[i]);
    }
    assert(nr_free_pages() == before);
}

/* Allocate a large block first, then small blocks; verify splitting works and
 * requests keep succeeding without overlap. */
static void
buddy_test_large_block_split(void) {
    size_t before = nr_free_pages();
    struct Page *big = alloc_pages(64);
    assert(big != NULL);
    assert(big->property == 64);
    assert(page2ppn(big) % 64 == 0);

    struct Page *s0 = alloc_pages(1);
    struct Page *s1 = alloc_pages(1);
    struct Page *s2 = alloc_pages(2);
    assert(s0 != NULL && s1 != NULL && s2 != NULL);
    assert(!buddy_overlap(big, 64, s0, 1));
    assert(!buddy_overlap(big, 64, s1, 1));
    assert(!buddy_overlap(big, 64, s2, 2));
    assert(!buddy_overlap(s0, 1, s1, 1));
    assert(!buddy_overlap(s0, 1, s2, 2));
    assert(!buddy_overlap(s1, 1, s2, 2));

    free_pages(s2, 2);
    free_pages(s1, 1);
    free_pages(s0, 1);
    free_pages(big, 64);
    assert(nr_free_pages() == before);
}

/* Free a block whose buddy may be free; verify merge (or no-merge) and then
 * exercise both free orders of an explicit buddy pair. */
static void
buddy_test_buddy_merge(void) {
    size_t before = nr_free_pages();

    struct Page *p = alloc_pages(4);
    assert(p != NULL && p->property == 4);
    struct Page *b = buddy_find_buddy(p, 2);

    if (b != NULL && PageProperty(b) && b->property == 4) {
        /* buddy is free: freeing p must merge them into an 8-page block */
        free_pages(p, 4);
        struct Page *head = (p < b) ? p : b;
        struct Page *tail = (p < b) ? b : p;
        assert(PageProperty(head) && head->property == 8);
        assert(!PageProperty(tail));
        assert(nr_free_pages() == before);
    } else {
        /* buddy is not free: p stays a standalone 4-page block */
        free_pages(p, 4);
        assert(PageProperty(p) && p->property == 4);
        assert(nr_free_pages() == before);
    }

    /* allocate an explicit pair and free in different orders */
    struct Page *q0 = alloc_pages(4);
    struct Page *q1 = alloc_pages(4);
    assert(q0 != NULL && q1 != NULL && q0 != q1);
    assert(!buddy_overlap(q0, 4, q1, 4));
    if (buddy_find_buddy(q0, 2) == q1) {
        free_pages(q1, 4);
        free_pages(q0, 4);
        struct Page *head = (q0 < q1) ? q0 : q1;
        assert(PageProperty(head) && head->property == 8);
    } else {
        free_pages(q1, 4);
        free_pages(q0, 4);
    }
    assert(nr_free_pages() == before);
}

/* Interleave allocations/frees of several orders; verify no overlap and that
 * the free-page count stays consistent. */
static void
buddy_test_mixed_orders(void) {
    size_t before = nr_free_pages();
    struct Page *a = alloc_pages(3);   /* 4 pages */
    struct Page *b = alloc_pages(6);   /* 8 pages */
    struct Page *c = alloc_pages(1);   /* 1 page */
    struct Page *d = alloc_pages(18);  /* 32 pages */
    assert(a != NULL && b != NULL && c != NULL && d != NULL);
    assert(a->property == 4 && b->property == 8);
    assert(c->property == 1 && d->property == 32);

    assert(!buddy_overlap(a, 4, b, 8) && !buddy_overlap(a, 4, c, 1));
    assert(!buddy_overlap(a, 4, d, 32) && !buddy_overlap(b, 8, c, 1));
    assert(!buddy_overlap(b, 8, d, 32) && !buddy_overlap(c, 1, d, 32));

    free_pages(b, 8);
    free_pages(d, 32);
    free_pages(a, 4);
    free_pages(c, 1);
    assert(nr_free_pages() == before);
}

/* Allocate one page at a time until allocation fails; verify every page is
 * distinct, the counter reaches zero, and everything can be freed back. */
static void
buddy_test_exhaustion(void) {
    size_t total = nr_free_pages();
    assert(total <= BUDDY_TEST_MAX_PAGES);
    memset(buddy_seen, 0, sizeof(buddy_seen));

    size_t n = 0;
    while (n < BUDDY_TEST_MAX_PAGES) {
        struct Page *p = alloc_pages(1);
        if (p == NULL) {
            break;
        }
        size_t idx = page2ppn(p) - nbase;
        assert(idx < BUDDY_TEST_MAX_PAGES);
        assert((buddy_seen[idx >> 3] & (1 << (idx & 7))) == 0);
        buddy_seen[idx >> 3] |= (1 << (idx & 7));
        buddy_allocated[n++] = p;
    }
    assert(n == total);
    assert(nr_free_pages() == 0);

    for (size_t i = 0; i < n; i++) {
        free_pages(buddy_allocated[i], 1);
    }
    assert(nr_free_pages() == total);
}

/* Requests that cannot be satisfied must return NULL and change nothing. */
static void
buddy_test_unsatisfiable_request(void) {
    size_t before = nr_free_pages();

    assert(alloc_pages(0) == NULL);
    assert(nr_free_pages() == before);

    /* request far more than the machine can ever provide */
    assert(alloc_pages(1UL << 31) == NULL);
    assert(nr_free_pages() == before);
}

/* Allocate a mix, free everything, restore the initial free-page count, then
 * re-verify the free-list structure with buddy_check(). */
static void
buddy_test_full_release_restore(void) {
    size_t total = nr_free_pages();
    struct Page *p0 = alloc_pages(1);
    struct Page *p1 = alloc_pages(5);
    struct Page *p2 = alloc_pages(17);
    struct Page *p3 = alloc_pages(3);
    assert(p0 != NULL && p1 != NULL && p2 != NULL && p3 != NULL);

    free_pages(p3, 4);
    free_pages(p2, 32);
    free_pages(p1, 8);
    free_pages(p0, 1);
    assert(nr_free_pages() == total);

    buddy_check();
}

void
buddy_system_check(void) {
    buddy_test_power_of_two();
    buddy_test_large_block_split();
    buddy_test_buddy_merge();
    buddy_test_mixed_orders();
    buddy_test_exhaustion();
    buddy_test_unsatisfiable_request();
    buddy_test_full_release_restore();
    cprintf("buddy_system_check(): PASS\n");
}
