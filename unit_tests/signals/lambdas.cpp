#include <alia/signals/lambdas.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("simple lambda readable signal", "[signals][lambdas]")
{
    auto s = lambda_reader([]() { return 1; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("lambda readable signal", "[signals][lambdas]")
{
    auto s = lambda_reader(always_has_value, []() { return 1; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
}

TEST_CASE("empty lambda readable signal", "[signals][lambdas]")
{
    auto s = lambda_reader([]() { return false; }, []() { return 1; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
}

TEST_CASE("lambda readable signal with ID", "[signals][lambdas]")
{
    auto s = lambda_reader(
        always_has_value, []() { return 1; }, []() { return unit_id; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(s.value_id() == unit_id);
}

TEST_CASE("empty lambda readable signal with ID", "[signals][lambdas]")
{
    auto s = lambda_reader(
        []() { return false; }, []() { return 1; }, []() { return unit_id; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
}

TEST_CASE("lambda bidirectional signal", "[signals][lambdas]")
{
    int x = 1;

    auto s = lambda_bidirectional(
        always_has_value,
        [&x]() { return x; },
        always_ready,
        [&x](int v) { x = v; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(signal_ready_to_write(s));
    captured_id original_id = s.value_id();
    write_signal(s, 0);
    REQUIRE(read_signal(s) == 0);
    REQUIRE(!original_id.matches(s.value_id()));
}

TEST_CASE("empty/unready lambda bidirectional signal", "[signals][lambdas]")
{
    int x = 1;

    auto s = lambda_bidirectional(
        []() { return false; },
        [&x]() { return x; },
        []() { return false; },
        [&x](int v) { x = v; });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
    REQUIRE(!signal_ready_to_write(s));
}

TEST_CASE("lambda bidirectional signal with ID", "[signals][lambdas]")
{
    int x = 1;

    auto s = lambda_bidirectional(
        always_has_value,
        [&x]() { return x; },
        always_ready,
        [&x](int v) { x = v; },
        [&x]() { return make_id(x); });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1);
    REQUIRE(s.value_id() == make_id(1));
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 0);
    REQUIRE(read_signal(s) == 0);
    REQUIRE(s.value_id() == make_id(0));
}

TEST_CASE(
    "empty/unready lambda bidirectional signal with ID", "[signals][lambdas]")
{
    int x = 1;

    auto s = lambda_bidirectional(
        []() { return false; },
        [&x]() { return x; },
        []() { return false; },
        [&x](int v) { x = v; },
        [&x]() { return make_id(x); });

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
    REQUIRE(!signal_ready_to_write(s));
}
