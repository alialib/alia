#define ALIA_LOWERCASE_MACROS
#include <alia/core/flow/try_catch.hpp>

#include <alia/core/signals/async.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/text.hpp>

#include <flow/testing.hpp>

struct my_exception
{
    int x;
};

TEST_CASE("try_catch", "[flow][try_catch]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        do_container(ctx, "main", [&](test_context ctx) {
            alia_try
            {
                do_container(ctx, "sub", [&](test_context ctx) {
                    alia_try
                    {
                        do_object(ctx, "inner");

                        alia_if(n == 0)
                        {
                            do_object(
                                ctx, std::string(4, std::string().at(0)));
                        }
                        alia_end

                        alia_if(n < 2)
                        {
                            do_object(ctx, "conditional");
                        }
                        alia_end

                        if (n == 1)
                        {
                            throw my_exception{4};
                        }

                        if (n == 2)
                        {
                            throw n;
                        }
                    }
                    alia_catch(my_exception & e)
                    {
                        do_object(ctx, "my_exception:" + to_string(e.x));
                    }
                    alia_end

                    do_object(ctx, "outer");
                });
            }
            alia_catch(std::exception&)
            {
                do_object(ctx, "std::exception");
            }
            alia_catch(...)
            {
                do_object(ctx, "something");
                do_object(ctx, "else");
            }
            alia_end

            do_object(ctx, "tail");
        });
    };

    alia::test_system sys;
    initialize_test_system(sys, [&](core_context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = extend_context<tree_traversal_tag>(vanilla_ctx, traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = -1;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(inner();conditional();outer(););tail(););)");

    n = 4;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(inner();outer(););tail(););)");

    n = -1;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(inner();conditional();outer(););tail(););)");

    n = 0;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string() == "root(main(std::exception();tail(););)");

    n = 0;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string() == "root(main(std::exception();tail(););)");

    n = 1;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(my_exception:4();outer(););tail(););)");

    n = 1;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(my_exception:4();outer(););tail(););)");

    n = 0;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string() == "root(main(std::exception();tail(););)");

    n = -1;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string()
        == "root(main(sub(inner();conditional();outer(););tail(););)");

    n = 2;
    refresh_system(sys);
    REQUIRE(
        root.object.to_string() == "root(main(something();else();tail(););)");
}

TEST_CASE("async within catch", "[flow][try_catch]")
{
    clear_log();

    tree_node<test_object> root;
    root.object.name = "root";

    async_reporter<std::string> reporter;

    auto controller = [&](test_context ctx) {
        do_container(ctx, "main", [&](test_context ctx) {
            alia_try
            {
                std::string(4, std::string().at(0));
            }
            alia_catch(...)
            {
                do_object(ctx, "caught");

                auto message = async<std::string>(
                    ctx, [&](auto, auto r, auto) { reporter = r; }, value(2));
                do_object(ctx, message);
            }
            alia_end

            do_object(ctx, "tail");
        });
    };

    alia::test_system sys;
    initialize_test_system(sys, [&](core_context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = extend_context<tree_traversal_tag>(vanilla_ctx, traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(main(caught();();tail(););)");

    reporter.report_success("asyncd");
    REQUIRE(
        root.object.to_string() == "root(main(caught();asyncd();tail(););)");
}
