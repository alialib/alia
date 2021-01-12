#include "demo.hpp"
#include <alia/html/async.hpp>

#include <cmath>
#include <future>

namespace time_signal {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [time-signal]
html::p(ctx,
    alia::printf(ctx,
        "It's been %d seconds since you started looking at this page.",
        get_animation_tick_count(ctx) / 1000));
/// [time-signal]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("time-signal", init_demo);

} // namespace time_signal

namespace simple_animation {

void
demo_ui(html::context ctx)
{
    // clang-format off
    /// [simple-animation]
colored_box(
    ctx,
    interpolate(
        rgb8(255, 255, 255),
        rgb8(0, 118, 255),
        (1 + std::sin(get_raw_animation_tick_count(ctx) / 600.)) / 2));
    /// [simple-animation]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("simple-animation", init_demo);

} // namespace simple_animation

namespace number_smoothing {

void
demo_ui(html::context ctx, duplex<int> n)
{
    // clang-format off
/// [number-smoothing]
html::p(ctx, "Enter N:");
html::input(ctx, n);
html::button(ctx, "1", n <<= 1);
html::button(ctx, "100", n <<= 100);
html::button(ctx, "10000", n <<= 10000);
html::p(ctx, "Here's a smoothed view of N:");
html::p(ctx, smooth(ctx, n));
/// [number-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 42)));
    });
}

static demo the_demo("number-smoothing", init_demo);

} // namespace number_smoothing

namespace color_smoothing {

void
demo_ui(html::context ctx, duplex<rgb8> color)
{
    // clang-format off
/// [color-smoothing]
colored_box(
    ctx, smooth(ctx, color, animated_transition{ease_in_out_curve, 200}));
html::button(ctx, "Go Light", color <<= rgb8(210, 210, 220));
html::button(ctx, "Go Dark", color <<= rgb8(50, 50, 55));
/// [color-smoothing]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, get_state(ctx, rgb8(50, 50, 55)));
    });
}

static demo the_demo("color-smoothing", init_demo);

} // namespace color_smoothing

namespace flickering {

void
demo_ui(html::context ctx, duplex<int> n)
{
    // clang-format off
/// [flickering]
html::p(ctx, "Enter N:");
html::input(ctx, n);
html::button(ctx, "++", ++n);
html::p(ctx, "Here's a flickering signal carrying N * 2:");
html::p(ctx,
    alia::async<int>(ctx,
        [](auto ctx, auto reporter, auto n) {
            html::async_call([=]() {
                reporter.report_success(n * 2);
            }, 200);
        },
        n));
/// [flickering]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 1)));
    });
}

static demo the_demo("flickering-demo", init_demo);

} // namespace flickering

namespace deflickering {

void
demo_ui(html::context ctx, duplex<int> n)
{
    // clang-format off
/// [deflickering]
html::p(ctx, "Enter N:");
html::input(ctx, n);
html::button(ctx, "++", ++n);
html::p(ctx, "Here's the same signal from above with deflickering:");
html::p(ctx,
    deflicker(ctx,
        alia::async<int>(ctx,
            [](auto ctx, auto reporter, auto n) {
                html::async_call([=]() {
                    reporter.report_success(n * 2);
                }, 200);
            },
            n)));
/// [deflickering]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, get_state(ctx, 1));
    });
}

static demo the_demo("deflickering-demo", init_demo);

} // namespace deflickering
