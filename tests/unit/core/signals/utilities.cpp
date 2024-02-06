#include <alia/core/signals/utilities.hpp>

#include <catch2/catch_test_macros.hpp>

#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/state.hpp>

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
    id_interface const&
    write(int) const
    {
        return null_id;
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
    id_interface const&
    write(Value) const
    {
        return null_id;
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

TEST_CASE("refresh_signal_view", "[signals][utilities]")
{
    // 2-lambda version...

    int new_value_count = 0, lost_value_count = 0;

    auto on_new_value = [&](int expected) {
        return [&, expected](int n) {
            REQUIRE(n == expected);
            ++new_value_count;
        };
    };
    auto on_lost_value = [&]() { ++lost_value_count; };

    captured_id id;

    refresh_signal_view(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 1);
    REQUIRE(lost_value_count == 0);

    refresh_signal_view(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 1);
    REQUIRE(lost_value_count == 0);

    refresh_signal_view(id, value(1), on_new_value(1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 0);

    refresh_signal_view(id, empty<int>(), on_new_value(-1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 1);

    refresh_signal_view(id, empty<int>(), on_new_value(-1), on_lost_value);
    REQUIRE(new_value_count == 2);
    REQUIRE(lost_value_count == 1);

    refresh_signal_view(id, value(0), on_new_value(0), on_lost_value);
    REQUIRE(new_value_count == 3);
    REQUIRE(lost_value_count == 1);

    // 3-lambda version...

    int reset_count = 0;
    auto on_either = [&]() { ++reset_count; };

    refresh_signal_view(
        id, empty<int>(), on_new_value(-1), on_lost_value, on_either);
    REQUIRE(new_value_count == 3);
    REQUIRE(lost_value_count == 2);
    REQUIRE(reset_count == 1);

    refresh_signal_view(
        id, empty<int>(), on_new_value(-1), on_lost_value, on_either);
    REQUIRE(new_value_count == 3);
    REQUIRE(lost_value_count == 2);
    REQUIRE(reset_count == 1);

    refresh_signal_view(
        id, value(0), on_new_value(0), on_lost_value, on_either);
    REQUIRE(new_value_count == 4);
    REQUIRE(lost_value_count == 2);
    REQUIRE(reset_count == 2);

    refresh_signal_view(
        id, value(0), on_new_value(0), on_lost_value, on_either);
    REQUIRE(new_value_count == 4);
    REQUIRE(lost_value_count == 2);
    REQUIRE(reset_count == 2);
}

template<class Wrapped>
struct transparent_wrapper
    : signal_wrapper<transparent_wrapper<Wrapped>, Wrapped>
{
    transparent_wrapper(Wrapped wrapped)
        : transparent_wrapper::signal_wrapper(std::move(wrapped))
    {
    }
};
template<class Wrapped>
transparent_wrapper<Wrapped>
make_transparent_wrapper(Wrapped wrapped)
{
    return transparent_wrapper<Wrapped>(std::move(wrapped));
}

TEST_CASE("signal_wrapper", "[signals][utilities]")
{
    state_storage<std::string> storage;
    auto w = make_state_signal(storage);
    auto s = make_transparent_wrapper(w);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);
    REQUIRE(signal_is_clearable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
    REQUIRE(s.value_id() == w.value_id());

    REQUIRE(signal_ready_to_write(s));
    write_signal(s, "foo");
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "foo");

    std::string x = move_signal(alia::move(s));
    REQUIRE(x == "foo");

    write_signal(s, "bar");
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "bar");

    clear_signal(s);
    REQUIRE(!signal_has_value(s));
}

template<class Wrapped>
struct transparent_casting_wrapper : casting_signal_wrapper<
                                         transparent_casting_wrapper<Wrapped>,
                                         Wrapped,
                                         typename Wrapped::value_type>
{
    transparent_casting_wrapper(Wrapped wrapped)
        : transparent_casting_wrapper::casting_signal_wrapper(
            std::move(wrapped))
    {
    }

    typename Wrapped::value_type const&
    read() const override
    {
        return this->wrapped_.read();
    }
    typename Wrapped::value_type
    move_out() const override
    {
        return this->wrapped_.move_out();
    }
    typename Wrapped::value_type&
    destructive_ref() const override
    {
        return this->wrapped_.destructive_ref();
    }
    id_interface const&
    write(typename Wrapped::value_type value) const override
    {
        return this->wrapped_.write(value);
    }
};
template<class Wrapped>
transparent_casting_wrapper<Wrapped>
make_transparent_casting_wrapper(Wrapped wrapped)
{
    return transparent_casting_wrapper<Wrapped>(std::move(wrapped));
}

TEST_CASE("casting_signal_wrapper", "[signals][utilities]")
{
    state_storage<std::string> storage;
    auto w = make_state_signal(storage);
    auto s = make_transparent_casting_wrapper(w);

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);
    REQUIRE(signal_is_clearable<signal_t>::value);

    REQUIRE(!signal_has_value(s));
    REQUIRE(s.value_id() == w.value_id());

    REQUIRE(signal_ready_to_write(s));
    write_signal(s, "foo");
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "foo");

    std::string x = move_signal(alia::move(s));
    REQUIRE(x == "foo");

    write_signal(s, "bar");
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "bar");

    clear_signal(s);
    REQUIRE(!signal_has_value(s));
}
