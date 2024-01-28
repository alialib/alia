#define ALIA_LOWERCASE_MACROS

#include <alia/core/signals/higher_order.hpp>

#include <catch2/catch_test_macros.hpp>

#include <alia/core/flow/for_each.hpp>
#include <alia/core/signals/application.hpp>
#include <alia/core/signals/basic.hpp>

#include "traversal.hpp"

using namespace alia;
using std::string;

TEST_CASE("simple sequence transform", "[signals][higher_order]")
{
    std::vector<string> container{"foo", "barre", "q"};

    alia::system sys;
    initialize_standalone_system(sys, [](core_context) {});

    captured_id transform_id;
    auto controller = [&](int offset) {
        return [&, offset](core_context ctx) {
            auto transformed_signal = transform(
                ctx, direct(container), [&](readable<string> s) {
                    return lazy_apply(alia_mem_fn(length), s) + value(offset);
                });
            transform_id.capture(transformed_signal.value_id());
            for_each(ctx, transformed_signal, [&](readable<size_t> value) {
                do_text(ctx, apply(ctx, alia_lambdify(std::to_string), value));
            });
        };
    };

    check_traversal(sys, controller(1), "4;6;2;");
    captured_id last_id = transform_id;
    check_traversal(sys, controller(0), "3;5;1;");
    REQUIRE(last_id != transform_id);
}

TEST_CASE("simple associative transform", "[signals][higher_order]")
{
    std::map<string, string> container{
        {"a", "foo"}, {"b", "barre"}, {"d", "q"}};

    alia::system sys;
    initialize_standalone_system(sys, [](core_context) {});

    captured_id transform_id;
    auto controller = [&](int offset) {
        return [&, offset](core_context ctx) {
            auto transformed_signal = transform(
                ctx,
                direct(container),
                [&](readable<string>, readable<string> v) {
                    return lazy_apply(alia_mem_fn(length), v) + value(offset);
                });
            transform_id.capture(transformed_signal.value_id());
            for_each(
                ctx,
                transformed_signal,
                [&](readable<string> k, readable<size_t> v) {
                    do_text(ctx, k);
                    do_text(ctx, apply(ctx, alia_lambdify(std::to_string), v));
                });
        };
    };

    check_traversal(sys, controller(1), "a;4;b;6;d;2;");
    captured_id last_id = transform_id;
    check_traversal(sys, controller(0), "a;3;b;5;d;1;");
    REQUIRE(last_id != transform_id);
}
