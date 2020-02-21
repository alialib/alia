#include "demo.hpp"

void
do_switch_example(dom::context ctx, bidirectional<int> n)
{
    // clang-format off
/// [switch-example]
do_text(ctx, "Enter a number:");
do_input(ctx, n);
alia_switch(n)
{
 alia_case(0):
    do_text(ctx, "foo");
    break;
 alia_case(1):
    do_text(ctx, "bar");
 alia_case(2):
 alia_case(3):
    do_text(ctx, "baz");
    break;
 alia_default:
    do_text(ctx, "zub");
}
alia_end
/// [switch-example]
    // clang-format on
}

void
init_switch_example(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_switch_example(ctx, get_state(ctx, empty<int>()));
    });
}

static demo switch_example("switch-example", init_switch_example);
