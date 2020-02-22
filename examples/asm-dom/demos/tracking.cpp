#include "demo.hpp"

namespace switch_example {

void
do_ui(dom::context ctx, bidirectional<int> n)
{
    // clang-format off
/// [switch-example]
do_text(ctx, "Enter a number:");
do_input(ctx, n);
alia_switch(n)
{
 alia_case(0):
    do_text(ctx, "foo");
    break;
 alia_case(1):
    do_text(ctx, "bar");
 alia_case(2):
 alia_case(3):
    do_text(ctx, "baz");
    break;
 alia_default:
    do_text(ctx, "zub");
}
alia_end
/// [switch-example]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_ui(ctx, get_state(ctx, empty<int>()));
    });
}

static demo the_demo("switch-example", init_demo);

} // namespace switch_example

namespace loop_macros_demo {

// clang-format off
/// [loop-macros-demo]

struct my_record
{
    std::string label;
    int x = 0, y = 0;
};

bool
in_bounds(my_record const& r)
{
    return r.x >= 0 && r.x < 100 && r.y >= -20 && r.y < 20;
}

void
do_records_ui(dom::context ctx, std::vector<my_record>& records)
{
    // Loop through our records and present a UI for each of them...
    // Since we're trying to integrate our application's data structure with the
    // least possible effort, we'll just loop through them as we would in a
    // normal C++ for loop but using alia_for instead, since it provides
    // tracking.
    alia_for(auto& record : records)
    {
        // And now, at the point that we actually connect our individual records
        // to our widgets, we'll just use 'direct' to create signals that
        // directly connect our record's fields to the widgets.
        do_heading(ctx, "h4", direct(record.label));
        do_input(ctx, direct(record.x));
        do_input(ctx, direct(record.y));

        // We might also apply some tests to our records without worrying about
        // alia's 'proper', signal-based dataflow semantics...
        alia_if(!in_bounds(record))
        {
            do_text(ctx, "WARNING: This is out of bounds!");
        }
        alia_end

        // And, since we're doing this example as a toy without real
        // consideration for styling, let's throw in a horizontal rule for
        // separation between records...
        do_hr(ctx);
    }
    alia_end

    // Also present a little UI for adding new records...
    {
        // Get some local state for the new label.
        auto new_label = get_state(ctx, string());
        do_input(ctx, new_label);

        // Create an action that adds the new record.
        auto add_record = lambda_action(
            [&](std::string label) { records.push_back({label}); });

        // Present a button that adds the new record.
        do_button(ctx, "Add",
            // Hook up the new label to our action (if the label isn't empty).
            (add_record <<= mask(new_label, new_label != ""),
             // Also add in an action that resets the label.
             new_label <<= ""));
    }
}

/// [loop-macros-demo]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    static std::vector<my_record> the_records = {{"demo record", 2, 4}};

    initialize(the_dom, the_system, dom_id, [&](dom::context ctx) {
        do_records_ui(ctx, the_records);
    });
}

static demo the_demo("loop-macros-demo", init_demo);

} // namespace loop_macros_demo
