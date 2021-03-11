#include "demo.hpp"

namespace addition_ui {

/// [addition-ui]
void
addition_ui(html::context ctx, duplex<double> a, duplex<double> b)
{
    html::p(ctx, "Enter two numbers to add:");

    html::input(ctx, a);
    html::input(ctx, b);

    html::p(ctx, a + b);
}
/// [addition-ui]

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        addition_ui(
            ctx,
            enforce_validity(ctx, get_state(ctx, empty<double>())),
            enforce_validity(ctx, get_state(ctx, empty<double>())));
    });
}

static demo the_demo("addition-ui", init_demo);

} // namespace addition_ui

namespace numerical_analysis {

void
demo_ui(html::context ctx)
{
    auto n = enforce_validity(ctx, get_state(ctx, empty<double>()));

    // clang-format off
/// [analysis]
html::p(ctx, "Enter a number:");

html::input(ctx, n);

alia_if(n > 0)
{
    html::p(ctx, "The number is positive!");
}
alia_else_if(n < 0)
{
    html::p(ctx, "The number is negative!");
}
alia_else
{
    html::p(ctx, "The number is zero!");
}
alia_end
/// [analysis]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("numerical-analysis", init_demo);

} // namespace numerical_analysis

namespace tip_calculator {

// clang-format off
/// [tip-calculator]
void
tip_calculator(html::context ctx)
{
    // Get some component-local state for the bill amount.
    auto bill = get_state(ctx, empty<double>());
    p(ctx, "How much is the bill?");
    // Display an input that allows the user to manipulate our bill state.
    input(ctx, bill);

    // Get some more component-local state for the tip rate.
    auto tip_rate = get_state(ctx, empty<double>());
    p(ctx, "What percentage do you want to tip?");
    // Users like percentages, but we want to keep the 'tip_rate' state as a
    // rate internally, so this input presents a scaled view of it for the user.
    input(ctx, scale(tip_rate, 100));
    // Add a few buttons that set the tip rate to common values.
    button(ctx, "18%", tip_rate <<= 0.18);
    button(ctx, "20%", tip_rate <<= 0.20);
    button(ctx, "25%", tip_rate <<= 0.25);

    // Calculate the results and display them for the user.
    // Note that these operations have dataflow semantics, and since `bill` and
    // `tip_rate` both start out empty, nothing will actually be calculated
    // (or displayed) until the user supplies values for them.
    auto tip = bill * tip_rate;
    auto total = bill + tip;
    p(ctx,
        printf(ctx,
            "You should tip %.2f, for a total of %.2f.", tip, total));

    // Conditionally display a message suggesting cash for small amounts.
    alia_if (total < 10)
    {
        p(ctx,
            "You should consider using cash for small amounts like this.");
    }
    alia_end
}
/// [tip-calculator]
// clang-format on

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, tip_calculator);
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
factor_tree(html::context ctx, readable<int> n)
{
    html::scoped_div div(ctx, "subtree");

    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = alia::apply(ctx, factor, n);

    // If that factor is 1, n is prime.
    alia_if(f != 1)
    {
        html::p(ctx, alia::printf(ctx, "%i: composite", n));

        // Allow the user to expand this block to see more factor.
        auto expanded = get_state(ctx, false);
        html::button(ctx,
            conditional(expanded, "Hide Factors", "Show Factors"),
            actions::toggle(expanded));
        alia_if(expanded)
        {
            factor_tree(ctx, f);
            factor_tree(ctx, n / f);
        }
        alia_end
    }
    alia_else
    {
        html::p(ctx, alia::printf(ctx, "%i: prime", n));
    }
    alia_end
}

// And here's the demo UI that invokes the top-level of the tree:
void
factor_tree_demo(html::context ctx, duplex<int> n)
{
    html::p(ctx, "Enter a number:");
    html::input(ctx, n);
    factor_tree(ctx, n);
}
/// [factor-tree]
// clang-format on

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        factor_tree_demo(ctx, enforce_validity(ctx, get_state(ctx, 600)));
    });
}

static demo the_demo("factor-tree", init_demo);

} // namespace factor_tree
