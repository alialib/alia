#include "demo.hpp"

namespace hello_world {

/// [greeting]
void
do_greeting_ui(dom::context ctx, duplex<string> name)
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
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
}

static demo the_demo("greeting-ui", init_demo);

} // namespace hello_world

namespace expanded_greeting {

void
do_ui(dom::context ctx, duplex<string> name)
{
    // clang-format off
/// [expanded-greeting]
dom::do_text(ctx, "What's your name?"); // node a

dom::do_input(ctx, name); // node b

alia_if(name != "") // node c
{
    dom::do_text(ctx, "Hello, " + name + "!"); // node d
}
alia_end

dom::do_text(ctx, "My name is alia."); // node e
/// [expanded-greeting]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, string()));
    });
}

static demo the_demo("expanded-greeting", init_demo);

} // namespace expanded_greeting

namespace hello_button {

/// [hello-button]
void
do_demo(dom::context ctx, duplex<bool> show_message)
{
    dom::do_button(ctx, "Toggle the Message", toggle(show_message));

    alia_if(show_message)
    {
        dom::do_text(ctx, "Hello, World!");
    }
    alia_end
}
/// [hello-button]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_demo(ctx, get_state(ctx, false));
    });
}

static demo the_demo("hello-button", init_demo);

} // namespace hello_button

namespace contrived_hello {

/// [contrived-hello]
struct widget_data
{
    int super_realstic_data_that_my_widgets_need_to_function = 12;
};

void
do_widget(dom::context ctx, std::string name)
{
    widget_data* data;
    get_data(ctx, &data);

    dom::do_text(
        ctx,
        printf(
            ctx, "Hi, I'm widget %s. I live at %p.", value(name), value(data)));
}

void
do_contrived_hello(dom::context ctx)
{
    auto n = get_animation_tick_count(ctx) % 2000;

    do_widget(ctx, "A");

    ALIA_IF(n > 1000)
    {
        do_widget(ctx, "B");
    }
    ALIA_ELSE
    {
        do_widget(ctx, "C");
    }
    ALIA_END

    ALIA_IF(n > 500)
    {
        do_widget(ctx, "D");
    }
    ALIA_END

    do_widget(ctx, "E");
}
/// [contrived-hello]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_contrived_hello(ctx);
    });
}

static demo the_demo("contrived-hello", init_demo);

} // namespace contrived_hello
