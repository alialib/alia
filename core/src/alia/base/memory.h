#pragma once

#include <alia/abi/base/memory.h>

#include <stdint.h>

// fixed count of memory pool slots - Kernel, UI, and one renderer pool fit
// with room for a few custom pools.
#define ALIA_MEMORY_POOL_SLOT_COUNT 16

#define ALIA_MEMORY_COMMIT_CHUNK (64u * 1024u)
#define ALIA_MEMORY_DEFAULT_VM_CAP (64u * 1024u * 1024u)
#define ALIA_MEMORY_DEFAULT_HEAP_CAP (1u * 1024u * 1024u)
#define ALIA_MEMORY_DEFAULT_GENERAL_CAP (64u * 1024u * 1024u)

// backing of one pool slot - The memory system is the only owner.
struct alia_memory_pool
{
    // hard ceiling in bytes
    size_t cap;

    union
    {
        // bump or stack pool
        struct
        {
            // block address - This is null until the backing is installed,
            // and again after a heap block is released. Caller backing is
            // installed by `alia_memory_set_storage`.
            void* base;
            // reserved address range - equal to `cap` for a heap block or a
            // caller buffer
            size_t reserved;
            // committed prefix of `base`
            size_t committed;
            // last published cursor
            size_t used;
            // high water of published cursors since the last idle release -
            // This is the trim retain line.
            size_t retain_peak;
            // high water of published cursors across idle releases
            size_t peak;
            // open cursors - `alia_memory_release_idle` asserts when this is
            // non-zero on a pool that opted into idle release.
            uint8_t open_count;
            // backing type
            uint8_t backing;
        } cursor;

        // general pool
        struct
        {
            // bytes currently outstanding
            size_t outstanding;
            // high water of outstanding bytes
            size_t peak;
        } general;
    };

    // index of this slot in `alia_memory_system::pools`
    uint32_t flags;
    alia_memory_pool_id id;
    uint8_t kind;
    uint8_t registered;
    // cap and caller storage are frozen
    uint8_t frozen;
};

// `alia_memory_system` is the pool table for one kernel instance.
// A slot recovers this object from its ID. `pools` must stay an embedded
// array, and this struct must stay standard layout.
struct alia_memory_system
{
    alia_memory_ops ops;
    alia_memory_exhausted_fn on_exhausted;
    void* exhausted_user;
    // pool slots - An ID is an index into this array.
    alia_memory_pool pools[ALIA_MEMORY_POOL_SLOT_COUNT];
    // number of registered pools
    alia_memory_pool_id pool_count;
};
