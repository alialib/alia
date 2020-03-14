#include "demo.hpp"

namespace stateful_component {

/// [stateful-component]
// This implements a single "flashcard" component.
// 'n' is the number that the user is trying to guess.
void
do_sqrt_flashcard(dom::context ctx, readable<int> n)
{
    // Put this whole component inside an 'item' div...
    dom::scoped_div div(ctx, value("item"));

    // Show the square.
    dom::do_text(ctx, n * n);

    // Get the local state we need for this component.
    auto answer_revealed = get_state(ctx, false);

    // If the answer is revealed, show it. Otherwise show a button to reveal it.
    alia_if(answer_revealed)
    {
        dom::do_text(ctx, printf(ctx, "The square root is %d.", n));
    }
    alia_else
    {
        dom::do_button(ctx, "Show Answer", answer_revealed <<= true);
    }
    alia_end
}

// Do the main UI for the "app".
void
do_app_ui(dom::context ctx)
{
    dom::do_text(ctx, "Try to figure out the square roots of these numbers...");
    do_sqrt_flashcard(ctx, value(9));
    do_sqrt_flashcard(ctx, value(4));
    do_sqrt_flashcard(ctx, value(5));
    do_sqrt_flashcard(ctx, value(17));
    do_sqrt_flashcard(ctx, value(4));
}
/// [stateful-component]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(
        the_dom, the_system, dom_id, [](dom::context ctx) { do_app_ui(ctx); });
}

static demo the_demo("stateful-component", init_demo);

} // namespace stateful_component
