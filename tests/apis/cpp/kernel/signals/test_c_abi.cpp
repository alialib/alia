#include <alia/kernel/signals/c_abi.hpp>

#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("inline_c_signal concept")
{
    static_assert(inline_c_signal<alia_bool_signal>);
    static_assert(inline_c_signal<alia_double_signal>);
}

TEST_CASE("to_inline_c_signal packs a readable view")
{
    auto const c = to_inline_c_signal<alia_bool_signal>(value(true));
    CHECK((c.flags & ALIA_SIGNAL_READABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITABLE) == 0);
    CHECK(c.value == true);
}

TEST_CASE("to_inline_c_signal packs a writable binding")
{
    bool x = true;
    auto const c = to_inline_c_signal<alia_bool_signal>(ref(x));
    CHECK((c.flags & ALIA_SIGNAL_READABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITTEN) == 0);
    CHECK(c.value == true);
}

TEST_CASE("to_inline_c_signal packs an empty binding")
{
    auto const c = to_inline_c_signal<alia_bool_signal>(empty<bool>());
    CHECK(c.flags == 0);
}

TEST_CASE("to_inline_c_signal packs disable_writes")
{
    bool x = true;
    auto const c
        = to_inline_c_signal<alia_bool_signal>(disable_writes(ref(x)));
    CHECK((c.flags & ALIA_SIGNAL_READABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITABLE) == 0);
    CHECK(c.value == true);
}

TEST_CASE("to_inline_c_signal packs fake_writability")
{
    auto const c
        = to_inline_c_signal<alia_bool_signal>(fake_writability(value(false)));
    CHECK((c.flags & ALIA_SIGNAL_READABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITABLE) == 0);
    CHECK(c.value == false);
}

TEST_CASE("to_inline_c_signal converts float to double")
{
    float x = 1.5f;
    auto const c = to_inline_c_signal<alia_double_signal>(ref(x));
    CHECK((c.flags & ALIA_SIGNAL_READABLE) != 0);
    CHECK((c.flags & ALIA_SIGNAL_WRITABLE) != 0);
    CHECK(c.value == doctest::Approx(1.5));
}

TEST_CASE("write_back_c_signal is a no-op when signal isn't written")
{
    effect_fixture fx;
    bool x = false;
    auto s = ref(x);
    auto c = to_inline_c_signal<alia_bool_signal>(s);
    write_back_c_signal(&fx.ctx, s, c);
    fx.run();
    CHECK(x == false);
}

TEST_CASE("write_back_c_signal posts a written value")
{
    effect_fixture fx;
    bool x = false;
    auto s = ref(x);
    auto c = to_inline_c_signal<alia_bool_signal>(s);
    c.value = true;
    c.flags |= ALIA_SIGNAL_WRITTEN;
    write_back_c_signal(&fx.ctx, s, c);
    fx.run();
    CHECK(x == true);
}

TEST_CASE("write_back_c_signal converts double to float")
{
    effect_fixture fx;
    float x = 1.f;
    auto s = ref(x);
    auto c = to_inline_c_signal<alia_double_signal>(s);
    c.value = 2.25;
    c.flags |= ALIA_SIGNAL_WRITTEN;
    write_back_c_signal(&fx.ctx, s, c);
    fx.run();
    CHECK(x == doctest::Approx(2.25f));
}

TEST_CASE("write_back_c_signal is a no-op when writes are disabled")
{
    effect_fixture fx;
    bool x = false;
    auto s = disable_writes(ref(x));
    alia_bool_signal c{
        .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITTEN,
        .value = true,
    };
    write_back_c_signal(&fx.ctx, s, c);
    fx.run();
    CHECK(x == false);
}

TEST_CASE("erased binding works with inline C signals")
{
    effect_fixture fx;
    bool x = false;
    auto r = ref(x);
    binding<bool> s = r;
    auto c = to_inline_c_signal<alia_bool_signal>(s);
    CHECK(c.value == false);
    c.value = true;
    c.flags |= ALIA_SIGNAL_WRITTEN;
    write_back_c_signal(&fx.ctx, s, c);
    fx.run();
    CHECK(x == true);
}
