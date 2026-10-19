#pragma once

// C++ effect-log test fixture. Not part of the public alia API.

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/effect.h>
#include <alia/context.hpp>

#include <doctest/doctest.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace alia { namespace test {

inline void*
effect_aligned_alloc(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    size_t rounded = (size + align - 1u) / align * align;
    return aligned_alloc(align, rounded);
#endif
}

inline void
effect_aligned_free(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

// `effect_fixture` provides a scratch arena and effect log wired into a
// `kernel_context` for tests.
struct effect_fixture
{
    // scratch arena object storage
    void* storage = nullptr;
    // scratch arena buffer
    void* buffer = nullptr;
    // scratch arena
    alia_arena* arena = nullptr;
    // kernel context with scratch and effects wired
    kernel_context ctx{};

    effect_fixture()
    {
        alia_struct_spec const spec = alia_arena_object_spec();
        storage = effect_aligned_alloc(spec.align, spec.size);
        REQUIRE(storage);
        buffer = effect_aligned_alloc(ALIA_MAX_ALIGN, 64u * 1024u);
        REQUIRE(buffer);
        arena = alia_arena_init(
            storage, buffer, 64u * 1024u, alia_arena_no_controller());
        REQUIRE(arena);
        alia_bump_allocator_init(&ctx.scratch, arena);
    }

    ~effect_fixture()
    {
        if (arena)
            alia_arena_destroy(arena);
        arena = nullptr;
        if (buffer)
            effect_aligned_free(buffer);
        buffer = nullptr;
        if (storage)
            effect_aligned_free(storage);
        storage = nullptr;
    }

    effect_fixture(effect_fixture const&) = delete;
    effect_fixture&
    operator=(effect_fixture const&) = delete;

    // Run posted effects and reset the scratch arena for the next posting
    // sequence.
    void
    run()
    {
        alia_run_effects(&ctx);
        alia_arena_reset(&ctx.scratch);
        ctx.effects = {};
    }
};

}} // namespace alia::test
