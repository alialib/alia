#define ALIA_IMPLEMENTATION
#define ALIA_LOWERCASE_MACROS
#include "alia.hpp"

#include "dom.hpp"

using namespace alia;
using namespace dom;

void
do_greeting_ui(dom::context ctx, bidirectional<string> name)
{
    // Allow the user to input their name.
    do_input(ctx, name);

    // Greet the user.
    alia_if(name != "")
    {
        do_text(ctx, "Hello, " + name + "!");
    }
    alia_end
}

void
init_greeting_ui()
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, "greeting-ui", [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
}

void
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    do_input(ctx, a);
    do_input(ctx, b);
    do_text(ctx, a + b);
}

void
init_addition_ui()
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, "addition-ui", [](dom::context ctx) {
        do_addition_ui(
            ctx,
            get_state(ctx, empty<double>()),
            get_state(ctx, empty<double>()));
    });
}

int
main()
{
    init_greeting_ui();
    init_addition_ui();
    return 0;
};
