#include <alia/animation/transitions.hpp>

#include <doctest/doctest.h>

using namespace alia;

namespace {

struct test_system
{
    transition_animation_map transitions;
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
    smoothing_bitfield smoothing;
};

using test_bits = bitpack<test_layout>;

} // namespace

TEST_CASE("simple animated transition")
{
    test_context ctx;
    test_bits bits = {0};
    auto animation_bits = ALIA_BITREF(bits, smoothing);
    REQUIRE(!ctx.animation_in_progress);

    auto invoke_smoother
        = [&](nanosecond_count tick_count, bool current_state) {
              ctx.animation_in_progress = false;
              ctx.tick_count = tick_count;
              return smooth_between_values(
                  ctx,
                  animation_bits,
                  current_state,
                  0.0f,
                  1.0f,
                  {linear_curve, milliseconds(1000)});
          };

    REQUIRE(invoke_smoother(milliseconds(0), true) == 0.0f);
    REQUIRE(!ctx.animation_in_progress);

    REQUIRE(invoke_smoother(milliseconds(0), false) == 0.0f);
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(
        invoke_smoother(milliseconds(400), false) == doctest::Approx(0.4f));
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(
        invoke_smoother(milliseconds(800), false) == doctest::Approx(0.8f));
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(
        invoke_smoother(milliseconds(1200), false) == doctest::Approx(1.0f));
    REQUIRE(!ctx.animation_in_progress);
}

TEST_CASE("animated transition reversal")
{
    test_context ctx;
    test_bits bits = {0};
    auto animation_bits = ALIA_BITREF(bits, smoothing);
    REQUIRE(!ctx.animation_in_progress);

    auto invoke_smoother
        = [&](nanosecond_count tick_count, bool current_state) {
              ctx.animation_in_progress = false;
              ctx.tick_count = tick_count;
              return smooth_between_values(
                  ctx,
                  animation_bits,
                  current_state,
                  0.0f,
                  1.0f,
                  {linear_curve, milliseconds(1000)});
          };

    REQUIRE(invoke_smoother(milliseconds(0), true) == 0.0f);
    REQUIRE(!ctx.animation_in_progress);

    REQUIRE(invoke_smoother(milliseconds(0), false) == 0.0f);
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(
        invoke_smoother(milliseconds(400), false) == doctest::Approx(0.4f));
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(invoke_smoother(milliseconds(400), true) == doctest::Approx(0.4f));
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(invoke_smoother(milliseconds(600), true) == doctest::Approx(0.2f));
    REQUIRE(ctx.animation_in_progress);

    REQUIRE(invoke_smoother(milliseconds(1000), true) == 0.0f);
    REQUIRE(!ctx.animation_in_progress);
}
