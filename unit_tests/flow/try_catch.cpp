#define ALIA_LOWERCASE_MACROS
#include <alia/flow/try_catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/text.hpp>

#include <flow/testing.hpp>

struct my_exception
{
    int x = 0;
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

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
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
