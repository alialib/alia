#include <alia/base/memory.h>

#include <alia/abi/panic.h>
#include <alia/prelude.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace alia;

namespace {

enum alia_memory_backing
{
    ALIA_MEMORY_BACKING_NONE = 0,
    ALIA_MEMORY_BACKING_VM,
    ALIA_MEMORY_BACKING_HEAP,
    ALIA_MEMORY_BACKING_CALLER,
};

size_t
align_up_size(size_t value, size_t align)
{
    if (align <= 1)
        return value;
    return (value + align - 1u) & ~(align - 1u);
}

void
panic_memory(alia_panic_reason reason, char const* msg)
{
    alia_panic_info info
        = {.reason = reason, .subsystem = "memory", .msg = msg};
    alia_panic_now(&info);
}

// Recover the system from a slot. `pools` is an embedded array and `id` is
// the slot index, so this is the inverse of `&system->pools[id]`.
alia_memory_system*
memory_from_pool(alia_memory_pool* pool)
{
    return reinterpret_cast<alia_memory_system*>(
        reinterpret_cast<char*>(pool - pool->id)
        - offsetof(alia_memory_system, pools));
}

void
call_exhausted(alia_memory_pool* pool, size_t needed)
{
    alia_memory_system* system = memory_from_pool(pool);
    if (system->on_exhausted)
        system->on_exhausted(system->exhausted_user, pool->id, needed);
    panic_memory(ALIA_PANIC_OUT_OF_MEMORY, "pool exhausted");
}

alia_memory_pool*
pool_at(alia_memory_system* memory, alia_memory_pool_id id)
{
    ALIA_ASSERT(memory);
    ALIA_ASSERT(id < ALIA_MEMORY_POOL_SLOT_COUNT);
    return &memory->pools[id];
}

alia_memory_pool*
pool_registered(alia_memory_system* memory, alia_memory_pool_id id)
{
    alia_memory_pool* pool = pool_at(memory, id);
    ALIA_ASSERT(pool->registered);
    return pool;
}

void
note_open(alia_memory_pool* pool)
{
    ALIA_ASSERT(pool->cursor.open_count < UINT8_MAX);
    ++pool->cursor.open_count;
}

void
note_close(alia_memory_pool* pool)
{
    ALIA_ASSERT(pool->cursor.open_count > 0);
    --pool->cursor.open_count;
}

size_t
commit_quantum(alia_memory_system const* memory)
{
    size_t page = ALIA_MEMORY_COMMIT_CHUNK;
    if (memory->ops.vm && memory->ops.vm->page_size > page)
        page = memory->ops.vm->page_size;
    return page;
}

size_t
default_cap(alia_memory_pool const* pool, alia_memory_ops const* ops)
{
    if (pool->kind == ALIA_MEMORY_POOL_GENERAL)
        return ALIA_MEMORY_DEFAULT_GENERAL_CAP;
    if (pool->kind == ALIA_MEMORY_POOL_STACK || ops->vm == nullptr)
        return ALIA_MEMORY_DEFAULT_HEAP_CAP;
    return ALIA_MEMORY_DEFAULT_VM_CAP;
}

void
release_backing(alia_memory_pool* pool)
{
    if (pool->kind == ALIA_MEMORY_POOL_GENERAL)
        return;

    if (!pool->cursor.base
        || pool->cursor.backing == ALIA_MEMORY_BACKING_CALLER)
    {
        pool->cursor.base = nullptr;
        pool->cursor.reserved = 0;
        pool->cursor.committed = 0;
        return;
    }

    alia_memory_system* system = memory_from_pool(pool);
    alia_memory_ops const* ops = &system->ops;
    if (pool->cursor.backing == ALIA_MEMORY_BACKING_VM && ops->vm
        && ops->vm->release)
    {
        ops->vm->release(
            ops->vm->user, pool->cursor.base, pool->cursor.reserved);
    }
    else if (
        pool->cursor.backing == ALIA_MEMORY_BACKING_HEAP && ops->heap.free)
    {
        ops->heap.free(
            ops->heap.user_data, pool->cursor.base, pool->cap, ALIA_MAX_ALIGN);
    }

    pool->cursor.base = nullptr;
    pool->cursor.reserved = 0;
    pool->cursor.committed = 0;
    pool->cursor.backing = ALIA_MEMORY_BACKING_NONE;
}

size_t
pool_grow(
    void* user, void* base, size_t existing_capacity, size_t bytes_requested)
{
    alia_memory_pool* pool = static_cast<alia_memory_pool*>(user);
    alia_memory_system* system = memory_from_pool(pool);
    if (pool->cursor.backing != ALIA_MEMORY_BACKING_VM
        || system->ops.vm == nullptr || system->ops.vm->commit == nullptr)
    {
        call_exhausted(pool, bytes_requested);
    }

    // Another cursor may already have committed past this one. Catch up
    // before reserving another chunk, and never record a smaller prefix.
    size_t const caller_capacity = existing_capacity;
    if (existing_capacity < pool->cursor.committed)
        existing_capacity = pool->cursor.committed;
    if (existing_capacity > caller_capacity
        && existing_capacity
               >= caller_capacity + bytes_requested + ALIA_MAX_ALIGN)
        return existing_capacity;

    size_t const quantum = commit_quantum(system);
    size_t extra = bytes_requested + ALIA_MAX_ALIGN;
    if (extra < quantum)
        extra = quantum;

    size_t new_capacity = existing_capacity + extra;
    if (new_capacity < existing_capacity)
        call_exhausted(pool, bytes_requested);
    new_capacity = align_up_size(new_capacity, quantum);
    if (new_capacity > pool->cap)
        new_capacity = pool->cap;
    if (new_capacity <= existing_capacity)
        call_exhausted(pool, bytes_requested);

    size_t const grow_bytes = new_capacity - existing_capacity;
    void* commit_at = static_cast<uint8_t*>(base) + existing_capacity;
    if (!system->ops.vm->commit(system->ops.vm->user, commit_at, grow_bytes))
        call_exhausted(pool, bytes_requested);

    pool->cursor.committed = new_capacity;
    return new_capacity;
}

void
freeze_cap(alia_memory_pool* pool)
{
    if (pool->frozen)
        return;

    alia_memory_system* memory = memory_from_pool(pool);
    if (pool->kind != ALIA_MEMORY_POOL_GENERAL
        && pool->cursor.backing == ALIA_MEMORY_BACKING_CALLER)
    {
        pool->frozen = 1;
        return;
    }

    if (pool->cap == 0)
        pool->cap = default_cap(pool, &memory->ops);

    if (pool->kind != ALIA_MEMORY_POOL_GENERAL)
    {
        pool->cap = align_up_size(pool->cap, ALIA_MAX_ALIGN);
        if (memory->ops.vm)
            pool->cap = align_up_size(pool->cap, commit_quantum(memory));
    }
    pool->frozen = 1;
}

void
ensure_backing(alia_memory_pool* pool)
{
    freeze_cap(pool);
    if (pool->kind == ALIA_MEMORY_POOL_GENERAL || pool->cursor.base)
        return;

    alia_memory_system* memory = memory_from_pool(pool);
    if (memory->ops.vm)
    {
        void* base = memory->ops.vm->reserve(memory->ops.vm->user, pool->cap);
        if (!base)
            call_exhausted(pool, pool->cap);
        pool->cursor.backing = ALIA_MEMORY_BACKING_VM;
        pool->cursor.base = base;
        pool->cursor.reserved = pool->cap;
        pool->cursor.committed = 0;
        return;
    }

    void* base = memory->ops.heap.alloc(
        memory->ops.heap.user_data, pool->cap, ALIA_MAX_ALIGN);
    if (!base)
        call_exhausted(pool, pool->cap);
    pool->cursor.backing = ALIA_MEMORY_BACKING_HEAP;
    pool->cursor.base = base;
    pool->cursor.reserved = pool->cap;
    pool->cursor.committed = pool->cap;
}

void*
general_alloc(void* user, size_t size, size_t alignment)
{
    alia_memory_pool* pool = static_cast<alia_memory_pool*>(user);
    ALIA_ASSERT(pool->kind == ALIA_MEMORY_POOL_GENERAL);
    ALIA_ASSERT(pool->registered);
    freeze_cap(pool);
    if (size > pool->cap || pool->general.outstanding > pool->cap - size)
        call_exhausted(pool, size);

    alia_memory_system* system = memory_from_pool(pool);
    alia_general_allocator const* heap = &system->ops.heap;
    void* block = heap->alloc(heap->user_data, size, alignment);
    if (!block)
        call_exhausted(pool, size);

    pool->general.outstanding += size;
    if (pool->general.outstanding > pool->general.peak)
        pool->general.peak = pool->general.outstanding;
    return block;
}

void
general_free(void* user, void* ptr, size_t size, size_t alignment)
{
    alia_memory_pool* pool = static_cast<alia_memory_pool*>(user);
    if (pool->general.outstanding >= size)
        pool->general.outstanding -= size;
    else
        pool->general.outstanding = 0;

    alia_memory_system* system = memory_from_pool(pool);
    alia_general_allocator const* heap = &system->ops.heap;
    if (heap->free)
        heap->free(heap->user_data, ptr, size, alignment);
}

alia_grow_callback
pool_grow_callback(alia_memory_pool* pool)
{
    alia_grow_callback callback;
    callback.user = pool;
    callback.grow = pool_grow;
    return callback;
}

void*
system_heap_alloc(void* user, size_t size, size_t alignment)
{
    (void) user;
    if (alignment <= sizeof(void*))
        return std::malloc(size);
#if defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#else
    size_t const rounded = align_up_size(size, alignment);
    return aligned_alloc(alignment, rounded);
#endif
}

void
system_heap_free(void* user, void* ptr, size_t size, size_t alignment)
{
    (void) user;
    (void) size;
    if (!ptr)
        return;
#if defined(_MSC_VER)
    if (alignment > sizeof(void*))
        _aligned_free(ptr);
    else
        std::free(ptr);
#else
    (void) alignment;
    std::free(ptr);
#endif
}

void*
system_vm_reserve(void* user, size_t size)
{
    (void) user;
#if defined(_WIN32)
    return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* block
        = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return block == MAP_FAILED ? nullptr : block;
#endif
}

bool
system_vm_commit(void* user, void* base, size_t size)
{
    (void) user;
#if defined(_WIN32)
    return VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    return mprotect(base, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

void
system_vm_decommit(void* user, void* base, size_t size)
{
    (void) user;
#if defined(_WIN32)
    VirtualFree(base, size, MEM_DECOMMIT);
#else
    mprotect(base, size, PROT_NONE);
#if defined(MADV_DONTNEED)
    madvise(base, size, MADV_DONTNEED);
#endif
#endif
}

void
system_vm_release(void* user, void* base, size_t size)
{
    (void) user;
#if defined(_WIN32)
    (void) size;
    VirtualFree(base, 0, MEM_RELEASE);
#else
    munmap(base, size);
#endif
}

size_t
system_page_size(void)
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
#else
    long page = sysconf(_SC_PAGESIZE);
    return page > 0 ? static_cast<size_t>(page) : 4096u;
#endif
}

void
decommit_tail(alia_memory_pool* pool, size_t keep)
{
    if (pool->cursor.backing != ALIA_MEMORY_BACKING_VM || !pool->cursor.base)
        return;

    alia_memory_system* system = memory_from_pool(pool);
    if (system->ops.vm == nullptr || system->ops.vm->decommit == nullptr)
        return;

    size_t const quantum = commit_quantum(system);
    keep = align_up_size(keep, quantum);
    if (keep >= pool->cursor.committed)
        return;

    void* tail = static_cast<uint8_t*>(pool->cursor.base) + keep;
    system->ops.vm->decommit(
        system->ops.vm->user, tail, pool->cursor.committed - keep);
    pool->cursor.committed = keep;
}

void
clear_published(alia_memory_pool* pool)
{
    pool->cursor.used = 0;
    pool->cursor.retain_peak = 0;
}

void
raise_peak(alia_memory_pool* pool, size_t high)
{
    if (high > pool->cursor.retain_peak)
        pool->cursor.retain_peak = high;
    if (high > pool->cursor.peak)
        pool->cursor.peak = high;
}

} // namespace

extern "C" {

alia_memory_ops
alia_memory_ops_system(void)
{
    static alia_vm_ops vm = {
        .user = nullptr,
        .page_size = 0,
        .reserve = system_vm_reserve,
        .commit = system_vm_commit,
        .decommit = system_vm_decommit,
        .release = system_vm_release,
    };
    vm.page_size = system_page_size();

    alia_memory_ops ops = {};
    ops.vm = &vm;
    ops.heap.user_data = nullptr;
    ops.heap.alloc = system_heap_alloc;
    ops.heap.free = system_heap_free;
    return ops;
}

alia_struct_spec
alia_memory_system_object_spec(void)
{
    return alia_struct_spec{
        .size = sizeof(alia_memory_system),
        .align = alignof(alia_memory_system)};
}

alia_memory_system*
alia_memory_system_init(void* object_storage, alia_memory_ops ops)
{
    ALIA_ASSERT(object_storage);
    ALIA_ASSERT(ops.heap.alloc);
    ALIA_ASSERT(ops.heap.free);
    ALIA_ASSERT(!ops.vm || ops.vm->page_size != 0);

    return new (object_storage)
        alia_memory_system{ops, nullptr, nullptr, {}, 0};
}

void
alia_memory_system_destroy(alia_memory_system* memory)
{
    if (!memory)
        return;

    for (uint16_t id = 0; id < ALIA_MEMORY_POOL_SLOT_COUNT; ++id)
    {
        alia_memory_pool* pool = &memory->pools[id];
        if (!pool->registered)
            continue;
        release_backing(pool);
        pool->registered = 0;
    }
    memory->pool_count = 0;
}

void
alia_memory_set_exhausted_handler(
    alia_memory_system* memory, alia_memory_exhausted_fn fn, void* user)
{
    ALIA_ASSERT(memory);
    memory->on_exhausted = fn;
    memory->exhausted_user = user;
}

alia_memory_pool_id
alia_memory_register_pool(
    alia_memory_system* memory, alia_memory_pool_kind kind, uint32_t flags)
{
    ALIA_ASSERT(memory);
    ALIA_ASSERT(
        kind == ALIA_MEMORY_POOL_BUMP || kind == ALIA_MEMORY_POOL_STACK
        || kind == ALIA_MEMORY_POOL_GENERAL);
    ALIA_ASSERT((flags & ~uint32_t(ALIA_MEMORY_RELEASE_ON_IDLE)) == 0);
    ALIA_ASSERT(
        kind != ALIA_MEMORY_POOL_GENERAL
        || (flags & ALIA_MEMORY_RELEASE_ON_IDLE) == 0);
    if (memory->pool_count == ALIA_MEMORY_POOL_SLOT_COUNT)
    {
        panic_memory(ALIA_PANIC_API_MISUSE, "memory pool table is full");
        return 0;
    }

    alia_memory_pool_id const start = memory->pool_count;
    for (alia_memory_pool_id n = 0; n < ALIA_MEMORY_POOL_SLOT_COUNT; ++n)
    {
        alia_memory_pool_id const id
            = (start + n) % ALIA_MEMORY_POOL_SLOT_COUNT;
        alia_memory_pool* pool = &memory->pools[id];
        if (pool->registered)
            continue;
        *pool = {};
        pool->id = id;
        pool->kind = static_cast<uint8_t>(kind);
        pool->flags = flags;
        pool->registered = 1;
        ++memory->pool_count;
        return id;
    }

    panic_memory(ALIA_PANIC_API_MISUSE, "memory pool table is full");
    return 0;
}

void
alia_memory_unregister_pool(alia_memory_system* memory, alia_memory_pool_id id)
{
    alia_memory_pool* pool = pool_at(memory, id);
    if (!pool->registered)
        return;
    release_backing(pool);
    *pool = {};
    ALIA_ASSERT(memory->pool_count > 0);
    --memory->pool_count;
}

void
alia_memory_set_cap(
    alia_memory_system* memory, alia_memory_pool_id id, size_t cap)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(!pool->frozen);
    ALIA_ASSERT(
        pool->kind == ALIA_MEMORY_POOL_GENERAL
        || pool->cursor.backing != ALIA_MEMORY_BACKING_CALLER);
    if (cap == 0)
        return;
    pool->cap = cap;
}

void
alia_memory_set_storage(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    void* storage,
    size_t size)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(!pool->frozen);
    ALIA_ASSERT(pool->kind != ALIA_MEMORY_POOL_GENERAL);
    ALIA_ASSERT(storage);
    ALIA_ASSERT(size > 0);
    ALIA_ASSERT(
        (reinterpret_cast<uintptr_t>(storage) & (ALIA_MAX_ALIGN - 1u)) == 0);

