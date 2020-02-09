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

int
main()
{
    init_greeting_ui();
    return 0;
};
