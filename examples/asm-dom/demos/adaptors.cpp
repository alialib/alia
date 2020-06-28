#include "demo.hpp"

namespace numeric_adaptors {

void
demo_ui(dom::context ctx, duplex<double> n)
{
    // clang-format off
/// [numeric-adaptors]
dom::text(ctx, alia::printf(ctx, "N is %g.", n));
dom::text(ctx, "Here you can edit a scaled view of N:");
dom::input(ctx, scale(n, 10));
dom::text(ctx, "Here you can edit an offset view of N:");
dom::input(ctx, offset(n, 10));
/// [numeric-adaptors]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 1.)));
    });
}

static demo the_demo("numeric-adaptors", init_demo);

} // namespace numeric_adaptors
