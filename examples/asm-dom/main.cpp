#include "asm-dom.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/val.h>

#include <functional>
#include <iostream>
#include <string>

#define ALIA_IMPLEMENTATION
#define ALIA_LOWERCASE_MACROS
#include "alia.hpp"

#include "color.hpp"
#include "dom.hpp"

using std::string;

using namespace alia;
using namespace alia::literals;

using namespace dom;

alia::system the_system;
dom::system the_dom;

void
do_ui(dom::context ctx)
{
    auto color = get_state(ctx, value(rgb8(0, 0, 0)));

    do_button(ctx, "black"_a, color <<= value(rgb8(50, 50, 55)));
    do_button(ctx, "white"_a, color <<= value(rgb8(230, 230, 255)));

    do_colored_box(ctx, smooth_value(ctx, color));
}

int
main()
{
    initialize(the_dom, the_system, "greeting-ui", [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
    return 0;
};
