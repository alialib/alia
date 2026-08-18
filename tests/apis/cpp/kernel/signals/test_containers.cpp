#include <alia/kernel/signals/containers.hpp>

#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

#include <string>

using namespace alia;

TEST_CASE("container_size_view")
{
    std::string x = "foob";
    auto s = container_size_view(alia::ref(x));

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 4);
}

TEST_CASE("container_empty_view")
{
    {
        std::string x;
        auto s = container_empty_view(alia::ref(x));

        static_assert(view_signal<decltype(s)>);
        static_assert(!sink_signal<decltype(s)>);

        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == true);
    }
    {
        std::string x = "foo";
        auto s = container_empty_view(alia::ref(x));
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == false);
    }
    {
        auto s = container_empty_view(empty<std::string>());
        static_assert(view_signal<decltype(s)>);
        static_assert(!sink_signal<decltype(s)>);
        CHECK_FALSE(signal_has_value(s));
    }
}
