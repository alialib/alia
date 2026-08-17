#include <alia/kernel/substrate.hpp>

#include <alia/test/kernel/substrate_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("use_object persists across traversals")
{
    substrate_fixture t;

    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    struct counted
    {
        int n = 0;
    };

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto obj = use_object<counted>(&t.ctx);
        CHECK(obj.is_init());
        CHECK(obj->n == 0);
        obj->n = 11;
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_frame();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto obj = use_object<counted>(&t.ctx);
        CHECK_FALSE(obj.is_fresh());
        CHECK(obj->n == 11);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("use_memory persists across traversals")
{
    substrate_fixture t;

    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto mem = use_memory<int>(&t.ctx);
        CHECK(mem.is_fresh());
        *mem = 7;
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_frame();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto mem = use_memory<int>(&t.ctx);
        CHECK_FALSE(mem.is_fresh());
        CHECK(*mem == 7);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}
