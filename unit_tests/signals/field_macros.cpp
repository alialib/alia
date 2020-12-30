#include <alia/signals/field_macros.hpp>

#include <testing.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/lambdas.hpp>

// __declspec(property(...)) is non-standard.
#if defined(__clang__) || defined(_MSC_VER)

namespace {

struct foo
{
    int x;
    std::string y;
};

ALIA_DEFINE_STRUCT_SIGNALS(foo, x, y)

} // namespace

TEST_CASE("ALIA_DEFINE_STRUCT_SIGNALS", "[signals][field_macros]")
{
    using namespace alia;

    foo f = {2, "1.5"};
    auto f_signal = lambda_duplex(
        always_has_value,
        [&]() { return f; },
        always_ready,
        [&](foo const& v) { f = v; },
        [&]() { return combine_ids(make_id(f.x), make_id(f.y)); });

    REQUIRE((std::is_same<readable_foo::value_type, foo>::value));
    REQUIRE(signal_is_readable<readable_foo>::value);
    REQUIRE(!signal_is_writable<readable_foo>::value);

    readable_foo rf = f_signal;

    REQUIRE(signal_has_value(rf.x));
    REQUIRE(read_signal(rf.x) == 2);

    REQUIRE((std::is_same<duplex_foo::value_type, foo>::value));
    REQUIRE(signal_is_readable<duplex_foo>::value);
    REQUIRE(signal_is_writable<duplex_foo>::value);

    duplex_foo df = f_signal;

    typedef decltype(df.x) x_signal_t;
    REQUIRE((std::is_same<x_signal_t::value_type, int>::value));
    REQUIRE(signal_is_readable<x_signal_t>::value);
    REQUIRE(signal_is_writable<x_signal_t>::value);

    REQUIRE(signal_has_value(df.x));
    REQUIRE(read_signal(df.x) == 2);
    REQUIRE(signal_ready_to_write(df.x));
    write_signal(df.x, 1);
    REQUIRE(f.x == 1);

    auto x_signal = df.x;
    auto y_signal = df.y;

    typedef decltype(y_signal) y_signal_t;
    REQUIRE((std::is_same<y_signal_t::value_type, std::string>::value));
    REQUIRE(signal_is_readable<y_signal_t>::value);
    REQUIRE(signal_is_writable<y_signal_t>::value);

    REQUIRE(y_signal.value_id() != x_signal.value_id());
    REQUIRE(signal_has_value(y_signal));
    REQUIRE(read_signal(y_signal) == "1.5");
    REQUIRE(signal_ready_to_write(y_signal));
    captured_id original_y_id = y_signal.value_id();
    write_signal(y_signal, "0.5");
    REQUIRE(y_signal.value_id() != original_y_id.get());
    REQUIRE(f.y == "0.5");
}

#endif
