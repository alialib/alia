#include <alia/abi/prelude.h>

#include <doctest/doctest.h>

// Probe type for the operator macros. Each function records which operation
// ran, so a macro wired to the wrong function fails even though it compiles.
struct probe
{
    int op;
    int a;
    int b;
};

probe
probe_add(probe x, probe y)
{
    return probe{1, x.a, y.a};
}

probe
probe_sub(probe x, probe y)
{
    return probe{2, x.a, y.a};
}

void
probe_add_inplace(probe* x, probe y)
{
    x->op = 1;
    x->b = y.a;
}

void
probe_sub_inplace(probe* x, probe y)
{
    x->op = 2;
    x->b = y.a;
}

probe
probe_scale(probe x, int s)
{
    return probe{3, x.a, s};
}

void
probe_scale_inplace(probe* x, int s)
{
    x->op = 3;
    x->b = s;
}

extern "C" {
ALIA_DEFINE_PLUS_OPERATOR(probe)
ALIA_DEFINE_MINUS_OPERATOR(probe)
ALIA_DEFINE_SCALE_OPERATOR(probe, int)
}

using namespace alia::operators;

TEST_CASE("operator macros call the matching functions")
{
    probe a{0, 10, 0};
    probe b{0, 4, 0};

    probe sum = a + b;
    CHECK(sum.op == 1);
    CHECK(sum.a == 10);
    CHECK(sum.b == 4);

    probe diff = a - b;
    CHECK(diff.op == 2);
    CHECK(diff.a == 10);
    CHECK(diff.b == 4);

    probe scaled = a * 5;
    CHECK(scaled.op == 3);
    CHECK(scaled.a == 10);
    CHECK(scaled.b == 5);

    probe scaled_left = 7 * a;
    CHECK(scaled_left.op == 3);
    CHECK(scaled_left.a == 10);
    CHECK(scaled_left.b == 7);

    a += b;
    CHECK(a.op == 1);
    CHECK(a.b == 4);

    a -= b;
    CHECK(a.op == 2);
    CHECK(a.b == 4);

    a *= 9;
    CHECK(a.op == 3);
    CHECK(a.b == 9);
}
