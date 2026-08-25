#include <alia/kernel/macros.hpp>
#include <alia/kernel/substrate.hpp>

#include <alia/impl/events.hpp>
#include <alia/test/kernel/substrate_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

// Run `content` until refresh is complete. A discovery pass marks refresh
// incomplete so the system re-traverses for INIT.
template<class Content>
void
traverse_until_complete(
    substrate_fixture& t, alia_struct_spec& root_spec, Content&& content)
{
    for (;;)
    {
        t.reset_traversal();
        alia_context* ctx = &t.ctx;
        alia_substrate_begin_block(ctx, t.root_anchor(), &root_spec);
        content(ctx);
        (void) alia_substrate_end_block(ctx);
        if (!as_refresh_event(*ctx).incomplete)
            break;
    }
    t.advance_refresh();
}

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
        CHECK(obj.is_fresh());
        CHECK(obj->n == 0);
        obj->n = 11;
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto obj = use_object<counted>(&t.ctx);
        CHECK_FALSE(obj.is_fresh());
        CHECK_FALSE(obj.is_init());
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

    t.advance_refresh();
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

TEST_CASE("use_object clear_cache is a no-op without the member")
{
    substrate_fixture t;

    alia_struct_spec root_spec = {.size = 1024u, .align = 16u};
    alia_struct_spec child_spec = {.size = 256u, .align = 16u};

    struct persist_only
    {
        int n = 0;
    };

    alia_substrate_anchor* child = nullptr;

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    child = use_anchor(&t.ctx);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto obj = use_object<persist_only>(&t.ctx);
        CHECK(obj.is_init());
        obj->n = 42;
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_substrate_deactivate_anchor(
        alia_test_substrate_fixture_system(t.fixture), child);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    CHECK(use_anchor(&t.ctx) == child);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto obj = use_object<persist_only>(&t.ctx);
        CHECK_FALSE(obj.is_fresh());
        CHECK(obj->n == 42);
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("use_object clear_cache runs the member")
{
    substrate_fixture t;

    alia_struct_spec root_spec = {.size = 1024u, .align = 16u};
    alia_struct_spec child_spec = {.size = 256u, .align = 16u};

    struct with_cache
    {
        int persist = 0;
        int cached = 0;

        void
        clear_cache()
        {
            cached = 0;
        }
    };

    alia_substrate_anchor* child = nullptr;

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    child = use_anchor(&t.ctx);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto obj = use_object<with_cache>(&t.ctx);
        obj->persist = 7;
        obj->cached = 99;
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_substrate_deactivate_anchor(
        alia_test_substrate_fixture_system(t.fixture), child);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    CHECK(use_anchor(&t.ctx) == child);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto obj = use_object<with_cache>(&t.ctx);
        CHECK_FALSE(obj.is_fresh());
        CHECK(obj->persist == 7);
        CHECK(obj->cached == 0);
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("use_cache recreates after deactivate")
{
    substrate_fixture t;

    alia_struct_spec root_spec = {.size = 1024u, .align = 16u};
    alia_struct_spec child_spec = {.size = 256u, .align = 16u};

    struct cached_value
    {
        int n = 0;
    };

    alia_substrate_anchor* child = nullptr;

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    child = use_anchor(&t.ctx);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto cache = use_cache<cached_value>(&t.ctx);
        CHECK(cache.is_fresh());
        CHECK(cache.is_init());
        cache->n = 5;
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_substrate_deactivate_anchor(
        alia_test_substrate_fixture_system(t.fixture), child);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    CHECK(use_anchor(&t.ctx) == child);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto cache = use_cache<cached_value>(&t.ctx);
        CHECK(cache.is_fresh());
        CHECK_FALSE(cache.is_init());
        CHECK(cache->n == 0);
        cache->n = 8;
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &root_spec);
    CHECK(use_anchor(&t.ctx) == child);
    alia_substrate_begin_block(&t.ctx, child, &child_spec);
    {
        auto cache = use_cache<cached_value>(&t.ctx);
        CHECK_FALSE(cache.is_fresh());
        CHECK(cache->n == 8);
    }
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("alia_if/alia_else preserves persist and clears cache")
{
    substrate_fixture t;

    alia_struct_spec root_spec = {.size = 2048u, .align = 16u};

    struct with_cache
    {
        int persist = 0;
        int cached = 0;

        void
        clear_cache()
        {
            cached = 0;
        }
    };

    int then_visits = 0;
    int then_cached = -1;
    int then_persist_field = -1;
    int else_visits = 0;

    auto run = [&](bool take_then) {
        traverse_until_complete(t, root_spec, [&](alia_context* ctx) {
            alia_if (take_then)
            {
                auto visits = use_memory<int>(ctx);
                auto obj = use_object<with_cache>(ctx);
                if (obj.is_init())
                {
                    *visits = 1;
                    obj->persist = 1;
                    obj->cached = 10;
                }
                else if (!obj.is_fresh())
                {
                    CHECK(obj->persist == 1);
                    ++*visits;
                    ++obj->cached;
                }
                if (obj.is_init() || !obj.is_fresh())
                {
                    then_visits = *visits;
                    then_cached = obj->cached;
                    then_persist_field = obj->persist;
                }
            }
            alia_else
            {
                auto visits = use_memory<int>(ctx);
                auto obj = use_object<with_cache>(ctx);
                if (obj.is_init())
                {
                    *visits = 1;
                    obj->persist = 2;
                    obj->cached = 20;
                }
                else if (!obj.is_fresh())
                {
                    CHECK(obj->persist == 2);
                    ++*visits;
                    ++obj->cached;
                }
                if (obj.is_init() || !obj.is_fresh())
                    else_visits = *visits;
            }
            alia_end
        });
    };

    run(true);
    CHECK(then_visits == 1);
    CHECK(then_cached == 10);
    CHECK(then_persist_field == 1);

    run(true);
    CHECK(then_visits == 2);
    CHECK(then_cached == 11);

    run(false);
    CHECK(else_visits == 1);

    run(false);
    CHECK(else_visits == 2);

    // Confirm that returning to the then-branch keeps the visit counter and
    // persist field, while cached state was cleared while inactive and then
    // incremented once on this visit.
    run(true);
    CHECK(then_visits == 3);
    CHECK(then_persist_field == 1);
    CHECK(then_cached == 1);

    t.cleanup_root_block();
}

TEST_CASE("alia_if/alia_else_if/alia_else share one chain")
{
    substrate_fixture t;

    alia_struct_spec root_spec = {.size = 2048u, .align = 16u};

    int visited = -1;
    int branch_value = -1;

    auto run = [&](int which) {
        visited = -1;
        branch_value = -1;
        traverse_until_complete(t, root_spec, [&](alia_context* ctx) {
            alia_if (which == 0)
            {
                auto n = use_memory<int>(ctx);
                if (n.is_init())
                    *n = 100;
                if (n.is_init() || !n.is_fresh())
                {
                    visited = 0;
                    branch_value = *n;
                    CHECK(*n == 100);
                }
            }
            alia_else_if (which == 1)
            {
                auto n = use_memory<int>(ctx);
                if (n.is_init())
                    *n = 200;
                if (n.is_init() || !n.is_fresh())
                {
                    visited = 1;
                    branch_value = *n;
                    CHECK(*n == 200);
                }
            }
            alia_else
            {
                auto n = use_memory<int>(ctx);
                if (n.is_init())
                    *n = 300;
                if (n.is_init() || !n.is_fresh())
                {
                    visited = 2;
                    branch_value = *n;
                    CHECK(*n == 300);
                }
            }
            alia_end
        });
    };

    run(0);
    CHECK(visited == 0);
    CHECK(branch_value == 100);
    run(1);
    CHECK(visited == 1);
    CHECK(branch_value == 200);
    run(2);
    CHECK(visited == 2);
    CHECK(branch_value == 300);
    run(0);
    CHECK(visited == 0);
    CHECK(branch_value == 100);
    run(1);
    CHECK(visited == 1);
    CHECK(branch_value == 200);

    t.cleanup_root_block();
}
