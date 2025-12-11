#include <alia/scratch.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {

struct alloc_controller
{
    size_t chunk_size = 256;
    size_t alloc_calls = 0;
    size_t free_calls = 0;
};
alia_scratch_chunk_allocation
test_alloc(void* user, size_t existing_capacity, size_t minimum_needed_bytes)
{
    auto* c = static_cast<alloc_controller*>(user);
    ++c->alloc_calls;
    size_t size = std::max(c->chunk_size, minimum_needed_bytes);
    return {malloc(size), size};
}
void
test_free(void* user, alia_scratch_chunk_allocation chunk)
{
    auto* c = static_cast<alloc_controller*>(user);
    ++c->free_calls;
    free(chunk.ptr);
}

struct test_rig
{
    alloc_controller allocator;
    void* storage = nullptr;
    alia_scratch_arena* arena = nullptr;

    void
    init(size_t chunk_size)
    {
        this->allocator.chunk_size = chunk_size;

        alia_scratch_allocator allocator;
        allocator.user = &this->allocator;
        allocator.alloc = &test_alloc;
        allocator.free = &test_free;

        auto const spec = alia_scratch_struct_spec();
        this->storage
            = ::operator new(spec.size, std::align_val_t{spec.align});
        this->arena = alia_scratch_construct(storage, allocator);
    }

    void
    destroy()
    {
        alia_scratch_destruct(this->arena);
        this->arena = nullptr;
        ::operator delete(
            this->storage, std::align_val_t{alia_scratch_struct_spec().align});
        this->storage = nullptr;
    }
};

} // namespace

TEST_CASE("construct_destruct")
{
    test_rig rig;
    rig.init(1024);

    REQUIRE(rig.arena != nullptr);

    auto stats = alia_scratch_get_stats(rig.arena);
    CHECK(stats.current.bytes_allocated == 0);
    CHECK(stats.current.bytes_used == 0);
    CHECK(stats.current.chunk_count == 0);
    CHECK(stats.peak.bytes_allocated == 0);
    CHECK(stats.peak.bytes_used == 0);
    CHECK(stats.peak.chunk_count == 0);

    rig.destroy();
}

TEST_CASE("allocation_alignment_and_growth")
{
    test_rig rig;
    rig.init(512);

    // Allocate a bunch with varying alignments and verify alignment.
    for (size_t i = 1; i <= 64; ++i)
    {
        size_t align = 1u << (i % 6); // 1,2,4,8,16,32
        INFO(align);
        void* p = alia_scratch_alloc(rig.arena, 37 + i, align);
        REQUIRE(p != nullptr);
        CHECK((reinterpret_cast<std::uintptr_t>(p) % align) == 0);
        memset(p, 0, 37 + i); // Touch the memory.
    }

    // Force a spill to a new chunk.
    size_t before_allocs = rig.allocator.alloc_calls;
    (void) alia_scratch_alloc(rig.arena, 1024, 64);
    CHECK(rig.allocator.alloc_calls > before_allocs); // new chunk appended

    // Stats should reflect multi-chunk touch.
    auto stats = alia_scratch_get_stats(rig.arena);
    CHECK(stats.peak.chunk_count >= 2);
    CHECK(stats.peak.bytes_used >= 1024);

    rig.destroy();
}

TEST_CASE("frames_push_pop_and_address_reuse")
{
    test_rig rig;
    rig.init(1024);

    // Do a base allocation.
    void* a0 = alia_scratch_alloc(rig.arena, 64, 8);

    // Set a mark and allocate more data.
    alia_scratch_marker m1 = alia_scratch_mark(rig.arena);
    void* a1 = alia_scratch_alloc(rig.arena, 80, 8);
    // Mark again and allocate even more.
    alia_scratch_marker m2 = alia_scratch_mark(rig.arena);
    void* a2 = alia_scratch_alloc(rig.arena, 96, 8);

    // Pop back to `m2`. The next allocation should reuse the address of `a2`.
    alia_scratch_jump(rig.arena, m2);
    void* b2 = alia_scratch_alloc(rig.arena, 96, 8);
    CHECK(b2 == a2);

    // Pop back to `m1`. The next allocation should reuse the address of `a1`.
    alia_scratch_jump(rig.arena, m1);
    void* b1 = alia_scratch_alloc(rig.arena, 80, 8);
    CHECK(b1 == a1);

    // And check that `a0` hasn't been stepped on.
    CHECK(
        reinterpret_cast<uintptr_t>(b1) - reinterpret_cast<uintptr_t>(a0)
        >= 64);

    rig.destroy();
}

