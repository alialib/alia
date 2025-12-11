#include <alia/substrate.hpp>

#include <alia/arenas.hpp>
#include <alia/scratch.h>

#include <doctest/doctest.h>

#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

namespace {

struct scratch_rig
{
    alia::lazy_commit_arena_allocator allocator;
    void* storage = nullptr;
    alia_scratch_arena* scratch = nullptr;

    scratch_rig()
    {
        alia_scratch_allocator allocator
            = make_lazy_commit_arena_allocator(&this->allocator);

        auto const spec = alia_scratch_struct_spec();
        this->storage
            = ::operator new(spec.size, std::align_val_t{spec.align});
        this->scratch = alia_scratch_construct(storage, allocator);
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
    CHECK(use1.mode == alia::substrate_block_traversal_mode::INIT);
    auto use2 = substrate_use_memory(traversal, 256, 16);
    CHECK(use2.ptr != nullptr);
    CHECK(
        reinterpret_cast<std::uint8_t*>(use2.ptr)
            - reinterpret_cast<std::uint8_t*>(use1.ptr)
        == 256);
    CHECK(use2.mode == alia::substrate_block_traversal_mode::INIT);
    auto use3 = substrate_use_memory(traversal, 256, 16);
    CHECK(use3.ptr != nullptr);
    CHECK(
        reinterpret_cast<std::uint8_t*>(use3.ptr)
            - reinterpret_cast<std::uint8_t*>(use2.ptr)
        == 256);
    CHECK(use3.mode == alia::substrate_block_traversal_mode::INIT);

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    destruct_substrate_block(system, block);
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
        CHECK(use1.mode == alia::substrate_block_traversal_mode::DISCOVERY);
        auto use2 = substrate_use_memory(traversal, 256, 16);
        CHECK(use2.ptr != nullptr);
        CHECK(use2.ptr != use1.ptr);
        CHECK(use2.mode == alia::substrate_block_traversal_mode::DISCOVERY);
        auto use3 = substrate_use_memory(traversal, 256, 16);
        CHECK(use3.ptr != nullptr);
        CHECK(use3.ptr != use2.ptr);
        CHECK(use3.mode == alia::substrate_block_traversal_mode::DISCOVERY);
    }

    auto spec = substrate_get_block_spec(traversal);
    CHECK(spec.size == 768);
    CHECK(spec.alignment == 16);

    substrate_end_block(traversal, scope);

    substrate_begin_block(traversal, scope, block, spec);

    {
        auto use1 = substrate_use_memory(traversal, 256, 16);
        CHECK(use1.ptr != nullptr);
        CHECK(use1.mode == alia::substrate_block_traversal_mode::INIT);
        auto use2 = substrate_use_memory(traversal, 256, 16);
        CHECK(use2.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use2.ptr)
                - reinterpret_cast<std::uint8_t*>(use1.ptr)
            == 256);
        CHECK(use2.mode == alia::substrate_block_traversal_mode::INIT);
        auto use3 = substrate_use_memory(traversal, 256, 16);
        CHECK(use3.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use3.ptr)
                - reinterpret_cast<std::uint8_t*>(use2.ptr)
            == 256);
        CHECK(use3.mode == alia::substrate_block_traversal_mode::INIT);
    }

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    destruct_substrate_block(system, block);
    destruct_substrate_system(system);
}

TEST_CASE("substrate_use_block")
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

    {
        auto use1 = substrate_use_memory(traversal, 256, 16);
        CHECK(use1.ptr != nullptr);
        CHECK(use1.mode == alia::substrate_block_traversal_mode::INIT);
    }

    auto subblock = substrate_use_block(traversal);
    CHECK(subblock != nullptr);
    CHECK(subblock->system == &system);
    CHECK(subblock->storage == nullptr);
    CHECK(subblock->destructors == nullptr);

    alia::substrate_block_scope subscope;

    substrate_begin_block(traversal, subscope, *subblock, spec);

    {
        auto use1 = substrate_use_memory(traversal, 256, 16);
        CHECK(use1.ptr != nullptr);
        CHECK(use1.mode == alia::substrate_block_traversal_mode::INIT);
        auto use2 = substrate_use_memory(traversal, 256, 16);
        CHECK(use2.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use2.ptr)
                - reinterpret_cast<std::uint8_t*>(use1.ptr)
            == 256);
        CHECK(use2.mode == alia::substrate_block_traversal_mode::INIT);
        auto use3 = substrate_use_memory(traversal, 256, 16);
        CHECK(use3.ptr != nullptr);
        CHECK(
            reinterpret_cast<std::uint8_t*>(use3.ptr)
                - reinterpret_cast<std::uint8_t*>(use2.ptr)
            == 256);
        CHECK(use3.mode == alia::substrate_block_traversal_mode::INIT);
    }

    substrate_end_block(traversal, subscope);

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    destruct_substrate_block(system, block);
    destruct_substrate_system(system);
}

TEST_CASE("substrate_destructors")
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

    std::vector<int> destructions;

    struct test_obejct
    {
        std::vector<int>* destructions = nullptr;
        int id = 0;
    };

    auto use_object = [&](int id) {
        auto result = substrate_use_object(
            traversal, sizeof(test_obejct), alignof(test_obejct), [](void* p) {
                auto* obj = static_cast<test_obejct*>(p);
                obj->destructions->push_back(obj->id);
            });
        CHECK(result.ptr != nullptr);
        CHECK(result.mode == alia::substrate_block_traversal_mode::INIT);
        auto* obj = new (result.ptr) test_obejct;
        obj->destructions = &destructions;
        obj->id = id;
        return obj;
    };

    use_object(1);
    use_object(2);

    auto subblock = substrate_use_block(traversal);
    CHECK(subblock != nullptr);
    CHECK(subblock->system == &system);
    CHECK(subblock->storage == nullptr);
    CHECK(subblock->destructors == nullptr);

    alia::substrate_block_scope subscope;

    substrate_begin_block(traversal, subscope, *subblock, spec);

    use_object(3);
    use_object(4);

    substrate_end_block(traversal, subscope);

    use_object(5);
    use_object(6);

    substrate_end_block(traversal, scope);

    substrate_end_traversal(traversal);

    CHECK(destructions.size() == 0);

    destruct_substrate_block(system, block);
    destruct_substrate_system(system);

    CHECK(destructions.size() == 6);
    CHECK(destructions == std::vector<int>({6, 5, 4, 3, 2, 1}));
}

TEST_CASE("substrate_generations")
{
    scratch_rig rig;

    alia::substrate_system system;

    alia::substrate_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = block_alloc;
    allocator.free = block_free;

    construct_substrate_system(system, allocator);

    alia::substrate_block block;

    for (int generation = 0; generation < 2; ++generation)
    {
        alia::substrate_traversal traversal = {};
        substrate_begin_traversal(traversal, system, rig.scratch);

        alia::substrate_block_spec spec;
        spec.size = 1024;
        spec.alignment = 16;

        alia::substrate_block_scope scope;

        substrate_begin_block(traversal, scope, block, spec);

        {
            auto use = substrate_use_memory(traversal, 256, 16);
            CHECK(use.ptr != nullptr);
            CHECK(use.generation == generation);
            CHECK(use.mode == alia::substrate_block_traversal_mode::INIT);
        }

        substrate_end_block(traversal, scope);

        substrate_end_traversal(traversal);

        destruct_substrate_block(system, block);
    }

    destruct_substrate_system(system);
}
