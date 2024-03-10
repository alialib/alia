#include <alia/core/signals/async.hpp>

#include <catch2/catch_test_macros.hpp>

#include <alia/core/flow/try_catch.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/operators.hpp>
#include <alia/core/signals/text.hpp>

#include <traversal.hpp>

using namespace alia;

TEST_CASE("async", "[signals][async]")
{
    alia::test_system sys;
    initialize_test_system(sys, [](core_context) {});

    async_reporter<int> reporter;

    auto make_controller = [&](int x) {
        return [=, &reporter](core_context ctx) {
            ALIA_TRY
            {
                do_text(
                    ctx,
                    printf(
                        ctx,
                        "%d",
                        add_default(
                            async<int>(
                                ctx,
                                [&](auto, auto r, auto x) {
                                    if (x < 0)
                                        throw 2;
                                    else
                                        reporter = r;
                                },
                                conditional(x >= -1, value(x), empty<int>())),
                            0)));
            }
            ALIA_CATCH(...)
            {
                do_text(ctx, value("(error)"));
            }
            ALIA_END
        };
    };

    check_traversal(sys, make_controller(-2), "0;");
    check_traversal(sys, make_controller(-1), "(error);");
    check_traversal(sys, make_controller(0), "0;");
    reporter.report_success(1);
    auto old_reporter = reporter;
    check_traversal(sys, make_controller(0), "1;");
    check_traversal(sys, make_controller(1), "0;");
    try
    {
        throw 7;
    }
    catch (...)
    {
        reporter.report_failure(std::current_exception());
    }
    check_traversal(sys, make_controller(1), "(error);");
    check_traversal(sys, make_controller(1), "(error);");
    old_reporter.report_success(1);
    check_traversal(sys, make_controller(1), "(error);");
    check_traversal(sys, make_controller(2), "0;");
    old_reporter.report_success(1);
    check_traversal(sys, make_controller(2), "0;");
}
