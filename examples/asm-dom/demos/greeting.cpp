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

void
do_expanded_greeting(dom::context ctx, bidirectional<string> name)
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
    // clang-format-on
}

void
init_expanded_greeting(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_expanded_greeting(ctx, get_state(ctx, string()));
    });
}

static demo expanded_greeting("expanded-greeting", init_expanded_greeting);

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
            ctx,
            "Hi, I'm a widget. My name is %s, and I live at %p.",
            value(name),
            value(data)));
}

void
do_contrived_hello(dom::context ctx)
{
    auto n = get_animation_tick_count(ctx) % 2000;

    do_widget(ctx, "Tom");

    ALIA_IF(n > 1000)
    {
        do_widget(ctx, "Dick");
    }
    ALIA_ELSE
    {
        do_widget(ctx, "Maria");
    }
    ALIA_END

    ALIA_IF(n > 500)
    {
        do_widget(ctx, "Harry");
    }
    ALIA_END

    do_widget(ctx, "Maria");
}
/// [contrived-hello]

void
init_contrived_hello(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_contrived_hello(ctx);
    });
}

static demo contrived_hello("contrived-hello", init_contrived_hello);
