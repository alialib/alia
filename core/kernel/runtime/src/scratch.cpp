#include <alia/scratch.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {

// INTERNAL TYPES

// 1) Pick a chunk/payload alignment you want to guarantee.
// 64 is a good default for SIMD/GPU-friendly writes.
#ifndef ALIA_SCRATCH_CHUNK_ALIGN
#define ALIA_SCRATCH_CHUNK_ALIGN 64
#endif

struct alignas(ALIA_SCRATCH_CHUNK_ALIGN) scratch_chunk
{
    alia_scratch_chunk_allocation alloc;
    scratch_chunk* next = nullptr;
    std::size_t cap = 0; // payload capacity (bytes)
    std::size_t used = 0; // payload used (bytes)
    std::uint8_t*
    data()
    {
        return reinterpret_cast<std::uint8_t*>(this + 1);
    }
    const std::uint8_t*
    data() const
    {
        return reinterpret_cast<const std::uint8_t*>(this + 1);
    }
};

} // namespace

struct alia_scratch_frame_header
{
    scratch_chunk* mark_chunk = nullptr;
    std::size_t mark_off = 0;
    void* prev_frame = nullptr;
};

struct alia_scratch_arena
{
    // allocator
    alia_scratch_allocator allocator{};

    // chunk list
    scratch_chunk* first = nullptr;
    scratch_chunk* head = nullptr;

    // stats
    std::size_t total_bytes = 0; // sum of chunk caps
    std::size_t peak_used = 0; // coarse high-water mark
    std::uint32_t chunks_touched = 0; // max prefix count touched in pass

    // pass state
    alia_scratch_frame_header* pass_base = nullptr;
    void* current_frame = nullptr;
};

// HELPERS

namespace {

inline std::size_t
align_up(std::size_t v, std::size_t a)
{
    const std::size_t m = a - 1;
    return (v + m) & ~m;
}

inline void*
align_ptr(void* ptr, std::size_t a)
{
    return reinterpret_cast<void*>(
        (reinterpret_cast<std::uintptr_t>(ptr) + a - 1) & ~(a - 1));
}

// Compute an approximate "bytes used" = sum(full prior chunk caps) +
// head->used
inline std::size_t
approx_used_bytes(const alia_scratch_arena* A)
{
    if (!A->first)
        return 0;
    std::size_t used = 0;
    for (auto* c = A->first; c; c = c->next)
    {
        if (c == A->head)
        {
            used += c->used;
            break;
        }
        else
        {
            used += c->cap;
        }
    }
    return used;
}

inline void
ensure_head_exists(alia_scratch_arena* A)
{
    if (A->first)
        return;

    const std::size_t payload = 0; // TODO
    const std::size_t need
        = sizeof(scratch_chunk) + alignof(scratch_chunk) + payload;

    alia_scratch_chunk_allocation alloc
        = (A->allocator.alloc)(A->allocator.user, 0, need);
    auto* mem = static_cast<scratch_chunk*>(
        align_ptr(alloc.ptr, alignof(scratch_chunk)));
    mem->alloc = alloc;
    mem->next = nullptr;
    mem->cap = reinterpret_cast<std::uint8_t*>(alloc.ptr) + alloc.size
             - reinterpret_cast<std::uint8_t*>(mem) - sizeof(scratch_chunk);
    mem->used = 0;

    A->first = A->head = mem;
    A->total_bytes += mem->cap;
}

inline void
ensure_head_space(alia_scratch_arena* A, std::size_t need_bytes)
{
    ensure_head_exists(A);

    // If head has space, done.
    if (A->head->used + need_bytes <= A->head->cap)
        return;

    // If there is a next chunk already, reuse it.
    if (A->head->next)
    {
        A->head = A->head->next;
        A->head->used = 0;
        if (A->head->used + need_bytes <= A->head->cap)
            return;
        // else fall-through to append a larger chunk
    }

    const std::size_t bytes
        = sizeof(scratch_chunk) + alignof(scratch_chunk) + need_bytes;

    alia_scratch_chunk_allocation alloc
        = (A->allocator.alloc)(A->allocator.user, 0, bytes);
    auto* mem = static_cast<scratch_chunk*>(
        align_ptr(alloc.ptr, alignof(scratch_chunk)));
    mem->alloc = alloc;
    mem->next = nullptr;
    mem->cap = reinterpret_cast<std::uint8_t*>(alloc.ptr) + alloc.size
             - reinterpret_cast<std::uint8_t*>(mem) - sizeof(scratch_chunk);
    mem->used = 0;

    // Link after current head, even if head is mid-list due to a pop.
    mem->next = A->head->next;
    A->head->next = mem;
    A->head = mem;
    A->total_bytes += mem->cap;
}

inline void*
arena_alloc_raw(alia_scratch_arena* A, std::size_t bytes, std::size_t align)
{
    ensure_head_exists(A);

    // ensure space considering alignment slack
    const std::size_t worst = bytes + (align - 1);
    ensure_head_space(A, worst);

    std::size_t p = align_up(A->head->used, align);
    if (p + bytes > A->head->cap)
    {
        // Spill once to next/append, then align again
        ensure_head_space(A, bytes + (align - 1));
        p = align_up(A->head->used, align);
        assert(p + bytes <= A->head->cap);
    }

    void* out = A->head->data() + p;
    A->head->used = p + bytes;

    // stats
    const std::size_t used = approx_used_bytes(A);
    if (used > A->peak_used)
        A->peak_used = used;

    // chunks_touched: number of chunks up to head
    std::uint32_t count = 0;
    for (auto* c = A->first; c; c = c->next)
    {
        ++count;
        if (c == A->head)
            break;
    }
    if (count > A->chunks_touched)
        A->chunks_touched = count;

    return out;
}

inline void
free_tail_after(
    alia_scratch_arena* A,
    scratch_chunk* keep_tail_of_prefix,
    std::uint32_t keep_min_chunks)
{
    if (!A->first)
        return;

    // Ensure we keep at least keep_min_chunks from the front
    scratch_chunk* keep_tail = A->first;
    for (std::uint32_t k = 1; k < std::max<std::uint32_t>(keep_min_chunks, 1);
         ++k)
    {
        if (!keep_tail->next)
            break;
        keep_tail = keep_tail->next;
    }
    // If caller wants to keep beyond that, advance.
    if (keep_tail_of_prefix)
    {
        // Walk from first until we reach keep_tail_of_prefix
        auto* c = A->first;
        while (c && c != keep_tail_of_prefix)
        {
            keep_tail = c;
            c = c->next;
        }
        if (c)
            keep_tail = c;
    }

    scratch_chunk* to_free = keep_tail->next;
    keep_tail->next = nullptr;

    // If head pointed into the tail, move it back to keep_tail.
    for (auto* c = to_free; c; c = c->next)
    {
        if (A->head == c)
        {
            A->head = keep_tail;
            break;
        }
    }

    // Free tail
    while (to_free)
    {
        auto* nxt = to_free->next;
        A->total_bytes -= to_free->cap;
        (A->allocator.free)(A->allocator.user, to_free->alloc);
        to_free = nxt;
    }
}

} // namespace

