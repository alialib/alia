#include <alia/abi/arena.h>

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

size_t
test_grow_fn(
    void* user, void* base, size_t existing_capacity, size_t bytes_requested)
{
    realloc(base, existing_capacity * 2);
    return existing_capacity * 2;
}

void
test_free(void* user, void* base, size_t capacity)
{
    free(base);
}

struct test_rig
{
    void* storage = nullptr;
    alia_arena* arena = nullptr;

    void
    init(size_t initial_capacity)
    {
        auto const spec = alia_arena_struct_spec();
        this->storage
            = ::operator new(spec.size, std::align_val_t{spec.align});
        this->arena = alia_arena_construct(
            this->storage,
            malloc(initial_capacity),
            initial_capacity,
            alia_arena_controller{
                .user = nullptr, .grow = test_grow_fn, .free = test_free});
    }

    void
    destroy()
    {
        alia_arena_destruct(this->arena);
        this->arena = nullptr;
        ::operator delete(
            this->storage, std::align_val_t{alia_arena_struct_spec().align});
        this->storage = nullptr;
    }

    alia_arena_view*
    view()
    {
        return alia_arena_get_view(this->arena);
    }
};

} // namespace

TEST_CASE("construct_destruct")
{
    test_rig rig;
    rig.init(1024);

    REQUIRE(rig.arena != nullptr);

    auto stats = alia_arena_get_stats(rig.arena);
    CHECK(stats.current_usage == 0);
    CHECK(stats.peak_usage == 0);

    rig.destroy();
}

TEST_CASE("allocation_alignment")
{
    test_rig rig;
    rig.init(4096);

    // Allocate a bunch with varying alignments and verify alignment.
    for (size_t i = 0; i < 4; ++i)
    {
        size_t size = 64 << i;
        size_t align = ALIA_MIN_ARENA_ALIGN << i;
        INFO(align);
        alia_offset p = alia_arena_alloc_aligned(rig.view(), size, align);
        CHECK((p % align) == 0);
        memset(alia_arena_ptr(rig.view(), p), 0, size); // Touch the memory.
    }

    rig.destroy();
}

TEST_CASE("allocation_growth")
{
    test_rig rig;
    rig.init(1024);

    alia_offset p = alia_arena_alloc(rig.view(), 1600);
    CHECK(p == 0);
    CHECK(rig.view()->capacity == 2048);
    rig.destroy();
}

TEST_CASE("mark_jump_reset")
{
    test_rig rig;
    rig.init(1024);

    // Do a base allocation.
    alia_offset a0 = alia_arena_alloc(rig.view(), 64);

    // Set a mark and allocate more data.
    alia_arena_marker m1 = alia_arena_mark(rig.view());
    alia_offset a1 = alia_arena_alloc(rig.view(), 80);
    // Mark again and allocate even more.
    alia_arena_marker m2 = alia_arena_mark(rig.view());
    alia_offset a2 = alia_arena_alloc(rig.view(), 96);

    // Jump back to `m2`. The next allocation should reuse the address of `a2`.
    alia_arena_jump(rig.view(), m2);
    alia_offset b2 = alia_arena_alloc(rig.view(), 96);
    CHECK(b2 == a2);

    // Jump back to `m1`. The next allocation should reuse the address of `a1`.
    alia_arena_jump(rig.view(), m1);
    alia_offset b1 = alia_arena_alloc(rig.view(), 80);
    CHECK(b1 == a1);

    // Confirm that the peak usage is correct.
    auto stats = alia_arena_get_stats(rig.arena);
    CHECK(stats.peak_usage == 240);

    // Reset the arena and confirm that the peak usage is correct.
    alia_arena_reset(rig.view());
    stats = alia_arena_get_stats(rig.arena);
    CHECK(stats.peak_usage == 240);

    rig.destroy();
}

TEST_CASE("stats")
{
    test_rig rig;
    rig.init(256);

    (void) alia_arena_alloc(rig.view(), 112);
    (void) alia_arena_alloc(rig.view(), 203);

    {
        auto stats = alia_arena_get_stats(rig.arena);
        CHECK(stats.current_usage == 315);
        CHECK(stats.peak_usage == 315);
    }

    alia_arena_reset(rig.view());

    {
        auto stats = alia_arena_get_stats(rig.arena);
        CHECK(stats.current_usage == 0);
        CHECK(stats.peak_usage == 315);
    }

    rig.destroy();
}
