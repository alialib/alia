#include <alia/signals/utilities.hpp>

#include <testing.hpp>

#include <alia/signals/basic.hpp>

using namespace alia;

struct empty_regular_signal
    : regular_signal<empty_regular_signal, int, read_only_signal>
{
    bool
    has_value() const
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

TEST_CASE("empty regular_signal", "[signals][utilities]")
{
    empty_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
    REQUIRE(s.value_id() == null_id);
}

struct normal_regular_signal
    : regular_signal<normal_regular_signal, int, readable_duplex_signal>
{
    bool
    has_value() const
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
    ready_to_write() const
    {
        return false;
    }
    void
    write(int) const
    {
    }
};

TEST_CASE("normal regular_signal", "[signals][utilities]")
{
    normal_regular_signal s;

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(s.value_id() == make_id_by_reference(s.read()));
    REQUIRE(!signal_ready_to_write(s));
}

TEST_CASE("signals_all_have_values", "[signals][utilities]")
{
    normal_regular_signal s;
    auto t = value(0);
    empty_regular_signal u;

    REQUIRE(signals_all_have_values());
    REQUIRE(signals_all_have_values(s));
    REQUIRE(signals_all_have_values(s, t));
    REQUIRE(signals_all_have_values(s, t, s));
    REQUIRE(!signals_all_have_values(u));
    REQUIRE(!signals_all_have_values(t, u));
    REQUIRE(!signals_all_have_values(u, s));
    REQUIRE(!signals_all_have_values(s, t, u));
}

template<class Value>
struct preferred_id_test_signal : preferred_id_signal<
                                      preferred_id_test_signal<Value>,
                                      Value,
                                      readable_duplex_signal,
                                      simple_id<std::string>>
{
    bool
    has_value() const
    {
        return true;
    }
    Value const&
    read() const
    {
        static Value value = Value();
        return value;
    }
    bool
    ready_to_write() const
    {
        return false;
    }
    void write(Value) const
    {
    }
    simple_id<std::string>
    complex_value_id() const
    {
        return simple_id<std::string>("a very complex ID");
    }
};

TEST_CASE("simple ID preferring", "[signals][utilities]")
{
    preferred_id_test_signal<int> i;
    int zero = 0;
    REQUIRE(i.value_id() == make_id_by_reference(zero));

    preferred_id_test_signal<std::string> s;
    REQUIRE(s.value_id() == simple_id<std::string>("a very complex ID"));
}

TEST_CASE("refresh_signal_shadow", "[signals][utilities]")
{
    int new_value_count = 0, lost_value_count = 0;

    auto on_new_value = [&](int expected) {
        return [&, expected](int n) {
            REQUIRE(n == expected);
            ++new_value_count;
        };
    };
    auto on_lost_value = [&]() { ++lost_value_count; };

    captured_id id;

    refresh_signal_shadow(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 1);
    REQUIRE(lost_value_count == 0);

    refresh_signal_shadow(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 1);
    REQUIRE(lost_value_count == 0);

    refresh_signal_shadow(id, value(1), on_new_value(1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 0);

    refresh_signal_shadow(id, empty<int>(), on_new_value(-1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 1);

    refresh_signal_shadow(id, empty<int>(), on_new_value(-1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 1);

    refresh_signal_shadow(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 3);
    REQUIRE(lost_value_count == 1);
}
