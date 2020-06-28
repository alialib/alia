#include "demo.hpp"

namespace addition_ui {

/// [addition-ui]
void
addition_ui(dom::context ctx, duplex<double> a, duplex<double> b)
{
    dom::text(ctx, "Enter two numbers to add:");

    dom::input(ctx, a);
    dom::input(ctx, b);

    dom::text(ctx, a + b);
}
/// [addition-ui]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        addition_ui(
            ctx,
            get_state(ctx, enforce_validity(ctx, empty<double>())),
            get_state(ctx, enforce_validity(ctx, empty<double>())));
    });
}

static demo the_demo("addition-ui", init_demo);

} // namespace addition_ui

namespace numerical_analysis {

void
demo_ui(dom::context ctx)
{
    auto n = enforce_validity(ctx, get_state(ctx, empty<double>()));

    // clang-format off
/// [analysis]
dom::text(ctx, "Enter a number:");

dom::input(ctx, n);

alia_if(n > 0)
{
    dom::text(ctx, "The number is positive!");
}
alia_else_if(n < 0)
{
    dom::text(ctx, "The number is negative!");
}
alia_else
{
    dom::text(ctx, "The number is zero!");
}
alia_end
/// [analysis]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(
        the_dom, the_system, dom_id, [](dom::context ctx) { demo_ui(ctx); });
}

static demo the_demo("numerical-analysis", init_demo);

} // namespace numerical_analysis

namespace tip_calculator {

// clang-format off
/// [tip-calculator]
void
tip_calculator(dom::context ctx)
{
    // Get some component-local state for the bill amount.
    auto bill = alia::get_state(ctx, empty<double>());
    dom::text(ctx, "How much is the bill?");
    // Display an input that allows the user to manipulate our bill state.
    dom::input(ctx, bill);

    // Get some more component-local state for the tip rate.
    auto tip_rate = alia::get_state(ctx, empty<double>());
    dom::text(ctx, "What percentage do you want to tip?");
    // Users like percentages, but we want to keep the 'tip_rate' state as a
    // rate internally, so this input presents a scaled view of it for the user.
    dom::input(ctx, scale(tip_rate, 100));
    // Add a few buttons that set the tip rate to common values.
    dom::button(ctx, "18%", tip_rate <<= 0.18);
    dom::button(ctx, "20%", tip_rate <<= 0.20);
    dom::button(ctx, "25%", tip_rate <<= 0.25);

    // Calculate the results and display them for the user.
    // Note that these operations have dataflow semantics, and since `bill` and
    // `tip_rate` both start out empty, nothing will actually be calculated
    // (or displayed) until the user supplies values for them.
    auto tip = bill * tip_rate;
    auto total = bill + tip;
    dom::text(ctx,
        alia::printf(ctx,
            "You should tip %.2f, for a total of %.2f.", tip, total));

    // Conditionally display a message suggesting cash for small amounts.
    alia_if (total < 10)
    {
        dom::text(ctx,
            "You should consider using cash for small amounts like this.");
    }
    alia_end
}
/// [tip-calculator]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, tip_calculator);
}

static demo the_demo("tip-calculator-demo", init_demo);

} // namespace tip_calculator

namespace factor_tree {

int
factor(int n)
{
    int i;
    for (i = int(std::sqrt(n) + 0.5); i > 1 && n % i != 0; --i)
        ;
    return i;
}

// clang-format off
/// [factor-tree]
void
factor_tree(dom::context ctx, readable<int> n)
{
    dom::scoped_div div(ctx, value("subtree"));

    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = apply(ctx, factor, n);

    // If that factor is 1, n is prime.
    alia_if(f != 1)
    {
        dom::text(ctx, printf(ctx, "%i: composite", n));

        // Allow the user to expand this block to see more factor.
        auto expanded = get_state(ctx, false);
        dom::button(ctx,
            conditional(expanded, "Hide Factors", "Show Factors"),
            toggle(expanded));
        alia_if(expanded)
        {
            factor_tree(ctx, f);
            factor_tree(ctx, n / f);
        }
        alia_end
    }
    alia_else
    {
        dom::text(ctx, printf(ctx, "%i: prime", n));
    }
    alia_end
}

// And here's the demo UI that invokes the top-level of the tree:
void
factor_tree_demo(dom::context ctx, duplex<int> n)
{
    dom::text(ctx, "Enter a number:");
    dom::input(ctx, n);
    factor_tree(ctx, n);
}
/// [factor-tree]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        factor_tree_demo(ctx, enforce_validity(ctx, get_state(ctx, 600)));
    });
}

static demo the_demo("factor-tree", init_demo);

} // namespace factor_tree
