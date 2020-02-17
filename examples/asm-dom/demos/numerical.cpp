#include "demo.hpp"

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
init_addition_ui(std::string dom_id)
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

static demo addition_ui("addition-ui", init_addition_ui);

void
do_numerical_analysis(dom::context ctx, bidirectional<double> n)
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
init_numerical_analysis(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_numerical_analysis(ctx, get_state(ctx, empty<double>()));
    });
}

static demo addition_analysis("numerical-analysis", init_numerical_analysis);

void
do_tip_calculator(dom::context ctx)
{
    // clang-format off
/// [tip-calculator]
auto bill = get_state(ctx, empty<double>());
dom::do_text(ctx, "How much is the bill?");
dom::do_input(ctx, bill);

auto tip_ratio = get_state(ctx, empty<double>());
dom::do_text(ctx, "How much do you want to tip?");
dom::do_input(ctx, scale(tip_ratio, 100)); // Users like %.
dom::do_button(ctx, "18%", tip_ratio <<= 0.18);
dom::do_button(ctx, "20%", tip_ratio <<= 0.20);
dom::do_button(ctx, "25%", tip_ratio <<= 0.25);

auto n_people = get_state(ctx, 1);
dom::do_text(ctx, "How many people?");
dom::do_input(ctx, n_people);

auto tip = bill * tip_ratio;
auto total = bill + tip;
dom::do_text(ctx, printf(ctx, "Tip: %.2f", tip));
dom::do_text(ctx, printf(ctx, "Total: %.2f", total));

alia_if(n_people > 1)
{
    dom::do_heading(ctx, "h4", "Per Person");
    dom::do_text(ctx, printf(ctx, "Tip: %.2f", tip / n_people));
    dom::do_text(ctx, printf(ctx, "Total: %.2f", total / n_people));
}
alia_end
/// [tip-calculator]
    // clang-format on
}

void
init_tip_calculator(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, do_tip_calculator);
}

static demo tip_calculator("tip-calculator-demo", init_tip_calculator);
