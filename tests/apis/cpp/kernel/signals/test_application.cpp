#include <alia/kernel/signals/application.hpp>

#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/state.hpp>
#include <alia/test/kernel/substrate_fixture.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

#include <string>

using namespace alia;
using namespace alia::operators;
using namespace alia::test;

namespace {

void
begin_refresh_block(substrate_fixture& t, alia_struct_spec& spec)
{
    alia_test_substrate_fixture_prepare_refresh_event(t.fixture, &t.ctx);
    t.reset_traversal();
    alia_substrate_begin_block(&t.ctx, t.root_anchor(), &spec);
}

struct counting_int_view
    : signal<counting_int_view, int, view_caps<signal_readable>, int>
{
    counting_int_view(int* reads, int* read_intos, int value)
        : reads_(reads), read_intos_(read_intos), value_(value)
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    int const&
    read() const override
    {
        ++*reads_;
        return value_;
    }
    void
    read_into(int* dst) const override
    {
        ++*read_intos_;
        *dst = value_;
    }
    int
    value_id() const
    {
        return value_;
    }

 private:
    int* reads_;
    int* read_intos_;
    int value_;
};

} // namespace

TEST_CASE("lazy_bidirectional_apply")
{
    effect_fixture fx;
    int n = 0;
    auto f = [](int x) -> double { return x * 2.0; };
    auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };
    auto s = lazy_bidirectional_apply(f, r, ref(n));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);
    static_assert(signal_with<decltype(s), view_caps<signal_readable>>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0.0);
    CHECK(signal_ready_to_write(s));

    auto id_before = s.value_id();
    write_signal(&fx.ctx, s, 4.0);
    fx.run();
    CHECK(n == 2);
    CHECK(read_signal(s) == 4.0);
    CHECK((s.value_id() != id_before));

    write_signal(&fx.ctx, s, 2.0);
    fx.run();
    CHECK(n == 1);
    CHECK(read_signal(s) == 2.0);
}

TEST_CASE("lazy_bidirectional_apply with empty arg")
{
    auto f = [](int x) -> double { return x * 2.0; };
    auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };
    auto s = lazy_bidirectional_apply(f, r, empty<int>());

    CHECK_FALSE(signal_has_value(s));
    CHECK_FALSE(signal_ready_to_write(s));
}

TEST_CASE("apply caches across refreshes")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};
    int calls = 0;
    auto f = [&calls](int x, int y) {
        ++calls;
        return x + y;
    };

    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, value(2), value(3));
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 5);
        CHECK(calls == 1);
        CHECK(read_signal(s) == 5);
        CHECK(calls == 1);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, value(2), value(3));
        CHECK(read_signal(s) == 5);
        CHECK(calls == 1);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, value(4), value(3));
        CHECK(read_signal(s) == 7);
        CHECK(calls == 2);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("apply with empty argument")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};
    int calls = 0;
    auto f = [&calls](int x) {
        ++calls;
        return x * 2;
    };

    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, empty<int>());
        CHECK_FALSE(signal_has_value(s));
        CHECK(calls == 0);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("apply recomputes after destructive move")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};
    int calls = 0;
    auto f = [&calls](int x) {
        ++calls;
        return x + 1;
    };

    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, value(10));
        static_assert(signal_with<decltype(s), view_caps<signal_movable>>);
        CHECK(read_signal(s) == 11);
        CHECK(calls == 1);
        CHECK(std::move(s.durable_ref()) == 11);
        CHECK(calls == 1);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto s = apply(&t.ctx, f, value(10));
        CHECK(read_signal(s) == 11);
        CHECK(calls == 2);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("apply feeds by-value args via read_into and refs via read")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    int reads = 0;
    int read_intos = 0;

    begin_refresh_block(t, spec);
    {
        auto by_value = apply(
            &t.ctx,
            [](int x) { return x + 1; },
            counting_int_view(&reads, &read_intos, 3));
        CHECK(read_signal(by_value) == 4);
        CHECK(reads == 0);
        CHECK(read_intos == 1);

        reads = 0;
        read_intos = 0;
        auto by_ref = apply(
            &t.ctx,
            [](int const& x) { return x + 1; },
            counting_int_view(&reads, &read_intos, 5));
        CHECK(read_signal(by_ref) == 6);
        CHECK(reads == 1);
        CHECK(read_intos == 0);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("lift")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};
    auto add = lift([](int a, int b) { return a + b; });

    begin_refresh_block(t, spec);
    {
        auto s = add(&t.ctx, value(1), value(2));
        CHECK(read_signal(s) == 3);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("apply with use_state input")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 1024u, .align = 16u};
    int calls = 0;
    auto f = [&calls](int x) {
        ++calls;
        return x * 10;
    };

    begin_refresh_block(t, spec);
    {
        auto n = use_state(&t.ctx, 3);
        auto s = apply(&t.ctx, f, n);
        CHECK(read_signal(s) == 30);
        CHECK(calls == 1);
        write_signal(&t.ctx, n, 4);
        t.run_effects();
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto n = use_state(&t.ctx, 3);
        CHECK(read_signal(n) == 4);
        auto s = apply(&t.ctx, f, n);
        CHECK(read_signal(s) == 40);
        CHECK(calls == 2);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}

TEST_CASE("apply with id_view value ID")
{
    substrate_fixture t;
    alia_struct_spec spec = {.size = 2048u, .align = 16u};
    int calls = 0;
    auto f = [&calls](int x) {
        ++calls;
        return x + 1;
    };
    std::string key = "alpha";

    begin_refresh_block(t, spec);
    {
        auto input = override_id(value(1), [&] { return make_id_by_reference(key); });
        auto s = apply(&t.ctx, f, input);
        CHECK(read_signal(s) == 2);
        CHECK(calls == 1);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto input = override_id(value(1), [&] { return make_id_by_reference(key); });
        auto s = apply(&t.ctx, f, input);
        CHECK(read_signal(s) == 2);
        CHECK(calls == 1);
    }
    (void) alia_substrate_end_block(&t.ctx);

    key = "beta";
    t.advance_refresh();
    begin_refresh_block(t, spec);
    {
        auto input = override_id(value(1), [&] { return make_id_by_reference(key); });
        auto s = apply(&t.ctx, f, input);
        CHECK(read_signal(s) == 2);
        CHECK(calls == 2);
    }
    (void) alia_substrate_end_block(&t.ctx);

    t.cleanup_root_block();
}
