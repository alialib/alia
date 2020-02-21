#include "demo.hpp"

namespace {

/// [is-prime]
bool
is_prime(int n)
{
    if (n < 2)
        return false;
    int greatest_possible_factor = int(std::round(std::sqrt(n)));
    for (int i = 2; i <= greatest_possible_factor; ++i)
    {
        if (n % i == 0)
            return false;
    }
    return true;
}
/// [is-prime]

} // namespace

void
do_simple_apply(dom::context ctx, bidirectional<int> n)
{
    // clang-format off
/// [simple-apply]
dom::do_text(ctx, "Enter N:");
dom::do_input(ctx, n);
auto n_is_prime = apply(ctx, is_prime, n);
dom::do_text(ctx, conditional(n_is_prime, "N is prime!", "N is NOT prime."));
/// [simple-apply]
    // clang-format on
}

void
init_simple_apply(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_simple_apply(ctx, get_state(ctx, empty<int>()));
    });
}

static demo simple_apply("simple-apply", init_simple_apply);

void
do_transform_demo(dom::context ctx, bidirectional<std::vector<int>> numbers)
{
    // clang-format off
/// [transform-demo]
dom::do_text(ctx, "Enter some numbers:");

// Provide an input box for each number.
for_each(ctx, numbers,
    [ ](auto ctx, auto n) { dom::do_input(ctx, n); });

// Transform the vector of numbers to a vector of bools, indicating whether or
// not each number is prime.
auto prime_flags = transform(ctx, numbers,
    [&](auto ctx, auto n) { return apply(ctx, is_prime, n); });

// Count the number of true values.
auto prime_count = apply(
    ctx,
    [ ](auto flags) { return std::count(flags.begin(), flags.end(), true); },
    prime_flags);

dom::do_text(ctx, printf(ctx, "# of primes: %d", prime_count));

/// [transform-demo]
    // clang-format on
}

void
init_transform_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_transform_demo(ctx, get_state(ctx, std::vector<int>(3, 2)));
    });
}

static demo transform_demo("transform-demo", init_transform_demo);

void
do_metered_transform_demo(
    dom::context ctx, bidirectional<std::vector<int>> numbers)
{
    static int call_count = 0;
    auto counting_is_prime = [&](int n) {
        ++call_count;
        return is_prime(n);
    };

    dom::do_text(ctx, "Enter some numbers:");

    for_each(ctx, numbers, [](auto ctx, auto n) { dom::do_input(ctx, n); });

    auto prime_flags = transform(ctx, numbers, lift(counting_is_prime));

    auto prime_count = apply(
        ctx,
        [&](auto flags) {
            return std::count(flags.begin(), flags.end(), true);
        },
        prime_flags);

    dom::do_text(ctx, printf(ctx, "# of primes: %d", prime_count));

    dom::do_text(
        ctx, printf(ctx, "is_prime has been called %d times.", call_count));
}

void
init_metered_transform_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_metered_transform_demo(ctx, get_state(ctx, std::vector<int>(3, 2)));
    });
}

static demo metered_transform_demo(
    "metered-transform-demo", init_metered_transform_demo);

void
do_direct_counting(dom::context ctx, bidirectional<std::vector<int>> numbers)
{
    // clang-format off
dom::do_text(ctx, "Enter some numbers:");

for_each(ctx, numbers,
    [ ](auto ctx, auto n) { dom::do_input(ctx, n); });

/// [direct-counting]
auto prime_count = apply(
    ctx,
    [ ](auto numbers)
    { return std::count_if(numbers.begin(), numbers.end(), is_prime); },
    numbers);
/// [direct-counting]

dom::do_text(ctx, printf(ctx, "# of primes: %d", prime_count));
    // clang-format on
}

void
init_direct_counting(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_direct_counting(ctx, get_state(ctx, std::vector<int>(3, 2)));
    });
}

static demo direct_counting("direct-counting", init_direct_counting);

void
do_metered_direct_counting(
    dom::context ctx, bidirectional<std::vector<int>> numbers)
{
    static int call_count = 0;
    auto counting_is_prime = [&](int n) {
        ++call_count;
        return is_prime(n);
    };

    dom::do_text(ctx, "Enter some numbers:");

    for_each(ctx, numbers, [](auto ctx, auto n) { dom::do_input(ctx, n); });

    auto prime_count = apply(
        ctx,
        [&](auto numbers) {
            return std::count_if(
                numbers.begin(), numbers.end(), counting_is_prime);
        },
        numbers);

    dom::do_text(ctx, printf(ctx, "# of primes: %d", prime_count));

    dom::do_text(
        ctx, printf(ctx, "is_prime has been called %d times.", call_count));
}

void
init_metered_direct_counting(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_metered_direct_counting(ctx, get_state(ctx, std::vector<int>(3, 2)));
    });
}

static demo metered_direct_counting(
    "metered-direct-counting", init_metered_direct_counting);
