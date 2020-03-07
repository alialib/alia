#include "demo.hpp"

namespace unready_copier {

void
do_ui(dom::context ctx, duplex<int> n, duplex<int> m)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 0), get_state(ctx, empty<int>()));
    });
}

static demo the_demo("unready-copier", init_demo);

} // namespace unready_copier

namespace action_operators {

void
do_ui(dom::context ctx, duplex<int> n)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 1));
    });
}

static demo the_demo("action-operators", init_demo);

} // namespace action_operators

namespace action_combining {

void
do_ui(dom::context ctx, duplex<int> m, duplex<int> n)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 0), get_state(ctx, 0));
    });
}

static demo the_demo("action-combining", init_demo);

} // namespace action_combining

namespace action_latching {

void
do_ui(dom::context ctx, duplex<int> in_hand, duplex<int> in_bank)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, 0), get_state(ctx, 0));
    });
}

static demo the_demo("action-latching", init_demo);

} // namespace action_latching

namespace action_binding {

void
do_ui(dom::context ctx, duplex<int> duration)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, empty<int>()));
    });
}

static demo the_demo("action-binding", init_demo);

} // namespace action_binding

namespace action_demo {

void
do_ui(dom::context ctx, duplex<std::string> message)
{
    // clang-format off
/// [lambda-action-demo]
// Define a UI for inputting a message.
do_text(ctx, "Enter a message for your browser's console:");
do_input(ctx, message);

// Create an action that takes a message as a parameter.
auto sender = lambda_action(
    [](std::string message) { std::cout << message << std::endl; });

// Bind the message to the action and hook them up to a button.
do_button(ctx, "Send", sender <<= message);
/// [lambda-action-demo]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, std::string()));
    });
}

static demo the_demo("lambda-action-demo", init_demo);

} // namespace action_demo
