#include "demo.hpp"

#include <random>

namespace switch_example {

void
demo_ui(dom::context ctx, duplex<int> n)
{
    // clang-format off
/// [switch-example]
dom::text(ctx, "Enter a number:");
dom::input(ctx, n);
alia_switch(n)
{
 alia_case(0):
    dom::text(ctx, "foo");
    break;
 alia_case(1):
    dom::text(ctx, "bar");
 alia_case(2):
 alia_case(3):
    dom::text(ctx, "baz");
    break;
 alia_default:
    dom::text(ctx, "zub");
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
        demo_ui(ctx, enforce_validity(ctx, get_state(ctx, empty<int>())));
    });
}

static demo the_demo("switch-example", init_demo);

} // namespace switch_example

namespace loop_macros_demo {

// clang-format off
/// [loop-macros-demo]
// Here's an application data type, which has nothing to do with alia.
struct my_record
{
    std::string label;
    int x = 0, y = 0;
};

// Here's a function that operates on that type and also has nothing to do with
// alia.
bool
in_bounds(my_record const& r)
{
    return r.x >= 0 && r.x < 100 && r.y >= -20 && r.y < 20;
}

// Here's an alia function that exposes our application data via asm-dom.
void
records_ui(dom::context ctx, std::vector<my_record>& records)
{
    // Loop through our records and present a UI for each of them...
    // Since we're trying to integrate our application's data structure with the
    // least possible effort, we'll just loop through them as we would in a
    // normal C++ for loop but using alia_for instead, since it provides
    // tracking.
    alia_for(auto& record : records)
    {
        dom::scoped_div div(ctx, value("item"));

        // And now, at the point that we actually connect our individual records
        // to our widgets, we'll just use 'direct' to create signals that
        // directly connect our record's fields to the widgets.
        dom::element(ctx, "h4").text(direct(record.label));
        dom::input(ctx, direct(record.x));
        dom::input(ctx, direct(record.y));

        // Display a warning if the record fails our in_bounds check.
        // Note that although alia provides signal-based dataflow mechanics for
        // doing this test, we can also just call the function like we would in
        // normal C++ code...
        alia_if(!in_bounds(record))
        {
            // Apparently the Docsify CSS class for a warning is 'tip'.
            dom::scoped_div div(ctx, value("tip"));
            dom::text(ctx, "This is out of bounds!");
        }
        alia_end
    }
    alia_end

    // Also present a little UI for adding new records...
    {
        // Get some local state for the new label.
        auto new_label = alia::get_state(ctx, string());
        dom::input(ctx, new_label);

        // Create an action that adds the new record.
        auto add_record = callback(
            [&](std::string label) { records.push_back({label}); });

        // Present a button that adds the new record.
        dom::button(ctx, "Add",
            // Hook up the new label to our action (if the label isn't empty).
            (add_record << alia::mask(new_label, new_label != ""),
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

    static std::vector<my_record> the_records
        = {{"Demo Record", 2, 4}, {"Fix Me!", 200, 0}};

    initialize(the_dom, the_system, dom_id, [&](dom::context ctx) {
        records_ui(ctx, the_records);
    });
}

static demo the_demo("loop-macros-demo", init_demo);
} // namespace loop_macros_demo

namespace for_each_map_demo {

// clang-format off
/// [for-each-map-demo]
void
scoreboard(dom::context ctx, duplex<std::map<std::string, int>> scores)
{
    alia::for_each(ctx, scores,
        [](auto ctx, auto player, auto score) {
            dom::scoped_div div(ctx, value("item"));
            dom::element(ctx, "h4").text(player);
            dom::text(ctx, alia::printf(ctx, "%d points", score));
            dom::button(ctx, "GOAL!", ++score);
        });

    auto new_player = alia::get_state(ctx, string());
    dom::input(ctx, new_player);
    dom::button(ctx, "Add Player",
        (scores[new_player] <<= alia::mask(0, new_player != ""),
         new_player <<= ""));
}
/// [for-each-map-demo]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    static std::map<std::string, int> the_scores = {{"Karen", 5}, {"Tom", 2}};

    initialize(the_dom, the_system, dom_id, [&](dom::context ctx) {
        scoreboard(ctx, direct(the_scores));
    });
}

static demo the_demo("for-each-map-demo", init_demo);

} // namespace for_each_map_demo

namespace for_each_vector_demo {

// clang-format off
/// [for-each-vector-demo]
struct player
{
    std::string name;
    int score;
};

void
scoreboard(dom::context ctx, duplex<std::vector<player>> players)
{
    alia::for_each(ctx, players,
        [](auto ctx, auto player) {
            dom::scoped_div div(ctx, value("item"));
            dom::element(ctx, "h4").text(alia_field(player, name));
            dom::text(ctx,
                alia::printf(ctx, "%d points", alia_field(player, score)));
            dom::button(ctx, "GOAL!", ++alia_field(player, score));
        });

    auto new_player = alia::get_state(ctx, string());
    dom::input(ctx, new_player);
    dom::button(ctx, "Add Player",
        (alia::push_back(players) <<
            alia::apply(ctx,
                [](auto name) { return player{name, 0}; },
                alia::mask(new_player, new_player != "")),
         new_player <<= ""));
}
/// [for-each-vector-demo]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [&](dom::context ctx) {
        scoreboard(ctx,
            get_state(ctx,
                lambda_constant(
                    []() {
                        return std::vector<player>{{"Karen", 5}, {"Tom", 2}};
                    })));
    });
}
// clang-format on

static demo the_demo("for-each-vector-demo", init_demo);

} // namespace for_each_vector_demo

namespace named_blocks_demo {

static std::default_random_engine rng;

// clang-format off
/// [named-blocks-demo]
struct my_record
{
    std::string id, label;
    int x = 0, y = 0;
};

void
records_ui(dom::context ctx, std::vector<my_record>& records)
{
    naming_context nc(ctx);
    for(auto& record : records)
    {
        named_block nb(nc, make_id(record.id));

        // Do the controls for this record, like we normally would...

        dom::scoped_div div(ctx, value("item"));

        dom::element(ctx, "h4").text(direct(record.label));
        dom::input(ctx, direct(record.x));
        dom::input(ctx, direct(record.y));

        // Just to demonstrate that each record is associated with the same data
        // block, we'll get some local state here. Feel free to type something
        // in here and shuffle the records to see what happens...
        dom::text(ctx, "Local UI state associated with this record:");
        dom::input(ctx, alia::get_state(ctx, ""));
    }

    dom::button(ctx, "Shuffle!",
        callback(
            [&]() { std::shuffle(records.begin(), records.end(), rng); }));

}
/// [named-blocks-demo]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    static std::vector<my_record> the_records
        = {{"abc", "ABC", 2, 4},
           {"def", "DEF", 5, 1},
           {"ghi", "GHI", 1, 0},
           {"jkl", "JKL", -1, 3}};

    initialize(the_dom, the_system, dom_id, [&](dom::context ctx) {
        records_ui(ctx, the_records);
    });
}

static demo the_demo("named-blocks-demo", init_demo);
} // namespace named_blocks_demo
