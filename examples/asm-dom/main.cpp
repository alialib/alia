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
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    do_input(ctx, a);
    do_input(ctx, b);
    do_text(ctx, a + b);
}

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
do_ui(dom::context ctx)
{
    auto color = get_state(ctx, val(rgb8(0, 0, 0)));

    do_button(ctx, "black"_a, color <<= val(rgb8(50, 50, 55)));
    do_button(ctx, "white"_a, color <<= val(rgb8(230, 230, 255)));

    do_colored_box(ctx, smooth_value(ctx, color));
}

void
do_tip_calculator(dom::context ctx)
{
    // Get the state we need.
    auto bill = get_state(ctx, empty<double>()); // defaults to uninitialized
    auto tip_percentage = get_state(ctx, val(20.)); // defaults to 20%

    // Show some controls for manipulating our state.
    // std::cout << "do bill" << std::endl;
    do_number_input(ctx, bill);
    // std::cout << "do tip" << std::endl;
    do_number_input(ctx, tip_percentage);

    // Do some reactive calculations.
    auto tip = bill * tip_percentage / 100;
    auto total = bill + tip;

    // Show the results.
    do_text(ctx, printf(ctx, "tip: %.2f", tip));
    do_text(ctx, printf(ctx, "total: %.2f", total));

    // Allow the user to split the bill.
    auto n_people = get_state(ctx, val(1.));
    do_number_input(ctx, n_people);
    alia_if(n_people > val(1))
    {
        do_text(ctx, printf(ctx, "tip per person: %.2f", tip / n_people));
        do_text(ctx, printf(ctx, "total per person: %.2f", total / n_people));
    }
    alia_end
}

int
main()
{
    initialize(the_dom, the_system, "root", do_ui);

    return 0;
};