    pool->cursor.backing = ALIA_MEMORY_BACKING_CALLER;
    pool->cursor.base = storage;
    pool->cursor.reserved = size;
    pool->cursor.committed = size;
    pool->cap = size;
}

alia_bump_allocator
alia_memory_open_bump(alia_memory_system* memory, alia_memory_pool_id id)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(pool->kind == ALIA_MEMORY_POOL_BUMP);

    ensure_backing(pool);
    note_open(pool);

    alia_bump_allocator bump;
    bump.base = static_cast<uint8_t*>(pool->cursor.base);
    bump.capacity = pool->cursor.committed;
    bump.offset = 0;
    bump.grow = pool_grow_callback(pool);
    return bump;
}

alia_stack
alia_memory_open_stack(alia_memory_system* memory, alia_memory_pool_id id)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(pool->kind == ALIA_MEMORY_POOL_STACK);

    ensure_backing(pool);
    ALIA_ASSERT(pool->cursor.committed <= size_t(0xFFFFFFFFu));
    note_open(pool);

    alia_stack stack;
    stack.base = static_cast<uint8_t*>(pool->cursor.base);
    stack.capacity = pool->cursor.committed;
    stack.top = 0;
    stack.top_entry_size = 0;
    stack.peak = 0;
    stack.grow = pool_grow_callback(pool);
    return stack;
}

