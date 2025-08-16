#include <doctest/doctest.h>

#include <alia/base/arena.hpp>
#include <cstring> // for std::memset

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <cstdlib>
#endif

using namespace alia;

static void*
default_alloc(void*, size_t size, size_t alignment)
{
    void* ptr = nullptr;
#ifdef _MSC_VER
    ptr = _aligned_malloc(size, alignment);
#else
    posix_memalign(&ptr, alignment, size);
#endif
    return ptr;
}

static void
default_dealloc(void*, void* ptr)
{
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

TEST_CASE("Arena allocates memory")
{
    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* arena = create_arena(alloc, 1024);

    void* p1 = arena_alloc(arena, 64);
    void* p2 = arena_alloc(arena, 64);

    REQUIRE(p1 != nullptr);
    REQUIRE(p2 != nullptr);
    CHECK(p1 != p2);

    destroy_arena(arena);
}

TEST_CASE("Arena respects alignment")
{
    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* arena = create_arena(alloc, 1024);

    void* p = arena_alloc(arena, 64, 64);
    CHECK(reinterpret_cast<uintptr_t>(p) % 64 == 0);

    destroy_arena(arena);
}

TEST_CASE("Arena invokes destructors")
{
    struct DtorTracker
    {
        int* counter;
    };

    int destroyed = 0;

    auto dtor_fn = [](void* ptr) {
        auto* t = static_cast<DtorTracker*>(ptr);
        (*t->counter)++;
    };

    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* arena = create_arena(alloc, 1024);

    auto* tracker
        = static_cast<DtorTracker*>(arena_alloc(arena, sizeof(DtorTracker)));
    tracker->counter = &destroyed;

    arena_register_destructor(arena, tracker, dtor_fn);

    CHECK(destroyed == 0);
    reset_arena(arena);
    CHECK(destroyed == 1);

    destroy_arena(arena); // no double-destruction
    CHECK(destroyed == 1);
}
