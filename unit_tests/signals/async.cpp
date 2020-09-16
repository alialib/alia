#include <alia/signals/async.hpp>

#include <testing.hpp>

#include <alia/flow/try_catch.hpp>
#include <alia/signals/basic.hpp>
#include <alia/signals/text.hpp>

#include <traversal.hpp>

using namespace alia;

TEST_CASE("async", "[signals][async]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    async_reporter<int> reporter;

    auto make_controller = [&](int x) {
        return [=, &reporter](context ctx) {
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
                                value(x)),
                            0)));
            }
            ALIA_CATCH(...)
            {
                do_text(ctx, value("(error)"));
            }
            ALIA_END
        };
    };

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