extern "C" {

// OPAQUE TYPE SIZE/ALIGN (FOR CONSTRUCT)

size_t
alia_scratch_state_size(void)
{
    return sizeof(alia_scratch_arena);
}

size_t
alia_scratch_state_align(void)
{
    return alignof(alia_scratch_arena);
}

// CREATION AND DESTRUCTION (HEAP-OWNED)

alia_scratch_arena*
alia_scratch_create(const alia_scratch_allocator* allocator)
{
    void* mem = ::operator new(
        sizeof(alia_scratch_arena),
        std::align_val_t(alignof(alia_scratch_arena)));
    auto* A = reinterpret_cast<alia_scratch_arena*>(mem);
    // zero-initialize then construct config
    std::memset(A, 0, sizeof(*A));
    A->allocator = *allocator;

    return A;
}

void
alia_scratch_destroy(alia_scratch_arena* arena)
{
    if (!arena)
        return;

    // free all chunks
    auto* c = arena->first;
    while (c)
    {
        auto* nxt = c->next;
        (arena->allocator.free)(arena->allocator.user, c->alloc);
        c = nxt;
    }

    ::operator delete(arena, std::align_val_t(alignof(alia_scratch_arena)));
}

// CONSTRUCTION AND DESTRUCTION (CALLER-SUPPLIED STORAGE)

alia_scratch_arena*
alia_scratch_construct(void* mem, const alia_scratch_allocator* allocator)
{
    assert(mem && "alia_scratch_construct: mem must not be null");
    auto* A = new (mem) alia_scratch_arena{};
    A->allocator = *allocator;
    return A;
}

void
alia_scratch_destruct(alia_scratch_arena* arena)
{
    if (!arena)
        return;

    // free all chunks
    auto* c = arena->first;
    while (c)
    {
        auto* nxt = c->next;
        (arena->allocator.free)(arena->allocator.user, c->alloc);
        c = nxt;
    }

    // placement "destruct": explicitly call destructor (trivial here)
    arena->~alia_scratch_arena();
}

// PASS LIFECYCLE

void
alia_scratch_reset(alia_scratch_arena* arena)
{
    arena->current_frame = nullptr;
    arena->pass_base = nullptr;

    // Reset used on kept chunks
    for (auto* k = arena->first; k; k = k->next)
        k->used = 0;

    // Reset stats for next pass
    arena->peak_used = 0;
    arena->chunks_touched = 0;
}

void
alia_scratch_trim_chunks(alia_scratch_arena* arena, uint32_t n)
{
    free_tail_after(arena, nullptr, n);
}

// ALLOCATION

void*
alia_scratch_alloc(alia_scratch_arena* arena, size_t bytes, size_t align)
{
    assert(arena && "alloc: arena null");
    if (align == 0)
        align = alignof(std::max_align_t);
    return arena_alloc_raw(arena, bytes, align);
}

// FRAMES

alia_scratch_marker
alia_scratch_mark(alia_scratch_arena* arena)
{
    assert(arena && "push_frame: arena null");
    auto* h = static_cast<alia_scratch_frame_header*>(arena_alloc_raw(
        arena,
        sizeof(alia_scratch_frame_header),
        alignof(alia_scratch_frame_header)));
    h->mark_chunk = arena->head;
    h->mark_off = arena->head ? arena->head->used : 0;
    h->prev_frame = arena->current_frame;
    arena->current_frame = reinterpret_cast<void*>(h + 1);
    return h;
}

void
alia_scratch_rewind(alia_scratch_arena* arena, alia_scratch_marker m)
{
    arena->head = m->mark_chunk;
    if (arena->head)
        arena->head->used = m->mark_off;
    arena->current_frame = m->prev_frame;
}

// STATISTICS

alia_scratch_stats
alia_scratch_get_stats(const alia_scratch_arena* arena)
{
    alia_scratch_stats s{};
    if (!arena)
        return s;
    s.committed_bytes = arena->total_bytes;
    s.peak_used_bytes = arena->peak_used;
    s.max_chunks_touched = arena->chunks_touched;
    return s;
}

} // extern "C"
