#define ALIA_IMPLEMENTATION
#include "alia.hpp"

#include "color.hpp"
#include "dom.hpp"

using namespace alia;
using namespace dom;

alia::system the_system;
dom::system the_dom;

void
do_ui(dom::context ctx)
{
    auto flag = get_state(ctx, false);
    dom::do_button(ctx, "Toggle!", toggle(flag));
    dom::do_colored_box(
        ctx,
        smooth(ctx, conditional(flag, rgb8(210, 210, 220), rgb8(50, 50, 55))));

    do_hr(ctx);

    dom::do_text(ctx, "Enter a number:");
    auto n = get_state(ctx, 0);
    do_input(ctx, n);
    do_text(ctx, smooth(ctx, n));
    do_button(ctx, "Add 100", n += 100);
    do_button(ctx, "Reset", n <<= 4);
}

int
main()
{
    initialize(the_dom, the_system, "root", do_ui);
    return 0;
}
