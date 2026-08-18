#include <alia/kernel/actions/adaptors.hpp>

#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("actionize an action")
{
    int x = 0;
    auto a = actionize(ref(x) <<= 1);
    CHECK(a.is_ready());
    perform_action(a);
    CHECK(x == 1);
}

TEST_CASE("actionize a lambda")
{
    int x = 0;
    auto a = actionize([&] { x = 1; });
    CHECK(a.is_ready());
    perform_action(a);
    CHECK(x == 1);
}

TEST_CASE("only_if_ready on an unready input action")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    int x = 0;
    auto b = ref(x) <<= empty<int>();
    CHECK_FALSE(b.is_ready());

    auto combined = (a, only_if_ready(b));
    CHECK(combined.is_ready());
    perform_action(combined);
    CHECK(a_ran);
}

TEST_CASE("only_if_ready on a ready input action")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    int x = 0;
    auto b = ref(x) <<= value(1);
    CHECK(b.is_ready());

    auto combined = (a, only_if_ready(b));
    CHECK(combined.is_ready());
    perform_action(combined);
    CHECK(a_ran);
    CHECK(x == 1);
}

TEST_CASE("mask an action")
{
    bool a_ran = false;
    auto a = callback([&] { a_ran = true; });

    auto masked_off = mask(a, false);
    CHECK_FALSE(masked_off.is_ready());

    auto masked_on = mask(a, value(true));
    CHECK(masked_on.is_ready());
    perform_action(masked_on);
    CHECK(a_ran);
}
