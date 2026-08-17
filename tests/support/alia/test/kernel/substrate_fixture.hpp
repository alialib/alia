#pragma once

// C++ substrate test fixture. Not part of the public alia API.

#include <alia/abi/base/arena.h>
#include <alia/abi/base/stack.h>
#include <alia/abi/context.h>

#include "substrate_fixture.h"

#include <doctest/doctest.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace alia {
namespace test {

inline void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    size_t rounded = (size + align - 1u) / align * align;
    return aligned_alloc(align, rounded);
#endif
}

inline void
aligned_free_portable(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

inline void*
test_block_alloc(void* user_data, size_t size, size_t alignment)
{
    (void) user_data;
    if (alignment <= sizeof(void*))
        return malloc(size);
    return aligned_alloc_portable(alignment, size);
}

inline void
test_block_free(void* user_data, void* ptr, size_t size, size_t alignment)
{
    (void) user_data;
    (void) size;
    if (alignment <= sizeof(void*))
        free(ptr);
    else
        aligned_free_portable(ptr);
}

struct substrate_fixture
{
    alia_test_substrate_fixture* fixture = nullptr;
    alia_stack* stack = nullptr;
    void* stack_obj_storage = nullptr;
    void* stack_buffer = nullptr;
    alia_context ctx{};

    substrate_fixture()
    {
        init();
    }

    ~substrate_fixture()
    {
        destroy();
    }

    substrate_fixture(substrate_fixture const&) = delete;
    substrate_fixture&
    operator=(substrate_fixture const&)
        = delete;

    void
    reset_traversal()
    {
        alia_test_substrate_fixture_reset_traversal(fixture, true);
        alia_stack_reset(stack);
    }

    alia_substrate_anchor*
    root_anchor()
    {
        return alia_test_substrate_fixture_root_anchor(fixture);
    }

    void
    cleanup_root_block()
    {
        alia_test_substrate_fixture_cleanup_root_block(fixture);
    }

    void
    advance_frame()
    {
        alia_test_substrate_fixture_advance_frame(fixture);
    }

 private:
    void
    init()
    {
        alia_general_allocator allocator
            = {.alloc = test_block_alloc,
               .free = test_block_free,
               .user_data = nullptr};
        fixture = alia_test_substrate_fixture_create(allocator);
        REQUIRE(fixture);

        alia_struct_spec stack_spec = alia_stack_object_spec();
        stack_obj_storage
            = aligned_alloc_portable(stack_spec.align, stack_spec.size);
        REQUIRE(stack_obj_storage);
        stack_buffer = aligned_alloc_portable(ALIA_MAX_ALIGN, 64u * 1024u);
        REQUIRE(stack_buffer);

        stack = alia_stack_init(stack_obj_storage, stack_buffer, 64u * 1024u);
        REQUIRE(stack);
        alia_stack_reset(stack);

        ctx = {};
        ctx.substrate = alia_test_substrate_fixture_traversal(fixture);
        ctx.stack = stack;
    }

    void
    destroy()
    {
        if (stack)
            alia_stack_destroy(stack);
        stack = nullptr;

        if (stack_buffer)
            aligned_free_portable(stack_buffer);
        stack_buffer = nullptr;

        if (stack_obj_storage)
            aligned_free_portable(stack_obj_storage);
        stack_obj_storage = nullptr;

        if (fixture)
            alia_test_substrate_fixture_destroy(fixture);
        fixture = nullptr;
    }
};

} // namespace test
} // namespace alia