void
alia_memory_commit_bump(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_bump_allocator const* bump)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(bump);
    raise_peak(pool, bump->offset);
    pool->cursor.used = bump->offset;
    note_close(pool);
}

void
alia_memory_commit_stack(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_stack const* stack)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(stack);

    size_t high = stack->peak;
    if (size_t(stack->top) > high)
        high = stack->top;
    raise_peak(pool, high);
    pool->cursor.used = stack->top;
    note_close(pool);
}

void
alia_memory_discard_bump(alia_memory_system* memory, alia_memory_pool_id id)
{
    note_close(pool_registered(memory, id));
}

void
alia_memory_trim(
    alia_memory_system* memory,
    alia_memory_pool_id id,
    alia_memory_retain retain)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(
        retain == ALIA_MEMORY_RETAIN_PEAK
        || retain == ALIA_MEMORY_RETAIN_USED);
    if (pool->kind == ALIA_MEMORY_POOL_GENERAL)
        return;

    size_t const keep = retain == ALIA_MEMORY_RETAIN_PEAK
                          ? pool->cursor.retain_peak
                          : pool->cursor.used;
    decommit_tail(pool, keep);
}

alia_general_allocator
alia_memory_bind_general(alia_memory_system* memory, alia_memory_pool_id id)
{
    alia_memory_pool* pool = pool_registered(memory, id);
    ALIA_ASSERT(pool->kind == ALIA_MEMORY_POOL_GENERAL);

    alia_general_allocator allocator = {};
    allocator.user_data = pool;
    allocator.alloc = general_alloc;
    allocator.free = general_free;
    return allocator;
}

