#define ALIA_LOWERCASE_MACROS

#include <alia/signals/higher_order.hpp>

#include <testing.hpp>

#include <alia/flow/for_each.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/basic.hpp>

#include "traversal.hpp"

using namespace alia;
using std::string;

TEST_CASE("simple sequence transform", "[signals][higher_order]")
{
    std::vector<string> container{"foo", "barre", "q"};

    alia::system sys;

    captured_id transform_id;
    auto controller = [&](int offset) {
        return [&, offset](context ctx) {
            auto transformed_signal = transform(
                ctx, direct(container), [&](context, readable<string> s) {
                    return lazy_apply(alia_mem_fn(length), s) + value(offset);
                });
            transform_id.capture(transformed_signal.value_id());
            for_each(
                ctx,
                transformed_signal,
                [&](context ctx, readable<size_t> value) {
                    do_text(
                        ctx, apply(ctx, alia_lambdify(std::to_string), value));
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

    captured_id transform_id;
    auto controller = [&](int offset) {
        return [&, offset](context ctx) {
            auto transformed_signal = transform(
                ctx,
                direct(container),
                [&](context, readable<string>, readable<string> v) {
                    return lazy_apply(alia_mem_fn(length), v) + value(offset);
                });
            transform_id.capture(transformed_signal.value_id());
            for_each(
                ctx,
                transformed_signal,
                [&](context ctx, readable<string> k, readable<size_t> v) {
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
