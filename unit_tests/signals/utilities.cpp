#include <alia/signals/utilities.hpp>

#include <catch.hpp>

struct unreadable_regular_signal
    : alia::regular_signal<int, alia::read_only_signal>
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

TEST_CASE("unreadable regular_signal", "[signals]")
{
    using namespace alia;

    unreadable_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(!signal_is_readable(s));
    REQUIRE(s.value_id() == no_id);
}

struct normal_regular_signal : alia::regular_signal<int, alia::two_way_signal>
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

TEST_CASE("normal regular_signal", "[signals]")
{
    using namespace alia;

    normal_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(s.value_id() == make_id_by_reference(s.read()));
    REQUIRE(!signal_is_writable(s));
}
