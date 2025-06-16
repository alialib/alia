#include <alia/flow/arena.hpp>

namespace alia {

constexpr size_t CHUNK_ALIGNMENT = 64; // maximum supported alignment

struct ArenaChunk
{
    ArenaChunk* next;
    size_t size;
    size_t used;
    alignas(CHUNK_ALIGNMENT) char data[];
};

struct DestructorRecord
{
    DestructorRecord* next;
    void* ptr;
    DestructorFn dtor;
};

struct Arena
{
    ArenaAllocator allocator;
    size_t default_chunk_size;

    ArenaChunk* chunks = nullptr;
    DestructorRecord* dtors = nullptr;
};

// Allocate a new chunk
namespace {

ArenaChunk*
allocate_chunk(Arena* arena, size_t size)
{
    size_t total_size = sizeof(ArenaChunk) + size;
    void* raw = arena->allocator.alloc(
        arena->allocator.user_data, total_size, CHUNK_ALIGNMENT);
    auto* chunk = static_cast<ArenaChunk*>(raw);
    chunk->next = nullptr;
    chunk->size = size;
    chunk->used = 0;
    return chunk;
}

} // namespace

Arena*
create_arena(ArenaAllocator alloc, size_t chunk_size)
{
    auto* arena = static_cast<Arena*>(
        alloc.alloc(alloc.user_data, sizeof(Arena), alignof(Arena)));
    arena->allocator = alloc;
    arena->default_chunk_size = chunk_size;
    arena->chunks = nullptr;
    arena->dtors = nullptr;
    return arena;
}

void
destroy_arena(Arena* arena)
{
    reset_arena(arena);
    arena->allocator.dealloc(arena->allocator.user_data, arena);
}

void*
arena_alloc(Arena* arena, size_t size, size_t alignment)
{
    ArenaChunk* chunk = arena->chunks;

    // Try to use existing chunk
    if (chunk)
    {
        size_t offset = (chunk->used + alignment - 1) & ~(alignment - 1);
        if (offset + size <= chunk->size)
        {
            void* ptr = chunk->data + offset;
            chunk->used = offset + size;
            return ptr;
        }
    }

    // Allocate new chunk
    size_t chunk_size
        = size > arena->default_chunk_size ? size : arena->default_chunk_size;
    ArenaChunk* new_chunk = allocate_chunk(arena, chunk_size);
    new_chunk->next = arena->chunks;
    arena->chunks = new_chunk;

    size_t offset = (0 + alignment - 1) & ~(alignment - 1);
    new_chunk->used = offset + size;
    return new_chunk->data + offset;
}

void
arena_register_destructor(Arena* arena, void* ptr, DestructorFn dtor)
{
    auto* record = static_cast<DestructorRecord*>(arena_alloc(
        arena, sizeof(DestructorRecord), alignof(DestructorRecord)));
    record->ptr = ptr;
    record->dtor = dtor;
    record->next = arena->dtors;
    arena->dtors = record;
}

void
reset_arena(Arena* arena)
{
    // Call destructors in reverse order
    DestructorRecord* r = arena->dtors;
    while (r)
    {
        r->dtor(r->ptr);
        r = r->next;
    }
    arena->dtors = nullptr;

    // Free all chunks
    // TODO: Should this definitely be done here?
    ArenaChunk* chunk = arena->chunks;
    while (chunk)
    {
        ArenaChunk* next = chunk->next;
        arena->allocator.dealloc(arena->allocator.user_data, chunk);
        chunk = next;
    }
    arena->chunks = nullptr;
}

} // namespace alia
