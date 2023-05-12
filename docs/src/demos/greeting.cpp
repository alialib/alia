#include "demo.hpp"

namespace hello_world {

/// [greeting]
void
greeting_ui(html::context ctx, duplex<std::string> name)
{
    html::p(ctx, "What's your name?");

    // Allow the user to input their name.
    html::input(ctx, name);

    // If we have a name, greet the user.
    alia_if(name != "")
    {
        html::p(ctx, "Hello, " + name + "!");
    }
    alia_end
}
/// [greeting]

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        greeting_ui(ctx, get_state(ctx, std::string()));
    });
}

static demo the_demo("greeting-ui", init_demo);

} // namespace hello_world

namespace expanded_greeting {

void
demo_ui(html::context ctx, duplex<std::string> name)
{
    // clang-format off
/// [expanded-greeting]
html::p(ctx, "What's your name?"); // node a

html::input(ctx, name); // node b

alia_if(name != "") // node c
{
    html::p(ctx, "Hello, " + name + "!"); // node d
}
alia_end

html::p(ctx, "My name is alia."); // node e
/// [expanded-greeting]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, get_state(ctx, std::string()));
    });
}

static demo the_demo("expanded-greeting", init_demo);

} // namespace expanded_greeting

namespace hello_button {

/// [hello-button]
void
demo_ui(html::context ctx, duplex<bool> show_message)
{
    html::button(ctx, "Toggle the Message", actions::toggle(show_message));

    alia_if(show_message)
    {
        html::p(ctx, "Hello, World!");
    }
    alia_end
}
/// [hello-button]

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        demo_ui(ctx, get_state(ctx, false));
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
widget(html::context ctx, std::string name)
{
    widget_data* data;
    get_data(ctx, &data);

    html::p(
        ctx,
        alia::printf(
            ctx,
            "Hi, I'm widget %s. I live at %p.",
            value(name),
            value(data)));
}

void
contrived_hello(html::context ctx)
{
    auto n = get_animation_tick_count(ctx) % 2000;

    widget(ctx, "A");

    ALIA_IF(n > 1000)
    {
        widget(ctx, "B");
    }
    ALIA_ELSE
    {
        widget(ctx, "C");
    }
    ALIA_END

    ALIA_IF(n > 500)
    {
        widget(ctx, "D");
    }
    ALIA_END

    widget(ctx, "E");
}
/// [contrived-hello]

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(
        the_system, dom_id, [](html::context ctx) { contrived_hello(ctx); });
}

static demo the_demo("contrived-hello", init_demo);

} // namespace contrived_hello
