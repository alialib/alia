#include "demo.hpp"

namespace hello_world {

/// [greeting]
void
greeting_ui(dom::context ctx, duplex<string> name)
{
    dom::text(ctx, "What's your name?");

    // Allow the user to input their name.
    dom::input(ctx, name);

    // If we have a name, greet the user.
    alia_if(name != "")
    {
        dom::text(ctx, "Hello, " + name + "!");
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
        greeting_ui(ctx, get_state(ctx, string()));
    });
}

static demo the_demo("greeting-ui", init_demo);

} // namespace hello_world

namespace expanded_greeting {

void
demo_ui(dom::context ctx, duplex<string> name)
{
    // clang-format off
/// [expanded-greeting]
dom::text(ctx, "What's your name?"); // node a

dom::input(ctx, name); // node b

alia_if(name != "") // node c
{
    dom::text(ctx, "Hello, " + name + "!"); // node d
}
alia_end

dom::text(ctx, "My name is alia."); // node e
/// [expanded-greeting]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        demo_ui(ctx, get_state(ctx, string()));
    });
}

static demo the_demo("expanded-greeting", init_demo);

} // namespace expanded_greeting

namespace hello_button {

/// [hello-button]
void
demo_ui(dom::context ctx, duplex<bool> show_message)
{
    dom::button(ctx, "Toggle the Message", toggle(show_message));

    alia_if(show_message)
    {
        dom::text(ctx, "Hello, World!");
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
widget(dom::context ctx, std::string name)
{
    widget_data* data;
    get_data(ctx, &data);

    dom::text(
        ctx,
        alia::printf(
            ctx, "Hi, I'm widget %s. I live at %p.", value(name), value(data)));
}

void
contrived_hello(dom::context ctx)
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
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        contrived_hello(ctx);
    });
}

static demo the_demo("contrived-hello", init_demo);

} // namespace contrived_hello
