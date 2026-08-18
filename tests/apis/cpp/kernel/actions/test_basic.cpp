#include <alia/kernel/actions/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("unready action")
{
    CHECK_FALSE(actions::unready().is_ready());
    CHECK_FALSE(actions::unready<int>().is_ready());
}

TEST_CASE("noop action")
{
    CHECK(actions::noop().is_ready());
    perform_action(actions::noop());
    CHECK(actions::noop<int>().is_ready());
    perform_action(actions::noop<int>(), 1);
}

TEST_CASE("callbacks")
{
    int x = 0;
    auto a = callback([&](int y, int z) { x = y + z; });
    perform_action(a, 1, 2);
    CHECK(x == 3);

    bool ready = false;
    auto b = callback([&]() { return ready; }, [&](int y) { x += y; });
    CHECK_FALSE(b.is_ready());
    ready = true;
    CHECK(b.is_ready());
    perform_action(b, 1);
    CHECK(x == 4);
}
