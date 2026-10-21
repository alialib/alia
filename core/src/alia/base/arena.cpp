#include <alia/abi/base/arena.h>

#include <alia/abi/panic.h>
#include <alia/prelude.hpp>

#include <cassert>

using namespace alia;

namespace {

void
panic_bump(void)
{
    alia_panic_info info = {
        .reason = ALIA_PANIC_OUT_OF_MEMORY,
        .subsystem = "memory",
        .msg = "bump allocator exceeded its buffer",
    };
    alia_panic_now(&info);
}

} // namespace

extern "C" {

void
alia_arena_handle_out_of_memory(
    alia_bump_allocator* alloc, size_t bytes_requested)
{
    if (!alloc->grow.grow)
        panic_bump();

    size_t const new_capacity = alloc->grow.grow(
        alloc->grow.user, alloc->base, alloc->capacity, bytes_requested);
    if (new_capacity <= alloc->capacity)
        panic_bump();
    alloc->capacity = new_capacity;
}

alia_offset
alia_arena_alloc_aligned(
    alia_bump_allocator* alloc, size_t bytes, size_t align)
{
    ALIA_ASSERT((bytes & (align - 1)) == 0);
    ALIA_ASSERT(align <= ALIA_MAX_ALIGN);

    if (align <= ALIA_MIN_ALIGN)
        return alia_arena_alloc(alloc, alia_min_aligned_size(bytes));

    size_t aligned_offset = align_offset(alloc->offset, align);
    if (aligned_offset + bytes > alloc->capacity)
        alia_arena_handle_out_of_memory(alloc, bytes);

    alloc->offset = aligned_offset + bytes;
    return aligned_offset;
}

} // extern "C"
