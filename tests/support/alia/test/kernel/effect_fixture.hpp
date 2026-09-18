#pragma once

// C++ effect-log test fixture. Not part of the public alia API.

#include <alia/abi/base/arena.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/effect.h>

#include <doctest/doctest.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace alia {
namespace test {

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

// `effect_fixture` provides a scratch arena and effect log wired into an
// `alia_context` for tests.
struct effect_fixture
{
    // scratch arena object storage
    void* storage = nullptr;
    // scratch arena buffer
    void* buffer = nullptr;
    // scratch arena
    alia_arena* arena = nullptr;
    // bump allocator over the scratch arena
    alia_bump_allocator scratch{};
    // pass-local effect log
    alia_effect_log effects{};
    // context with scratch and effects wired
    alia_context ctx{};

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
        alia_bump_allocator_init(&scratch, arena);
        ctx.scratch = &scratch;
        ctx.effects = &effects;
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
    operator=(effect_fixture const&)
        = delete;

    // Run posted effects and reset the scratch arena for the next posting
    // sequence.
    void
    run()
    {
        alia_run_effects(&ctx);
        alia_arena_reset(&scratch);
        effects = {};
        ctx.effects = &effects;
    }
};

} // namespace test
} // namespace alia
