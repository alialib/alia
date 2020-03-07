#include "demo.hpp"

namespace numeric_adaptors {

void
do_ui(dom::context ctx, duplex<double> n)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 1.));
    });
}

static demo the_demo("numeric-adaptors", init_demo);

} // namespace numeric_adaptors
