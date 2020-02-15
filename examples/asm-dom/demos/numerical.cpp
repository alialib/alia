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
    /// [analysis]‎
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
