#define ALIA_IMPLEMENTATION
#define ALIA_LOWERCASE_MACROS
#include "alia.hpp"

#include "dom.hpp"

using namespace alia;
using namespace dom;

alia::system greeting_system;
dom::system greeting_dom;

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

alia::system addition_system;
dom::system addition_dom;

void
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    do_input(ctx, a);
    do_input(ctx, b);
    do_text(ctx, a + b);
}

int
main()
{
    initialize(
        addition_dom, addition_system, "addition-ui", [](dom::context ctx) {
            do_addition_ui(
                ctx,
                get_state(ctx, empty<double>()),
                get_state(ctx, empty<double>()));
        });

    initialize(
        greeting_dom, greeting_system, "greeting-ui", [](dom::context ctx) {
            do_greeting_ui(ctx, get_state(ctx, string()));
        });

    return 0;
};
