#include <alia/kernel/signals/state.hpp>

#include <alia/test/kernel/substrate_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::operators;
using namespace alia::test;

TEST_CASE("state_binding write and clear")
{
    state_storage<int> storage;
    alia_context ctx{};
    auto s = make_state_binding(storage, &ctx);

    CHECK_FALSE(signal_has_value(s));
    CHECK(
        (static_cast<untyped_signal_base const&>(s).value_id_view()
         == null_id()));

    write_signal(s, 7);
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 7);
    CHECK(s.value_id() == storage.version);
    CHECK(storage.version == 3u);

    write_signal(s, 9);
    CHECK(read_signal(s) == 9);
    CHECK(storage.version == 5u);

    clear_signal(s);
    CHECK_FALSE(signal_has_value(s));
    CHECK((storage.version & 1u) == 0u);
}

TEST_CASE("state_binding move_out bumps version without dirty tracking")
{
    state_storage<int> storage;
    alia_context ctx{};
    auto s = make_state_binding(storage, &ctx);
    write_signal(s, 4);

    uint32_t const before = storage.version;
    int const moved = s.move_out();
    CHECK(moved == 4);
    CHECK(storage.version == before + 2u);
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
        write_signal(s, 100);
        CHECK(read_signal(s) == 100);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_frame();
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
