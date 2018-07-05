#include <alia/signals/adaptors.hpp>

#include <type_traits>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>

#include <catch.hpp>

TEST_CASE("fake_readability", "[signals]")
{
    using namespace alia;

    {
        auto s = fake_readability(
            lambda_input([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(!signal_can_write<signal_t>::value);

        REQUIRE(!signal_is_readable(s));
    }

    {
        int x = 0;

        auto s = fake_readability(lambda_inout(
            [&]() { return true; },
            [&]() { return x; },
            [&]() { return true; },
            [&](int v) { x = v; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(signal_can_write<signal_t>::value);

        REQUIRE(s.value_id() == no_id);
        REQUIRE(!signal_is_readable(s));
        REQUIRE(signal_is_writable(s));
        write_signal(s, 1);
        REQUIRE(x == 1);
    }
}

TEST_CASE("fake_writability", "[signals]")
{
    using namespace alia;

    {
        auto s = fake_writability(
            lambda_input([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(signal_can_write<signal_t>::value);

        REQUIRE(signal_is_readable(s));
        REQUIRE(!signal_is_writable(s));
    }

    {
        auto s = fake_writability(lambda_inout(
            [&]() { return true; },
            [&]() { return 0; },
            [&]() { return true; },
            [&](int x) {}));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(signal_can_write<signal_t>::value);

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == 0);
        int x = 0;
        REQUIRE(s.value_id() == make_id_by_reference(x));
        REQUIRE(!signal_is_writable(s));
    }
}

TEST_CASE("signal_cast", "[signals]")
{
    using namespace alia;

    int x = 1;
    auto s = signal_cast<double>(direct(x));

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, double>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 1.0);
    REQUIRE(signal_is_writable(s));
    write_signal(s, 0.0);
    REQUIRE(x == 0);
}

TEST_CASE("is_readable", "[signals]")
{
    using namespace alia;

    bool readable = false;
    int x = 1;

    {
        auto s = is_readable(
            lambda_input([&]() { return readable; }, [&]() { return x; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(!signal_can_write<signal_t>::value);

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == false);
    }

    readable = true;

    {
        // Recreate the signal to circumvent internal caching.
        auto s = is_readable(
            lambda_input([&]() { return readable; }, [&]() { return x; }));

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == true);
    }
}

TEST_CASE("is_writable", "[signals]")
{
    using namespace alia;

    bool writable = false;
    int x = 1;

    {
        auto s = is_writable(lambda_inout(
            always_readable,
            [&]() { return x; },
            [&]() { return writable; },
            [&](int v) { x = v; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(!signal_can_write<signal_t>::value);

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == false);
    }

    writable = true;

    {
        // Recreate the signal to circumvent internal caching.
        auto s = is_writable(lambda_inout(
            always_readable,
            [&]() { return x; },
            [&]() { return writable; },
            [&](int v) { x = v; }));

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == true);
    }
}
