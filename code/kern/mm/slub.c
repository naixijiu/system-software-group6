#include <list.h>
#include <pmm.h>
#include <slub.h>
#include <stdio.h>

#define SLUB_CLASS_COUNT 9
#define SLUB_PAGE_MAGIC  0x534C554250414745ULL

/*
 * A free object stores the freelist pointer in its own first machine word.
 * Therefore the minimum supported object size is 8 bytes on RV64.
 */
struct slub_object {
    struct slub_object *next;
};

/*
 * Metadata stored at the beginning of every page managed by SLUB.
 *
 * Object storage starts after this header. The remaining bytes in the
 * 4 KiB page are divided into fixed-size objects.
 */
struct slub_page {
    uint64_t magic;
    unsigned int class_index;
    unsigned int capacity;
    unsigned int inuse;
    struct slub_object *free_list;
    list_entry_t link;
};

/*
 * One cache manages all slab pages belonging to one object-size class.
 *
 * partial_slabs:
 *     pages that still contain at least one free object
 *
 * full_slabs:
 *     pages whose objects have all been allocated
 */
struct slub_cache {
    size_t object_size;
    list_entry_t partial_slabs;
    list_entry_t full_slabs;
    size_t nr_slabs;
};
#define le2slub_page(le) \
    to_struct((le), struct slub_page, link)
static const size_t slub_class_sizes[SLUB_CLASS_COUNT] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048,
};

static struct slub_cache slub_caches[SLUB_CLASS_COUNT];
static bool slub_initialized = 0;

void
slub_init(void) {
    if (slub_initialized) {
        return;
    }

    for (size_t i = 0; i < SLUB_CLASS_COUNT; i++) {
        slub_caches[i].object_size = slub_class_sizes[i];
        list_init(&(slub_caches[i].partial_slabs));
        list_init(&(slub_caches[i].full_slabs));
        slub_caches[i].nr_slabs = 0;
    }

    slub_initialized = 1

;
}
/*
 * Map a requested size to the smallest cache that can contain it.
 *
 * A negative return value means that the request is either empty or larger
 * than the largest small-object class.
 */
static int
slub_size_to_class(size_t size) {
    if (size == 0 || size > slub_class_sizes[SLUB_CLASS_COUNT - 1]) {
        return -1;
    }

    for (size_t i = 0; i < SLUB_CLASS_COUNT; i++) {
        if (size <= slub_class_sizes[i]) {
            return (int)i;
        }
    }

    return -1;
}

/*
 * Allocate one backing page and divide its usable area into objects of the
 * cache's fixed size.
 */
static struct slub_page *
slub_create_page(unsigned int class_index) {
    struct Page *page = alloc_page();
    if (page == NULL) {
        return NULL;
    }

    struct slub_cache *cache = &(slub_caches[class_index]);
    uintptr_t page_begin = page2pa(page) + va_pa_offset;
    uintptr_t page_end = page_begin + PGSIZE;
    struct slub_page *slab = (struct slub_page *)page_begin;

    slab->magic = SLUB_PAGE_MAGIC;
    slab->class_index = class_index;
    slab->inuse = 0;
    slab->free_list = NULL;
    list_init(&(slab->link));

    /*
     * Align the first object to its cache size. All cache sizes are powers
     * of two, so masking can be used safely.
     */
    uintptr_t object_begin = (uintptr_t)(slab + 1);
    object_begin = (object_begin + cache->object_size - 1) &
                   ~(cache->object_size - 1);

    slab->capacity = (page_end - object_begin) / cache->object_size;

    if (slab->capacity == 0) {
        free_page(page);
        return NULL;
    }

    /*
     * Build the freelist inside the objects themselves.
     */
    for (unsigned int i = 0; i < slab->capacity; i++) {
        struct slub_object *object =
            (struct slub_object *)(object_begin + i * cache->object_size);

        object->next = slab->free_list;
        slab->free_list = object;
    }

    list_add(&(cache->partial_slabs), &(slab->link));
    cache->nr_slabs++;

    return slab;
}

void *
slub_alloc(size_t size) {
    if (!slub_initialized) {
        slub_init();
    }

    int class_index = slub_size_to_class(size);
    if (class_index < 0) {
        return NULL;
    }

    struct slub_cache *cache = &(slub_caches[class_index]);
    struct slub_page *slab;

    if (list_empty(&(cache->partial_slabs))) {
        slab = slub_create_page((unsigned int)class_index);
        if (slab == NULL) {
            return NULL;
        }
    }
    else {
        slab = le2slub_page(list_next(&(cache->partial_slabs)));
    }

    struct slub_object *object = slab->free_list;
    if (object == NULL) {
        return NULL;
    }

    slab->free_list = object->next;
    slab->inuse++;

    /*
     * A full slab is moved away from the allocation list, so later
     * allocations do not repeatedly scan a page with no free objects.
     */
    if (slab->inuse == slab->capacity) {
        list_del(&(slab->link));
        list_add(&(cache->full_slabs), &(slab->link));
    }

    return object;
}
/*
 * Return the address of the first object in a slab page.
 */
static uintptr_t
slub_first_object(struct slub_page *slab, struct slub_cache *cache) {
    uintptr_t object_begin = (uintptr_t)(slab + 1);

    return (object_begin + cache->object_size - 1) &
           ~(cache->object_size - 1);
}