TEST_CASE("cross_chunk_marks_and_chunk_reuse")
{
    test_rig rig;
    rig.init(256);

    // Create a mark, and then fill most of the first chunk..
    alia_scratch_marker m1 = alia_scratch_mark(rig.arena);
    std::size_t bytes_used_after_m1
        = alia_scratch_get_stats(rig.arena).current.bytes_used;
    void* in_first = alia_scratch_alloc(rig.arena, 203, 1);
    CHECK(
        alia_scratch_get_stats(rig.arena).current.bytes_used
        == bytes_used_after_m1 + 203);

    // This allocation should go into a second chunk.
    alia_scratch_marker m2 = alia_scratch_mark(rig.arena);
    std::size_t bytes_used_after_m2
        = alia_scratch_get_stats(rig.arena).current.bytes_used;
    void* in_second = alia_scratch_alloc(rig.arena, 112, 1);
    CHECK(
        alia_scratch_get_stats(rig.arena).current.bytes_used
        == bytes_used_after_m2 + 112);

    // Rewind to `m2` to check that stats are updated.
    alia_scratch_jump(rig.arena, m2);
    CHECK(
        alia_scratch_get_stats(rig.arena).current.bytes_used
        == bytes_used_after_m2);

    // Rewind back to `m1`, which is in the first chunk.
    alia_scratch_jump(rig.arena, m1);
    CHECK(
        alia_scratch_get_stats(rig.arena).current.bytes_used
        == bytes_used_after_m1);

    // Repeat the allocations. They should both produce the same addresses if
    // the chunks are being reused.
    REQUIRE(alia_scratch_alloc(rig.arena, 203, 1) == in_first);
    (void) alia_scratch_mark(rig.arena);
    REQUIRE(alia_scratch_alloc(rig.arena, 112, 1) == in_second);

    rig.destroy();
}

TEST_CASE("trim_frees_extra_chunks")
{
    test_rig rig;
    rig.init(256);

    // Allocate multiple chunks.
    (void) alia_scratch_alloc(rig.arena, 203, 1); // in chunk 0
    (void) alia_scratch_alloc(rig.arena, 203, 1); // in chunk 1
    (void) alia_scratch_alloc(rig.arena, 203, 1); // in chunk 2

    {
        auto stats = alia_scratch_get_stats(rig.arena);
        CHECK(stats.current.bytes_allocated == 768);
        CHECK(stats.current.chunk_count == 3);
    }

    alia_scratch_reset(rig.arena);
    alia_scratch_trim_chunks(rig.arena, 1);

    {
        auto stats = alia_scratch_get_stats(rig.arena);
        CHECK(stats.current.bytes_allocated == 256);
        CHECK(stats.current.chunk_count == 1);
    }

    CHECK(rig.allocator.free_calls == 2);

    rig.destroy();
}

TEST_CASE("stats_peak_used_and_chunks_touched")
{
    test_rig rig;
    rig.init(256);

    (void) alia_scratch_alloc(rig.arena, 112, 8);
    (void) alia_scratch_alloc(rig.arena, 203, 8);

    {
        auto stats = alia_scratch_get_stats(rig.arena);
        CHECK(stats.current.bytes_allocated == 512);
        CHECK(stats.current.bytes_used == 315);
        CHECK(stats.current.chunk_count == 2);
        CHECK(stats.peak.bytes_allocated == 512);
        CHECK(stats.peak.bytes_used == 315);
        CHECK(stats.peak.chunk_count == 2);
    }

    alia_scratch_reset(rig.arena);

    {
        auto stats = alia_scratch_get_stats(rig.arena);
        CHECK(stats.current.bytes_allocated == 512);
        CHECK(stats.current.bytes_used == 0);
        CHECK(stats.current.chunk_count == 2);
        CHECK(stats.peak.bytes_allocated == 512);
        CHECK(stats.peak.bytes_used == 315);
        CHECK(stats.peak.chunk_count == 2);
    }

    rig.destroy();
}