void
alia_memory_release_idle(alia_memory_system* memory)
{
    if (!memory)
        return;

    for (uint16_t id = 0; id < ALIA_MEMORY_POOL_SLOT_COUNT; ++id)
    {
        alia_memory_pool* pool = &memory->pools[id];
        if (!pool->registered
            || (pool->flags & ALIA_MEMORY_RELEASE_ON_IDLE) == 0)
            continue;
        ALIA_ASSERT(pool->cursor.open_count == 0);
        if (pool->cursor.backing == ALIA_MEMORY_BACKING_CALLER)
            continue;

        if (pool->cursor.backing == ALIA_MEMORY_BACKING_VM)
            decommit_tail(pool, 0);
        else if (pool->cursor.backing == ALIA_MEMORY_BACKING_HEAP)
            release_backing(pool);

        clear_published(pool);
    }
}

alia_memory_pool_stats
alia_memory_pool_query(
    alia_memory_system const* memory, alia_memory_pool_id id)
{
    alia_memory_pool_stats stats = {};
    if (!memory || id >= ALIA_MEMORY_POOL_SLOT_COUNT)
        return stats;

    alia_memory_pool const* pool = &memory->pools[id];
    stats.registered = pool->registered ? 1 : 0;
    if (!pool->registered)
        return stats;

    stats.cap = pool->cap;
    if (pool->kind == ALIA_MEMORY_POOL_GENERAL)
    {
        stats.used = pool->general.outstanding;
        stats.peak_used = pool->general.peak;
    }
    else
    {
        stats.reserved = pool->cursor.reserved;
        stats.committed = pool->cursor.committed;
        stats.used = pool->cursor.used;
        stats.peak_used = pool->cursor.peak;
    }
    return stats;
}

} // extern "C"
