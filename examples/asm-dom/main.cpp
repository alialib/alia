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

void
do_tip_calculator(dom::context ctx)
{
    // Get the state we need.
    auto bill = get_state(ctx, empty<double>()); // defaults to uninitialized
    auto tip_percentage = get_state(ctx, 20.); // defaults to 20%

    // Show some controls for manipulating our state.
    do_number_input(ctx, bill);
    do_number_input(ctx, tip_percentage);

    // Do some reactive calculations.
    auto tip = bill * tip_percentage / 100;
    auto total = bill + tip;

    // Show the results.
    do_text(ctx, printf(ctx, "tip: %.2f", tip));
    do_text(ctx, printf(ctx, "total: %.2f", total));

    // Allow the user to split the bill.
    auto n_people = get_state(ctx, 1);
    do_number_input(ctx, n_people);
    alia_if(n_people > 1)
    {
        do_text(ctx, printf(ctx, "tip per person: %.2f", tip / n_people));
        do_text(ctx, printf(ctx, "total per person: %.2f", total / n_people));
    }
    alia_end
}

int
main()
{
    initialize(the_dom, the_system, "greeting-ui", [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
    return 0;
};
