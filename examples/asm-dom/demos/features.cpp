#include "demo.hpp"

namespace stateful_component {

/// [stateful-component]
// This implements a single "flashcard" component.
// 'n' is the number that the user is trying to guess.
void
sqrt_flashcard(dom::context ctx, readable<int> n)
{
    // Put this whole component inside an 'item' div...
    dom::scoped_div div(ctx, value("item"));

    // Show the square.
    dom::text(ctx, n * n);

    // Get the local state we need for this component.
    auto answer_revealed = alia::get_state(ctx, false);

    // If the answer is revealed, show it. Otherwise show a button to reveal it.
    alia_if(answer_revealed)
    {
        dom::text(ctx, alia::printf(ctx, "The square root is %d.", n));
    }
    alia_else
    {
        dom::button(ctx, "Show Answer", answer_revealed <<= true);
    }
    alia_end
}

// Do the main UI for the "app".
void
app_ui(dom::context ctx)
{
    dom::text(ctx, "Try to figure out the square roots of these numbers...");
    sqrt_flashcard(ctx, value(9));
    sqrt_flashcard(ctx, value(4));
    sqrt_flashcard(ctx, value(5));
    sqrt_flashcard(ctx, value(17));
    sqrt_flashcard(ctx, value(4));
}
/// [stateful-component]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(
        the_dom, the_system, dom_id, [](dom::context ctx) { app_ui(ctx); });
}

static demo the_demo("stateful-component", init_demo);

} // namespace stateful_component
