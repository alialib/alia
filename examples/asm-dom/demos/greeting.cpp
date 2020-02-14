#include "demo.hpp"

/// [greeting]
void
do_greeting_ui(dom::context ctx, bidirectional<string> name)
{
    dom::do_text(ctx, "What's your name?");

    // Allow the user to input their name.
    dom::do_input(ctx, name);

    // If we have a name, greet the user.
    alia_if(name != "")
    {
        dom::do_text(ctx, "Hello, " + name + "!");
    }
    alia_end
}
/// [greeting]

void
init_greeting_ui(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
}

static demo greeting_ui("greeting-ui", init_greeting_ui);

/// [hello-button]
void
do_hello_button(dom::context ctx, bidirectional<bool> show_message)
{
    dom::do_button(
        ctx, "Toggle the Secret Message", show_message <<= !show_message);

    alia_if(show_message)
    {
        dom::do_text(ctx, "Hello, World!");
    }
    alia_end
}
/// [hello-button]

void
init_hello_button(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_hello_button(ctx, get_state(ctx, false));
    });
}

static demo hello_button("hello-button", init_hello_button);
