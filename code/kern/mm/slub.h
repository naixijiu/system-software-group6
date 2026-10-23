#ifndef __KERN_MM_SLUB_H__
#define __KERN_MM_SLUB_H__

#include <defs.h>

/*
 * Initialize all SLUB size-class caches.
 *
 * This function must be called after pmm_init(), because the SLUB allocator
 * obtains backing pages through alloc_page().
 */
void slub_init(void);

/*
 * Allocate an object of at least @size bytes.
 *
 * Returns:
 *   a kernel virtual address on success;
 *   NULL if size is zero or allocation fails.
 */
void *slub_alloc(size_t size);

/*
 * Release an object previously returned by slub_alloc().
 *
 * Passing NULL has no effect. Invalid addresses and double frees are treated
 * as allocator errors.
 */
void slub_free(void *ptr);

/*
 * Run the SLUB allocator self-check.
 *
 * The check covers allocation, alignment, data isolation, object reuse and
 * backing-page reclamation.
 */
void slub_check(void);

#endif /* !__KERN_MM_SLUB_H__ */
