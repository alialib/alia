#include "demo.hpp"

namespace numeric_adaptors {

void
demo_ui(html::context ctx, duplex<double> n)
{
    // clang-format off
/// [numeric-adaptors]
html::text(ctx, alia::printf(ctx, "N is %g.", n));
html::text(ctx, "Here you can edit a scaled view of N:");
html::input(ctx, scale(n, 10));
html::text(ctx, "Here you can edit an offset view of N:");
html::input(ctx, offset(n, 10));
/// [numeric-adaptors]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static html::system the_dom;

    initialize(the_dom, the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, 1.)));
    });
}

static demo the_demo("numeric-adaptors", init_demo);

} // namespace numeric_adaptors