/*
 * Check whether an object is already present in the slab's freelist.
 *
 * This is an O(n) check, but it allows the teaching implementation to detect
 * double frees without maintaining a separate allocation bitmap.
 */
static bool
slub_object_is_free(struct slub_page *slab, struct slub_object *target) {
    struct slub_object *object = slab->free_list;

    while (object != NULL) {
        if (object == target) {
            return 1;
        }
        object = object->next;
    }

    return 0;
}

void
slub_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    uintptr_t object_address = (uintptr_t)ptr;
    uintptr_t page_begin = object_address & ~(PGSIZE - 1);
    struct slub_page *slab = (struct slub_page *)page_begin;

    /*
     * The magic value distinguishes SLUB pages from arbitrary kernel pages.
     */
    if (slab->magic != SLUB_PAGE_MAGIC) {
        panic("slub_free: address %p is not owned by SLUB", ptr);
    }

    if (slab->class_index >= SLUB_CLASS_COUNT) {
        panic("slub_free: invalid size class");
    }

    struct slub_cache *cache = &(slub_caches[slab->class_index]);
    uintptr_t first = slub_first_object(slab, cache);
    uintptr_t limit = first + slab->capacity * cache->object_size;

    /*
     * A valid pointer must point exactly to the beginning of an object.
     */
    if (object_address < first || object_address >= limit ||
        (object_address - first) % cache->object_size != 0) {
        panic("slub_free: address %p is not an object boundary", ptr);
    }

    struct slub_object *object = (struct slub_object *)ptr;

    if (slub_object_is_free(slab, object)) {
        panic("slub_free: double free of %p", ptr);
    }

    if (slab->inuse == 0) {
        panic("slub_free: corrupted inuse counter");
    }

    bool was_full = (slab->inuse == slab->capacity);

    object->next = slab->free_list;
    slab->free_list = object;
    slab->inuse--;

    /*
     * The first release from a full slab makes it available for allocation
     * again, so move it back to the partial list.
     */
    if (was_full) {
        list_del(&(slab->link));
        list_add(&(cache->partial_slabs), &(slab->link));
    }

    /*
     * An empty slab is no longer cached: remove it from the cache and return
     * its backing page to the physical page allocator.
     */
    if (slab->inuse == 0) {
        list_del(&(slab->link));
        cache->nr_slabs--;

        uintptr_t physical_address = page_begin - va_pa_offset;
        struct Page *page = pa2page(physical_address);

        slab->magic = 0;
        free_page(page);
    }
}
void
slub_check(void) {
    cprintf("slub_check: begin\n");

    slub_init();

    size_t free_pages_before = nr_free_pages();

    /*
     * Test 1: requests must be placed in the smallest fitting size class.
     */
    static const size_t test_sizes[] = {
        1, 8, 9, 31, 32, 33, 127, 1024, 2048,
    };

    void *test_objects[
        sizeof(test_sizes) / sizeof(test_sizes[0])
    ];

    for (size_t i = 0;
         i < sizeof(test_sizes) / sizeof(test_sizes[0]);
         i++) {
        test_objects[i] = slub_alloc(test_sizes[i]);

        assert(test_objects[i] != NULL);
        assert(((uintptr_t)test_objects[i] & 7) == 0);
    }

    /*
     * A zero-sized request and a request outside the supported small-object
     * range must fail without consuming physical pages.
     */
    assert(slub_alloc(0) == NULL);
    assert(slub_alloc(2049) == NULL);

    for (size_t i = 0;
         i < sizeof(test_sizes) / sizeof(test_sizes[0]);
         i++) {
        slub_free(test_objects[i]);
    }

    assert(nr_free_pages() == free_pages_before);

    /*
     * Test 2: data written to adjacent objects must remain isolated.
     */
    unsigned char *first = slub_alloc(32);
    unsigned char *second = slub_alloc(32);

    assert(first != NULL);
    assert(second != NULL);
    assert(first != second);

    for (size_t i = 0; i < 32; i++) {
        first[i] = 0xA5;
        second[i] = 0x5A;
    }

    for (size_t i = 0; i < 32; i++) {
        assert(first[i] == 0xA5);
        assert(second[i] == 0x5A);
    }

    slub_free(second);
    slub_free(first);

    assert(nr_free_pages() == free_pages_before);

    /*
     * Test 3: a recently freed object should be reused from the freelist.
     *
     * Keep one object allocated so that the backing slab is not reclaimed
     * before the reuse check.
     */
    void *anchor = slub_alloc(32);
    void *released = slub_alloc(32);

    assert(anchor != NULL);
    assert(released != NULL);

    slub_free(released);

    void *reused = slub_alloc(32);
    assert(reused == released);

    slub_free(reused);
    slub_free(anchor);

    assert(nr_free_pages() == free_pages_before);

    /*
     * Test 4: allocate enough objects to require more than one slab page,
     * then verify that every backing page is returned to the PMM.
     */
    void *many_objects[128];

    for (size_t i = 0; i < 128; i++) {
        many_objects[i] = slub_alloc(32);
        assert(many_objects[i] != NULL);
    }

    assert(nr_free_pages() < free_pages_before);

    for (size_t i = 0; i < 128; i++) {
        slub_free(many_objects[i]);
    }

    assert(nr_free_pages() == free_pages_before);

    cprintf("slub_check: passed\n");
}
