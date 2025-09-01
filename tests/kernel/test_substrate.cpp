#include <alia/kernel/substrate.hpp>

#include <alia/kernel/infinite_arena.hpp>
#include <alia/scratch.h>

#include <doctest/doctest.h>

#include <cstdio>
#include <cstdlib>
#include <new>

namespace {

alia_scratch_chunk_allocation
infinite_alloc(
    void* user, size_t existing_capacity, size_t minimum_needed_bytes)
{
    auto* a = static_cast<alia::infinite_arena*>(user);
    return {a->peek(), alia::infinite_arena::INFINITE_CAPACITY};
}
void
infinite_free(void* user, alia_scratch_chunk_allocation chunk)
{
}

struct scratch_rig
{
    alia::infinite_arena allocator;
    void* storage = nullptr;
    alia_scratch_arena* scratch = nullptr;

    scratch_rig()
    {
        this->allocator.initialize();

        alia_scratch_allocator allocator;
        allocator.user = &this->allocator;
        allocator.alloc = &infinite_alloc;
        allocator.free = &infinite_free;

        auto const spec = alia_scratch_struct_spec();
        this->storage
            = ::operator new(spec.size, std::align_val_t{spec.align});
        this->scratch = alia_scratch_construct(storage, &allocator);
    }

    ~scratch_rig()
    {
        this->destroy();
    }

    void
    destroy()
    {
        alia_scratch_destruct(this->scratch);
        this->scratch = nullptr;
        ::operator delete(
            this->storage, std::align_val_t{alia_scratch_struct_spec().align});
        this->storage = nullptr;
    }
};

void*
block_alloc(void* user, size_t size, size_t alignment)
{
    return malloc(size);
}
void
block_free(void* user, void* ptr)
{
    free(ptr);
}

} // namespace

TEST_CASE("construct_destruct")
{
    scratch_rig rig;

    alia::substrate_system system;

    alia::substrate_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = block_alloc;
    allocator.free = block_free;

    construct_substrate_system(system, allocator);

    destruct_substrate_system(system);
}

TEST_CASE("basic_block")
{
    scratch_rig rig;

    alia::substrate_system system;

    alia::substrate_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = block_alloc;
    allocator.free = block_free;

    construct_substrate_system(system, allocator);

    alia::substrate_traversal traversal = {};
    substrate_begin_traversal(traversal, system, rig.scratch);

    alia::substrate_block_spec spec;
    spec.size = 1024;
    spec.alignment = 16;

    alia::substrate_block_scope scope;
    alia::substrate_block block;

    substrate_begin_block(traversal, scope, block, spec);

    auto use1 = substrate_use_memory(traversal, 256, 16);
    CHECK(use1.ptr != nullptr);
    CHECK(use1.code == alia::substrate_usage_result::NORMAL);
    auto use2 = substrate_use_memory(traversal, 256, 16);
    CHECK(use2.ptr != nullptr);
    CHECK(
        reinterpret_cast<std::uint8_t*>(use2.ptr)
            - reinterpret_cast<std::uint8_t*>(use1.ptr)
        == 256);
    CHECK(use2.code == alia::substrate_usage_result::NORMAL);
    auto use3 = substrate_use_memory(traversal, 256, 16);
    CHECK(use3.ptr != nullptr);
    CHECK(
        reinterpret_cast<std::uint8_t*>(use3.ptr)
            - reinterpret_cast<std::uint8_t*>(use2.ptr)
        == 256);
    CHECK(use3.code == alia::substrate_usage_result::NORMAL);

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    destruct_substrate_system(system);
}

TEST_CASE("basic_block_discovery")
{
    scratch_rig rig;

    alia::substrate_system system;

    alia::substrate_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = block_alloc;
    allocator.free = block_free;

    construct_substrate_system(system, allocator);

    alia::substrate_traversal traversal = {};
    substrate_begin_traversal(traversal, system, rig.scratch);

    alia::substrate_block_scope scope;
    alia::substrate_block block;

    substrate_begin_block(
        traversal, scope, block, alia::substrate_block_spec{});

    {
        auto use1 = substrate_use_memory(traversal, 256, 16);
        CHECK(use1.ptr != nullptr);
        CHECK(use1.code == alia::substrate_usage_result::DISCOVERY);
        auto use2 = substrate_use_memory(traversal, 256, 16);
        CHECK(use2.ptr != nullptr);
        CHECK(use2.ptr != use1.ptr);
        CHECK(use2.code == alia::substrate_usage_result::DISCOVERY);
        auto use3 = substrate_use_memory(traversal, 256, 16);
        CHECK(use3.ptr != nullptr);
        CHECK(use3.ptr != use2.ptr);
        CHECK(use3.code == alia::substrate_usage_result::DISCOVERY);
    }

    auto spec = substrate_get_block_spec(traversal);
    CHECK(spec.size == 768);
    CHECK(spec.alignment == 16);

    substrate_end_block(traversal, scope);

    substrate_begin_block(traversal, scope, block, spec);

    {
        auto use1 = substrate_use_memory(traversal, 256, 16);
        CHECK(use1.ptr != nullptr);
        CHECK(use1.code == alia::substrate_usage_result::NORMAL);
        auto use2 = substrate_use_memory(traversal, 256, 16);
        CHECK(use2.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use2.ptr)
                - reinterpret_cast<std::uint8_t*>(use1.ptr)
            == 256);
        CHECK(use2.code == alia::substrate_usage_result::NORMAL);
        auto use3 = substrate_use_memory(traversal, 256, 16);
        CHECK(use3.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use3.ptr)
                - reinterpret_cast<std::uint8_t*>(use2.ptr)
            == 256);
        CHECK(use3.code == alia::substrate_usage_result::NORMAL);
    }

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    destruct_substrate_system(system);
}
