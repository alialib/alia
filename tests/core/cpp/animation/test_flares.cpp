#include <alia/animation/flares.hpp>

#include <doctest/doctest.h>

using namespace alia;

namespace {

struct test_system
{
    flare_map flares;
};

struct test_context
{
    test_system system;
    nanosecond_count tick_count = 0;
    bool animation_in_progress = false;
};

test_system&
get_animation_system(test_context& ctx)
{
    return ctx.system;
}

nanosecond_count
get_tick_count(test_context& ctx)
{
    return ctx.tick_count;
}

void
mark_animation_in_progress(test_context& ctx)
{
    ctx.animation_in_progress = true;
}

struct test_layout
{
    flare_bit_field flares;
};

using test_bits = bitpack<test_layout>;

} // namespace

TEST_CASE("simple flares")
{
    test_context ctx;
    test_bits bits = {0};
    auto flare_bits = ALIA_BITREF(bits, flares);
    REQUIRE(!ctx.animation_in_progress);

    auto check_flares =
        [&](nanosecond_count tick_count,
            std::initializer_list<nanosecond_count> expected_flares) {
            ctx.animation_in_progress = false;
            ctx.tick_count = tick_count;
            auto expected_flare = expected_flares.begin();
            process_flares(ctx, flare_bits, [&](nanosecond_count ticks_left) {
                REQUIRE(expected_flare != expected_flares.end());
                REQUIRE(ticks_left == *expected_flare);
                ++expected_flare;
            });
            REQUIRE(expected_flare == expected_flares.end());
        };

    check_flares(milliseconds(0), {});
    REQUIRE(!ctx.animation_in_progress);

    fire_flare(ctx, flare_bits, milliseconds(1000));

    check_flares(milliseconds(100), {milliseconds(900)});
    REQUIRE(ctx.animation_in_progress);

    fire_flare(ctx, flare_bits, milliseconds(1000));

    check_flares(milliseconds(200), {milliseconds(800), milliseconds(900)});
    REQUIRE(ctx.animation_in_progress);

    check_flares(milliseconds(500), {milliseconds(500), milliseconds(600)});
    REQUIRE(ctx.animation_in_progress);

    check_flares(milliseconds(1050), {milliseconds(50)});
    REQUIRE(ctx.animation_in_progress);

    check_flares(milliseconds(1200), {});
    REQUIRE(!ctx.animation_in_progress);
}
