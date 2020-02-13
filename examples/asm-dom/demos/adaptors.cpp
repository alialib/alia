#include "demo.hpp"

void
do_numeric_adaptors(dom::context ctx, bidirectional<double> n)
{
    // clang-format off
/// [numeric-adaptors]
dom::do_text(ctx, printf(ctx, "N is %g.", n));
dom::do_text(ctx, "Here you can edit a scaled view of N:");
dom::do_input(ctx, scale(n, 10));
dom::do_text(ctx, "Here you can edit an offset view of N:");
dom::do_input(ctx, offset(n, 10));
/// [numeric-adaptors]
    // clang-format on
}

void
init_numeric_adaptors(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_numeric_adaptors(ctx, get_state(ctx, 1.));
    });
}

static demo numeric_adaptors("numeric-adaptors", init_numeric_adaptors);
