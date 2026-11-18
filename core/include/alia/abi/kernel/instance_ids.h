#ifndef ALIA_ABI_KERNEL_INSTANCE_IDS_H
#define ALIA_ABI_KERNEL_INSTANCE_IDS_H

#include <alia/abi/prelude.h>

#include <stdint.h>

ALIA_EXTERN_C_BEGIN

// `alia_instance_id` identifies one incarnation of a UI/kernel instance (e.g.
// a substrate identity slot). It is minted from an `alia_instance_id_pool` and
// never reused for the lifetime of that pool. 0 is reserved as "none".
typedef struct alia_instance_id
{
    uint64_t value;
} alia_instance_id;

#define ALIA_INSTANCE_ID_NONE ALIA_BRACED_INIT(alia_instance_id, 0)

static inline bool
alia_instance_id_is_valid(alia_instance_id id)
{
    return id.value != 0;
}

static inline bool
alia_instance_id_equal(alia_instance_id a, alia_instance_id b)
{
    return a.value == b.value;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_instance_id);

// Return the ID at `index` within a contiguous range whose first ID is `base`.
static inline alia_instance_id
alia_instance_id_at(alia_instance_id base, uint32_t index)
{
    return ALIA_BRACED_INIT(alia_instance_id, base.value + index);
}

// mint cursor for instance IDs - Owned by the persistent kernel. Allocation
// may happen inside or outside a pass.
typedef struct alia_instance_id_pool
{
    // next ID to hand out - Starts at 1 so 0 remains NONE.
    alia_instance_id next;
} alia_instance_id_pool;

// Initialize `pool` so the first allocation returns ID 1.
static inline void
alia_instance_id_pool_init(alia_instance_id_pool* pool)
{
    pool->next = ALIA_BRACED_INIT(alia_instance_id, 1);
}

// Allocate `count` IDs from `pool`. Advance the mint cursor by `count` and
// return the first ID. Passing a count of 0 is allowed. It is equivalent to
// peeking at the next ID without actually allocating it.
static inline alia_instance_id
alia_instance_id_alloc_range(alia_instance_id_pool* pool, uint32_t count)
{
    alia_instance_id const base = pool->next;
    pool->next = alia_instance_id_at(base, count);
    return base;
}

// Carve a single ID from `pool`.
static inline alia_instance_id
alia_instance_id_alloc(alia_instance_id_pool* pool)
{
    return alia_instance_id_alloc_range(pool, 1);
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_INSTANCE_IDS_H */
