#ifndef ALIA_ABI_BASE_MEMORY_H
#define ALIA_ABI_BASE_MEMORY_H

#include <alia/abi/base/allocator.h>
#include <alia/abi/base/arena.h>
#include <alia/abi/base/stack.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_memory_system alia_memory_system;

// index of a pool slot
typedef uint8_t alia_memory_pool_id;

// cursor kind for a pool
typedef enum alia_memory_pool_kind
{
    // bump cursor
    ALIA_MEMORY_POOL_BUMP = 0,
    // stack cursor
    ALIA_MEMORY_POOL_STACK,
    // counted heap allocations
    ALIA_MEMORY_POOL_GENERAL,
} alia_memory_pool_kind;

// published figure that `alia_memory_trim` keeps committed
typedef enum alia_memory_retain
{
    // high-water mark of committed cursors
    ALIA_MEMORY_RETAIN_PEAK = 0,
    // last published cursor
    ALIA_MEMORY_RETAIN_USED,
} alia_memory_retain;

// Drop this pool's pass-local memory from `alia_memory_release_idle`.
// Illegal on a general pool. The caller of `alia_memory_release_idle` must
// not have an open cursor on any pool that passes this flag.
#define ALIA_MEMORY_RELEASE_ON_IDLE (1u << 0)

// vtable for an allocator that supports virtual memory management
typedef struct alia_memory_ops_virtual
{
    void* user;
    // commit granule in bytes
    size_t page_size;
    // Reserve an address range of `size` bytes. (Do not commit any pages.)
    void* (*reserve)(void* user, size_t size);
    // Commit `size` bytes at `base`. `base` and `size` are page-aligned and
    // lie inside a reservation. False means the platform refused.
    bool (*commit)(void* user, void* base, size_t size);
    // Drop commit for a page-aligned range and keep the reservation.
    void (*decommit)(void* user, void* base, size_t size);
    // Release a whole reservation.
    void (*release)(void* user, void* base, size_t size);
} alia_vm_ops;

// platform allocation capabilities
typedef struct alia_memory_ops
{
    // virtual memory allocator capabilities - This can be NULL and the system
    // will fall back to using the heap for all allocations.
    alia_vm_ops const* vm;
    // heap allocator capabilities - This is always present.
    alia_general_allocator heap;
} alia_memory_ops;

// Get an `alia_memory_ops` structure that models the capabilities of the
// native system.
alia_memory_ops
alia_memory_ops_system(void);

// callback for when a pool hits its cap or the platform refuses a commit -
// This must not return to the allocator.
typedef void (*alia_memory_exhausted_fn)(
    void* user, alia_memory_pool_id pool, size_t needed);

// Get the size and alignment of `alia_memory_system`.
alia_struct_spec
alia_memory_system_object_spec(void);

// Initialize memory system in `object_storage`.
alia_memory_system*
alia_memory_system_init(void* object_storage, alia_memory_ops ops);

// Release every pool backing and end the lifetime of `memory`.
// (Caller-provided storage is not freed.)
void
alia_memory_system_destroy(alia_memory_system* memory);

// Install `fn` as the exhaustion handler. `fn` must not return.
// Pass `NULL` to restore the default handler, which panics with
// `ALIA_PANIC_OUT_OF_MEMORY`.
void
alia_memory_set_exhausted_handler(
    alia_memory_system* memory, alia_memory_exhausted_fn fn, void* user);

// Register a pool and return its ID.
alia_memory_pool_id
alia_memory_register_pool(
    alia_memory_system* memory, alia_memory_pool_kind kind, uint32_t flags);

// Drop a pool and release its backing. (Caller-provided storage is not freed.)
void
alia_memory_unregister_pool(
    alia_memory_system* memory, alia_memory_pool_id id);

// Set the pool ceiling. Pass `0` to keep the default.
// This can only be called before the first use of the pool.
void
alia_memory_set_cap(
    alia_memory_system* memory, alia_memory_pool_id id, size_t cap);

// Install `storage` as the (caller-owned) backing memory for a pool.
// The pool's `cap` becomes `size`. (It will not try to grow.)
// This can only be called before the first use of the pool.
void
alia_memory_set_storage(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    void* storage,
    size_t size);

// Open a bump cursor at offset zero. This counts as a use of the pool in that
// it will reserve the backing and freeze the cap.
alia_bump_allocator
alia_memory_open_bump(alia_memory_system* memory, alia_memory_pool_id id);

// Open a stack cursor at the bottom. This counts as a use of the pool in that
// it will reserve the backing and freeze the cap.
alia_stack
alia_memory_open_stack(alia_memory_system* memory, alia_memory_pool_id id);

// Publish `bump`'s cursor and close it. The cursor must already sit at the
// end of the allocated space for proper tracking of peak usage.
void
alia_memory_commit_bump(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_bump_allocator const* bump);

// Publish `stack`'s top and local peak, then close it.
void
alia_memory_commit_stack(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_stack const* stack);

// Close a bump cursor without publishing. The pool's published usage stays as
// it was.
void
alia_memory_discard_bump(alia_memory_system* memory, alia_memory_pool_id id);

// If the pool is backed by virtual memory, drop commit above `retain`.
// Otherwise, do nothing.
void
alia_memory_trim(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_memory_retain retain);

// Return an allocator for a general pool. (This does not reserve any memory.)
// The pool's `cap` freezes on the first allocation.
alia_general_allocator
alia_memory_bind_general(alia_memory_system* memory, alia_memory_pool_id id);

// Give back memory for pools that passed `ALIA_MEMORY_RELEASE_ON_IDLE`.
// Virtual reservations stay. Heap blocks are freed and allocated again on
// the next open. Caller buffers stay. The published cursor and the trim
// retain line are cleared so the next commit cannot republish dropped
// pages. The lifetime peak stays.
void
alia_memory_release_idle(alia_memory_system* memory);

typedef struct alia_memory_pool_stats
{
    size_t cap;
    size_t reserved;
    size_t committed;
    // last published cursor, or outstanding bytes for a general pool
    size_t used;
    // high water of `used` across idle releases
    size_t peak_used;
    int registered;
} alia_memory_pool_stats;

// Get the stats for a pool. An unknown ID yields zeroes.
alia_memory_pool_stats
alia_memory_pool_query(
    alia_memory_system const* memory, alia_memory_pool_id id);

ALIA_EXTERN_C_END

#endif
