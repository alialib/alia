#include <alia/scratch.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>

// simple aligned alloc hooks so we can count chunk allocs/frees
struct alloc_controller
{
    size_t chunk_size = 256;
    size_t alloc_calls = 0;
    size_t free_calls = 0;
};
static alia_scratch_chunk_allocation
test_alloc(void* user, size_t existing_capacity, size_t minimum_needed_bytes)
{
    auto* c = static_cast<alloc_controller*>(user);
    ++c->alloc_calls;
    size_t size = std::max(c->chunk_size, minimum_needed_bytes);
    return {malloc(size), size};
}
static void
test_free(void* user, alia_scratch_chunk_allocation chunk)
{
    auto* c = static_cast<alloc_controller*>(user);
    ++c->free_calls;
    free(chunk.ptr);
}

TEST_CASE("construction_create_and_inplace")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 64 * 1024;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;

    // heap-owned scratch arena
    alia_scratch_arena* A = alia_scratch_create(&allocator);
    REQUIRE(A != nullptr);

    // We should be able to begin/end a pass without allocations beyond the
    // first chunk.
    auto stats0 = alia_scratch_get_stats(A);
    CHECK(stats0.committed_bytes <= ctrl.chunk_size);
    alia_scratch_reset(A);

    alia_scratch_destroy(A);

    // in-place
    const size_t sz = alia_scratch_state_size();
    const size_t al = alia_scratch_state_align();
    void* mem = ::operator new(sz, std::align_val_t(al));
    A = alia_scratch_construct(mem, &allocator);
    REQUIRE(A != nullptr);

    alia_scratch_reset(A);
    alia_scratch_destruct(A);
    ::operator delete(mem, std::align_val_t(al));
}

TEST_CASE("allocation_alignment_and_growth")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 512;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;

    alia_scratch_arena* A = alia_scratch_create(&allocator);

    // Allocate a bunch with varying alignments; verify alignment.
    for (size_t i = 1; i <= 64; ++i)
    {
        size_t align = 1u << (i % 6); // 1,2,4,8,16,32
        void* p = alia_scratch_alloc(A, 37 + i, align);
        REQUIRE(p != nullptr);
        CHECK((reinterpret_cast<std::uintptr_t>(p) % align) == 0);
    }

    // Force a spill to a new chunk.
    size_t before_allocs = ctrl.alloc_calls;
    (void) alia_scratch_alloc(A, 1024, 64);
    CHECK(ctrl.alloc_calls >= before_allocs + 1); // new chunk appended

    // Stats should reflect multi-chunk touch.
    auto stats = alia_scratch_get_stats(A);
    CHECK(stats.max_chunks_touched >= 2);
    CHECK(stats.peak_used_bytes >= 1024);

    alia_scratch_destroy(A);
}

TEST_CASE("frames_push_pop_and_address_reuse")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 256;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;

    alia_scratch_arena* A = alia_scratch_create(&allocator);

    // base allocation
    void* a0 = alia_scratch_alloc(A, 64, alignof(std::max_align_t));

    // Push a frame and allocate.
    alia_scratch_marker m1 = alia_scratch_mark(A);
    void* a1 = alia_scratch_alloc(A, 80, alignof(std::max_align_t));
    // Push nested
    alia_scratch_marker m2 = alia_scratch_mark(A);
    void* a2 = alia_scratch_alloc(A, 96, alignof(std::max_align_t));

    // Pop inner; next alloc should reuse a2 address (same size/align) if no
    // spill
    alia_scratch_rewind(A, m2);
    void* b2 = alia_scratch_alloc(A, 96, alignof(std::max_align_t));
    CHECK(b2 == a2);

    // Pop outer; next alloc should reuse a1 address
    alia_scratch_rewind(A, m1);
    void* b1 = alia_scratch_alloc(A, 80, alignof(std::max_align_t));
    CHECK(b1 == a1);

    // Base still valid for overwrite after frames popped
    void* b0 = alia_scratch_alloc(A, 64, alignof(std::max_align_t));
    (void) a0;
    CHECK(b0 != nullptr);

    alia_scratch_destroy(A);
}

TEST_CASE("cross_chunk_frames_and_reuse_next_chunk")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 128;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;

    alia_scratch_arena* A = alia_scratch_create(&allocator);

    // Push a frame, then allocate to spill into a second chunk
    alia_scratch_marker m = alia_scratch_mark(A);

    // Fill (or nearly fill) first chunk
    (void) alia_scratch_alloc(A, 120, 8);

    // First allocation in second chunk (capture pointer)
    void* first_in_second = alia_scratch_alloc(A, 32, 8);
    REQUIRE(first_in_second != nullptr);

    // Pop back to frame start (rewind into first chunk)
    alia_scratch_rewind(A, m);

    // Reserve a big allocation to ensure we spill to "next" chunk and reuse it
    (void) alia_scratch_alloc(A, 120, 8);
    void* first_again = alia_scratch_alloc(A, 32, 8);
    CHECK(first_again == first_in_second); // we reused the same next chunk

    alia_scratch_destroy(A);
}

TEST_CASE("trim_frees_extra_chunks")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 256;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;

    alia_scratch_arena* A = alia_scratch_create(&allocator);

    // Build multiple chunks in a pass
    (void) alia_scratch_alloc(A, 226, 8); // in chunk 0
    (void) alia_scratch_alloc(A, 226, 8); // moves to chunk 1
    (void) alia_scratch_alloc(A, 226, 8); // moves to chunk 2

    auto stats_before = alia_scratch_get_stats(A);
    CHECK(stats_before.committed_bytes >= 256 * 2);

    alia_scratch_reset(A);
    alia_scratch_trim_chunks(A, 1);

    auto stats_after = alia_scratch_get_stats(A);
    CHECK(stats_after.committed_bytes <= stats_before.committed_bytes);
    CHECK(ctrl.free_calls >= 1);

    // Next pass starts fresh with kept chunk(s)
    (void) alia_scratch_alloc(A, 64, 8);

    alia_scratch_destroy(A);
}

TEST_CASE("stats_peak_used_and_chunks_touched")
{
    alloc_controller ctrl{};
    ctrl.chunk_size = 256;
    alia_scratch_allocator allocator{};
    allocator.user = &ctrl;
    allocator.alloc = &test_alloc;
    allocator.free = &test_free;
    alia_scratch_arena* A = alia_scratch_create(&allocator);

    (void) alia_scratch_alloc(A, 128, 8);
    (void) alia_scratch_alloc(A, 200, 8); // force spill
    auto s = alia_scratch_get_stats(A);
    CHECK(s.peak_used_bytes >= 200);
    CHECK(s.max_chunks_touched >= 2);
    alia_scratch_reset(A);

    alia_scratch_destroy(A);
}
