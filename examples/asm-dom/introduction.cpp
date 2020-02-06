#define ALIA_IMPLEMENTATION
#define ALIA_LOWERCASE_MACROS
#include "alia.hpp"

#include "dom.hpp"

using namespace alia;
using namespace dom;

#include "snippets/greeting_ui.cpp"

void
init_greeting_ui()
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, "greeting-ui", [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
}

#include "snippets/addition_ui.cpp"

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

#include "snippets/addition_analysis.cpp"

void
init_addition_analysis()
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, "addition-analysis", [](dom::context ctx) {
        do_addition_analysis(
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
    init_addition_analysis();
    return 0;
};
