#include <alia/core/timing/smoothing.hpp>

#include <complex>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <alia/core/system/interface.hpp>

#include "traversal.hpp"

using namespace alia;

struct testing_external_interface : default_external_interface
{
    testing_external_interface(alia::system& sys)
        : default_external_interface(sys)
    {
    }

    millisecond_count tick_count = 0;

    millisecond_count
    get_tick_count() const
    {
        return tick_count;
    }
};

TEST_CASE("interpolation", "[timing][smoothing]")
{
    REQUIRE(interpolate(-1., 1., 0.6) == Catch::Approx(0.2));
    REQUIRE(interpolate(0, 10, 0.57) == 6);
    REQUIRE(interpolate(0, 10, 0.64) == 6);
    REQUIRE(interpolate(1.f, 3.f, 0.5) == Catch::Approx(2.f));
    REQUIRE(
        interpolate(std::complex<double>(0), std::complex<double>(4), 0.25)
            .real()
        == Catch::Approx(1));
}

TEST_CASE("smooth_raw", "[timing][smoothing]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    REQUIRE(!system_needs_refresh(sys));

    value_smoother<int> smoother;

    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    external.tick_count = 100;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 10, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 110;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 10, animated_transition{linear_curve, 100});
        REQUIRE(x == 1);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 150;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 5);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 170;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 3);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 200;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 0, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    external.tick_count = 300;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 20, animated_transition{linear_curve, 100});
        REQUIRE(x == 0);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 350;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 10);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 400;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 20);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 450;
    do_traversal(sys, [&](context ctx) {
        int x = smooth_raw(
            ctx, smoother, 30, animated_transition{linear_curve, 100});
        REQUIRE(x == 30);
    });
    REQUIRE(!system_needs_refresh(sys));
}

TEST_CASE("smooth", "[timing][smoothing]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    REQUIRE(!system_needs_refresh(sys));

    auto transition = animated_transition{linear_curve, 100};

    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, value(0), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    captured_id last_id;

    external.tick_count = 100;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, value(10), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 0);
        last_id.capture(x.value_id());
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 150;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, value(10), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 5);
        REQUIRE(last_id != x.value_id());
        last_id.capture(x.value_id());
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 200;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, value(10), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 10);
        REQUIRE(last_id != x.value_id());
        last_id.capture(x.value_id());
    });
    REQUIRE(!system_needs_refresh(sys));

    external.tick_count = 240;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, empty<int>(), transition);
        REQUIRE(!signal_has_value(x));
        REQUIRE(last_id != x.value_id());
    });
    REQUIRE(!system_needs_refresh(sys));
}

TEST_CASE("smooth writing", "[timing][smoothing]")
{
    alia::system sys;
    auto* external_ptr = new testing_external_interface(sys);
    initialize_standalone_system(
        sys, [](context) {}, external_ptr);
    auto& external = *external_ptr;

    REQUIRE(!system_needs_refresh(sys));

    auto transition = animated_transition{linear_curve, 100};

    int raw_x = 0;

    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, direct(raw_x), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 0);
    });
    REQUIRE(!system_needs_refresh(sys));

    raw_x = 10;

    external.tick_count = 100;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, direct(raw_x), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 0);
    });
    REQUIRE(system_needs_refresh(sys));

    external.tick_count = 150;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, direct(raw_x), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 5);
        write_signal(x, 20);
    });

    REQUIRE(raw_x == 20);

    external.tick_count = 160;
    do_traversal(sys, [&](context ctx) {
        auto x = smooth(ctx, direct(raw_x), transition);
        REQUIRE(signal_has_value(x));
        REQUIRE(read_signal(x) == 20);
    });
    REQUIRE(!system_needs_refresh(sys));
}
