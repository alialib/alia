#include <alia/signals/adaptors.hpp>

#include <map>
#include <type_traits>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>
#include <alia/signals/operators.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("fake_readability", "[signals][adaptors]")
{
    {
        auto s = fake_readability(
            lambda_reader([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(!signal_can_write<signal_t>::value);

        REQUIRE(!signal_is_readable(s));
    }

    {
        int x = 0;

        auto s = fake_readability(lambda_bidirectional(
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

TEST_CASE("fake_writability", "[signals][adaptors]")
{
    {
        auto s = fake_writability(
            lambda_reader([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(signal_can_write<signal_t>::value);

        REQUIRE(signal_is_readable(s));
        REQUIRE(!signal_is_writable(s));
    }

    {
        auto s = fake_writability(lambda_bidirectional(
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

TEST_CASE("signal_cast", "[signals][adaptors]")
{
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

TEST_CASE("is_readable", "[signals][adaptors]")
{
    bool readable = false;
    int x = 1;

    {
        auto s = is_readable(
            lambda_reader([&]() { return readable; }, [&]() { return x; }));

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
            lambda_reader([&]() { return readable; }, [&]() { return x; }));

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == true);
    }
}

TEST_CASE("is_writable", "[signals][adaptors]")
{
    bool writable = false;
    int x = 1;

    {
        auto s = is_writable(lambda_bidirectional(
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
        auto s = is_writable(lambda_bidirectional(
            always_readable,
            [&]() { return x; },
            [&]() { return writable; },
            [&](int v) { x = v; }));

        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == true);
    }
}

TEST_CASE("add_fallback", "[signals][adaptors]")
{
    int p = 1;
    int f = 0;
    auto make_fallback = [&](bool primary_readable,
                             bool primary_writable,
                             bool fallback_readable) {
        return add_fallback(
            lambda_bidirectional(
                [=]() { return primary_readable; },
                [=]() { return p; },
                [=]() { return primary_writable; },
                [&p](int x) { p = x; }),
            lambda_bidirectional(
                [=]() { return fallback_readable; },
                [=]() { return f; },
                always_writable,
                [&f](int x) { f = x; }));
    };

    {
        typedef decltype(make_fallback(true, true, true)) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(signal_can_write<signal_t>::value);
    }

    {
        typedef decltype(add_fallback(value(0), value(1))) signal_t;
        REQUIRE(signal_can_read<signal_t>::value);
        REQUIRE(!signal_can_write<signal_t>::value);
    }

    {
        p = 1;
        auto s = make_fallback(true, true, true);
        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(signal_is_writable(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_fallback(false, true, true);
        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == 0);
        REQUIRE(signal_is_writable(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_fallback(false, true, false);
        REQUIRE(!signal_is_readable(s));
        REQUIRE(signal_is_writable(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_fallback(true, false, false);
        REQUIRE(signal_is_readable(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(!signal_is_writable(s));
    }

    {
        // Check that the overall signal produces a different value ID when
        // using the primary vs the fallback, even when the two component
        // signals have the same value ID.
        p = 0;
        auto s = make_fallback(true, false, false);
        auto t = make_fallback(false, false, true);
        REQUIRE(signal_is_readable(s));
        REQUIRE(signal_is_readable(t));
        REQUIRE(read_signal(s) == read_signal(t));
        REQUIRE(s.value_id() != t.value_id());
    }
}

TEST_CASE("simplify_id", "[signals][adaptors]")
{
    using namespace alia;

    auto c = std::map<int, std::string>{{2, "1"}, {0, "3"}};
    auto c_signal = direct(c);
    auto unwrapped = c_signal[value(2)];
    auto s = simplify_id(unwrapped);

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, std::string>::value));
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(s.value_id() != unwrapped.value_id());
    REQUIRE(s.value_id() == make_id_by_reference(c[2]));
    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == "1");
    REQUIRE(signal_is_writable(s));
    write_signal(s, "7");
    REQUIRE((c == std::map<int, std::string>{{2, "7"}, {0, "3"}}));
}

TEST_CASE("signalize a signal", "[signals][adaptors]")
{
    int x = 12;
    auto s = direct(x);
    auto t = signalize(s);
    REQUIRE(signal_is_readable(t));
    REQUIRE(read_signal(t) == 12);
}

TEST_CASE("signalize a value", "[signals][adaptors]")
{
    int x = 12;
    auto t = signalize(x);
    REQUIRE(signal_is_readable(t));
    REQUIRE(read_signal(t) == 12);
}
