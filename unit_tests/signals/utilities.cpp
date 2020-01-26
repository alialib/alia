#include <alia/signals/utilities.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>

using namespace alia;

struct unreadable_regular_signal
    : regular_signal<unreadable_regular_signal, int, read_only_signal>
{
    bool
    is_readable() const
    {
        return false;
    }
    int const&
    read() const
    {
        static int value = 17;
        return value;
    }
};

TEST_CASE("unreadable regular_signal", "[signals][utilities]")
{
    unreadable_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(!signal_is_readable(s));
    REQUIRE(s.value_id() == no_id);
}

struct normal_regular_signal
    : regular_signal<normal_regular_signal, int, bidirectional_signal>
{
    bool
    is_readable() const
    {
        return true;
    }
    int const&
    read() const
    {
        static int value = 17;
        return value;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(int const& value) const
    {
    }
};

TEST_CASE("normal regular_signal", "[signals][utilities]")
{
    normal_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(s.value_id() == make_id_by_reference(s.read()));
    REQUIRE(!signal_is_writable(s));
}

TEST_CASE("lazy_reader", "[signals][utilities]")
{
    int n = 0;

    auto generator = [&]() {
        ++n;
        return 12;
    };

    lazy_reader<int> reader;

    REQUIRE(reader.read(generator) == 12);
    REQUIRE(n == 1);
    // Check that it is caching and not re-invoking the generator.
    REQUIRE(reader.read(generator) == 12);
    REQUIRE(n == 1);
}

TEST_CASE("signals_all_readable", "[signals][utilities]")
{
    normal_regular_signal s;
    auto t = value(0);
    unreadable_regular_signal u;

    REQUIRE(signals_all_readable());
    REQUIRE(signals_all_readable(s));
    REQUIRE(signals_all_readable(s, t));
    REQUIRE(signals_all_readable(s, t, s));
    REQUIRE(!signals_all_readable(u));
    REQUIRE(!signals_all_readable(t, u));
    REQUIRE(!signals_all_readable(u, s));
    REQUIRE(!signals_all_readable(s, t, u));
}
