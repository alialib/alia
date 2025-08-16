#pragma once

#include <cstddef>

namespace alia {

struct Arena;

using AllocatorFn = void* (*) (void* user_data, size_t size, size_t alignment);
using DeallocatorFn = void (*)(void* user_data, void* ptr);

struct ArenaAllocator
{
    AllocatorFn alloc;
    DeallocatorFn dealloc;
    void* user_data;
};

Arena*
create_arena(ArenaAllocator allocator, size_t chunk_size = 4096);
void
destroy_arena(Arena*);

void*
arena_alloc(Arena*, size_t size, size_t alignment = alignof(std::max_align_t));

// Used to register a destructor to be called when the arena is reset or
// destroyed.
using DestructorFn = void (*)(void* ptr);
void
arena_register_destructor(Arena*, void* ptr, DestructorFn);

// Reset the arena by invoking all destructors and freeing all chunks.
void
reset_arena(Arena*);

} // namespace alia
