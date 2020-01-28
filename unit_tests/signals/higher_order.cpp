#define ALIA_LOWERCASE_MACROS

#include <alia/signals/higher_order.hpp>

#include <catch.hpp>

#include <alia/flow/for_each.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/basic.hpp>

#include "traversal.hpp"

using namespace alia;
using std::string;

TEST_CASE("simple transform", "[signals][higher_order]")
{
    std::vector<string> container{"foo", "barre", "q"};

    alia::system sys;

    auto controller = [&](context ctx) {
        for_each(
            ctx,
            transform(
                ctx,
                direct(container),
                [&](context ctx, readable<string> s) {
                    return lazy_apply(alia_method(length), s) + val(1);
                }),
            [&](context ctx, readable<size_t> value) {
                do_text(ctx, apply(ctx, alia_lambdify(std::to_string), value));
            });
    };

    check_traversal(sys, controller, "4;6;2;");
}
