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
dom::do_button(ctx, "Double", n *= 2);
dom::do_button(ctx, "Halve", n /= 2);
dom::do_button(ctx, "Square", n *= n);
dom::do_button(ctx, "Increment", ++n);
dom::do_button(ctx, "Decrement", n--);
dom::do_button(ctx, "Reset", n <<= 1);
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

void
do_action_combining(
    dom::context ctx, bidirectional<int> m, bidirectional<int> n)
{
    // clang-format off
/// [action-combining]
dom::do_text(ctx, printf(ctx, "M is %d and N is %d.", m, n));
dom::do_button(ctx, "Increment M", ++m);
dom::do_button(ctx, "Increment N", ++n);
dom::do_button(ctx, "Reset Both", (m <<= 0, n <<= 0));
/// [action-combining]
    // clang-format on
}

void
init_action_combining(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_action_combining(ctx, get_state(ctx, 0), get_state(ctx, 0));
    });
}

static demo action_combining("action-combining", init_action_combining);

void
do_action_latching(
    dom::context ctx, bidirectional<int> in_hand, bidirectional<int> in_bank)
{
    // clang-format off
/// [action-latching]
dom::do_text(ctx,
    printf(ctx,
        "You have %d coin(s) in hand and %d in the bank.",
        in_hand, in_bank));
dom::do_button(ctx, "Pick Up a Coin", ++in_hand);
dom::do_button(ctx, "Deposit Your Coins", (in_hand <<= 0, in_bank += in_hand));
/// [action-latching]
    // clang-format on
}

void
init_action_latching(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_action_latching(ctx, get_state(ctx, 0), get_state(ctx, 0));
    });
}

static demo action_latching("action-latching", init_action_latching);

void
do_action_binding(dom::context ctx, bidirectional<int> duration)
{
    // clang-format off
/// [action-binding]
animation_timer timer(ctx);

alia_if(timer.is_active())
{
    do_text(ctx, printf(ctx, "%d ms left.", timer.ticks_left()));
}
alia_else
{
    do_text(ctx, "The timer is stopped.");
}
alia_end

do_text(ctx, "Enter a duration in milliseconds:");
do_input(ctx, duration);
do_button(ctx, "Start", timer.start() <<= duration);
/// [action-binding]
    // clang-format on
}

void
init_action_binding(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_action_binding(ctx, get_state(ctx, empty<int>()));
    });
}

static demo action_binding("action-binding", init_action_binding);

void
do_lambda_action_demo(dom::context ctx, bidirectional<std::string> message)
{
    // clang-format off
/// [lambda-action-demo]
do_text(ctx, "Enter a message for your browser's console:");
do_input(ctx, message);
auto sender = lambda_action(
    [](std::string message) { std::cout << message << std::endl; });
do_button(ctx, "Send", sender <<= message);
/// [lambda-action-demo]
    // clang-format on
}

void
init_lambda_action_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_lambda_action_demo(ctx, get_state(ctx, std::string()));
    });
}

static demo lambda_action_demo("lambda-action-demo", init_lambda_action_demo);
