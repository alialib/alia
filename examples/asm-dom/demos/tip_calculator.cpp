#include "demo.hpp"

void
do_tip_calculator(dom::context ctx)
{
    // Get the state we need.
    auto bill = get_state(ctx, empty<double>()); // defaults to uninitialized
    auto tip_percentage = get_state(ctx, 20.); // defaults to 20%

    // Show some controls for manipulating our state.
    dom::do_number_input(ctx, bill);
    dom::do_number_input(ctx, tip_percentage);

    // Do some reactive calculations.
    auto tip = bill * tip_percentage / 100;
    auto total = bill + tip;

    // Show the results.
    dom::do_text(ctx, printf(ctx, "tip: %.2f", tip));
    dom::do_text(ctx, printf(ctx, "total: %.2f", total));

    // Allow the user to split the bill.
    auto n_people = get_state(ctx, 1);
    do_number_input(ctx, n_people);
    alia_if(n_people > 1)
    {
        dom::do_text(ctx, printf(ctx, "tip per person: %.2f", tip / n_people));
        dom::do_text(
            ctx, printf(ctx, "total per person: %.2f", total / n_people));
    }
    alia_end
}
