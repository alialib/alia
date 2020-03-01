#include "demo.hpp"

namespace addition_ui {

/// [addition-ui]
void
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    dom::do_text(ctx, "Enter two numbers to add:");

    dom::do_input(ctx, a);
    dom::do_input(ctx, b);

    dom::do_text(ctx, a + b);
}
/// [addition-ui]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_addition_ui(
            ctx,
            get_state(ctx, empty<double>()),
            get_state(ctx, empty<double>()));
    });
}

static demo the_demo("addition-ui", init_demo);

} // namespace addition_ui

namespace numerical_analysis {

void
do_ui(dom::context ctx, bidirectional<double> n)
{
    // clang-format off
/// [analysis]
dom::do_text(ctx, "Enter a number:");

dom::do_input(ctx, n);

alia_if(n > 0)
{
    dom::do_text(ctx, "The number is positive!");
}
alia_else_if(n < 0)
{
    dom::do_text(ctx, "The number is negative!");
}
alia_else
{
    dom::do_text(ctx, "The number is zero!");
}
alia_end
/// [analysis]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, empty<double>()));
    });
}

static demo the_demo("numerical-analysis", init_demo);

} // namespace numerical_analysis

namespace tip_calculator {

void
do_tip_calculator(dom::context ctx)
{
    // clang-format off
/// [tip-calculator]
auto bill = get_state(ctx, empty<double>());
dom::do_text(ctx, "How much is the bill?");
dom::do_input(ctx, bill);

auto tip_rate = get_state(ctx, empty<double>());
dom::do_text(ctx, "What percentage do you want to tip?");
dom::do_input(ctx, scale(tip_rate, 100));
dom::do_button(ctx, "18%", tip_rate <<= 0.18);
dom::do_button(ctx, "20%", tip_rate <<= 0.20);
dom::do_button(ctx, "25%", tip_rate <<= 0.25);

auto tip = bill * tip_rate;
auto total = bill + tip;
dom::do_text(ctx,
    printf(ctx, "You should tip %.2f, for a total of %.2f.", tip, total));

alia_if (total < 10)
{
    dom::do_text(ctx,
        "You should consider using cash for small amounts like this.");
}
alia_end
/// [tip-calculator]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, do_tip_calculator);
}

static demo the_demo("tip-calculator-demo", init_demo);

} // namespace tip_calculator
