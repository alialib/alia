#include "demo.hpp"

void
do_unready_copier(dom::context ctx, bidirectional<int> n, bidirectional<int> m)
{
    // clang-format off
/// [unready-copier]
dom::do_text(ctx, printf(ctx, "N is %d.", n));
dom::do_text(ctx, "What would you like to set N to?");
dom::do_input(ctx, m);
dom::do_button(ctx, "Set It!", n <<= m);
/// [unready-copier]
    // clang-format on
}

void
init_unready_copier(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_unready_copier(ctx, get_state(ctx, 0), get_state(ctx, empty<int>()));
    });
}

static demo unready_copier("unready-copier", init_unready_copier);

void
do_action_operators(dom::context ctx, bidirectional<int> n)
{
    // clang-format off
/// [action-operators]
dom::do_text(ctx, printf(ctx, "N is %d.", n));
dom::do_button(ctx, "double", n *= 2);
dom::do_button(ctx, "halve", n /= 2);
dom::do_button(ctx, "square", n *= n);
dom::do_button(ctx, "increment", ++n);
dom::do_button(ctx, "decrement", n--);
dom::do_button(ctx, "reset", n <<= 1);
/// [action-operators]
    // clang-format on
}

void
init_action_operators(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_action_operators(ctx, get_state(ctx, 1));
    });
}

static demo action_operators("action-operators", init_action_operators);
