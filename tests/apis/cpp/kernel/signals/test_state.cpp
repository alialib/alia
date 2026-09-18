#include <alia/kernel/signals/state.hpp>
#include <alia/kernel/signals/operators.hpp>

#include <alia/test/kernel/substrate_fixture.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::operators;
using namespace alia::test;

TEST_CASE("state_binding write and clear")
{
    effect_fixture fx;
    state_storage<int> storage;
    
    auto s = make_state_binding(storage, &fx.ctx);

    CHECK_FALSE(signal_has_value(s));
    CHECK(
        (static_cast<untyped_signal_base const&>(s).value_id_erased()
         == null_id()));

    write_signal(&fx.ctx, s, 7);
    fx.run();
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 7);
    CHECK(s.value_id() == storage.version);
    CHECK(storage.version == 3u);

    write_signal(&fx.ctx, s, 9);
    fx.run();
    CHECK(read_signal(s) == 9);
    CHECK(storage.version == 5u);

    clear_signal(&fx.ctx, s);
    fx.run();
    CHECK_FALSE(signal_has_value(s));
    CHECK((storage.version & 1u) == 0u);
}

TEST_CASE("state_binding durable_ref is pure; post_mutation_commit tracks changes")
{
    effect_fixture fx;
    state_storage<int> storage;

    auto s = make_state_binding(storage, &fx.ctx);
    write_signal(&fx.ctx, s, 4);
    fx.run();

    uint32_t const before = storage.version;
    int& ref = s.durable_ref();
    CHECK(storage.version == before);
    ref = 5;
    auto id = commit_signal_mutation(&fx.ctx, s);
    CHECK(id);
    CHECK(*id == before + 2u);
    fx.run();
    CHECK(storage.version == before + 2u);
    CHECK(read_signal(s) == 5);
}

TEST_CASE("state field write mutates in place")
{
    effect_fixture fx;
    struct point
    {
        int x;
        int y;
    };
    state_storage<point> storage;
    auto s = make_state_binding(storage, &fx.ctx);
    write_signal(&fx.ctx, s, point{1, 2});
    fx.run();

    auto x = s->*&point::x;
    static_assert(signal_with<decltype(x), view_caps<signal_durable>>);
    static_assert(!signal_with<decltype(x), view_caps<signal_movable>>);

    point const* original = &storage.value;
    write_signal(&fx.ctx, x, 9);
    fx.run();
    CHECK(&storage.value == original);
    CHECK(storage.value.x == 9);
    CHECK(storage.value.y == 2);
}

TEST_CASE("use_state persists across traversals")
{
    substrate_fixture t;

    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto s = use_state(&t.ctx, 42);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 42);
        write_signal(&t.ctx, s, 100);
    t.run_effects();
        CHECK(read_signal(s) == 100);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
    {
        auto s = use_state(&t.ctx, 42);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 100);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}
