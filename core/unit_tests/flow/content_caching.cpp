#include <alia/core/flow/content_caching.hpp>

#include <alia/core/flow/try_catch.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/operators.hpp>

#include <flow/testing.hpp>

TEST_CASE("simple content caching", "[flow][content_caching]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_object(ctx, "bit0");
        }
        ALIA_END

        ALIA_IF(n & 2)
        {
            do_object(ctx, "bit1");
        }
        ALIA_END

        // Objects inside named blocks aren't necessarily deleted in order, so
        // this tests different code paths.
        naming_context nc(ctx);
        if (n & 32)
        {
            named_block nb(nc, make_id(32));
            do_object(ctx, "bit5");
        }

        invoke_pure_component(
            ctx,
            [&](auto ctx, auto n) {
                the_log << "traversing cached content; ";

                ALIA_IF(n & 4)
                {
                    do_object(ctx, "bit2");
                }
                ALIA_END

                ALIA_IF(n & 8)
                {
                    do_object(ctx, "bit3");
                }
                ALIA_END
            },
            value(n & 12));

        ALIA_IF(n & 16)
        {
            do_object(ctx, "bit4");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
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

    n = 0b000000;
    refresh_system(sys);
    check_log("traversing cached content; ");
    REQUIRE(root.object.to_string() == "root()");

    n = 0b000011;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();)");

    n = 0b000010;
    refresh_system(sys);
    check_log("removing bit0; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");

    n = 0b001111;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "traversing cached content; "
        "relocating bit2 into root after bit1; "
        "relocating bit3 into root after bit2; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();bit2();bit3();)");

    n = 0b001110;
    refresh_system(sys);
    check_log("removing bit0; ");
    REQUIRE(root.object.to_string() == "root(bit1();bit2();bit3();)");

    n = 0b101110;
    refresh_system(sys);
    check_log("relocating bit5 into root after bit1; ");
    REQUIRE(root.object.to_string() == "root(bit1();bit5();bit2();bit3();)");

    n = 0b101100;
    refresh_system(sys);
    check_log("removing bit1; ");
    REQUIRE(root.object.to_string() == "root(bit5();bit2();bit3();)");

    n = 0b101101;
    refresh_system(sys);
    check_log("relocating bit0 into root; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit5();bit2();bit3();)");

    n = 0b001101;
    refresh_system(sys);
    check_log(
        "relocating bit2 into root after bit0; "
        "relocating bit3 into root after bit2; "
        "removing bit5; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit2();bit3();)");

    n = 0b000101;
    refresh_system(sys);
    check_log(
        "traversing cached content; "
        "removing bit3; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit2();)");

    n = 0b100100;
    refresh_system(sys);
    check_log("removing bit0; relocating bit5 into root; ");
    REQUIRE(root.object.to_string() == "root(bit5();bit2();)");
}

TEST_CASE("exceptional content caching", "[flow][content_caching]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_object(ctx, "bit0");
        }
        ALIA_END

        ALIA_IF(n & 2)
        {
            do_object(ctx, "bit1");
        }
        ALIA_END

        ALIA_TRY
        {
            invoke_pure_component(
                ctx,
                [&](auto ctx, auto n) {
                    the_log << "traversing cached content; ";

                    ALIA_IF(n & 4)
                    {
                        do_object(ctx, "bit2");
                    }
                    ALIA_END

                    ALIA_IF(n & 8)
                    {
                        auto f = [&](int i) {
                            return std::string("abcdef").substr(size_t(i));
                        };
                        do_object(ctx, lazy_apply(f, n));
                    }
                    ALIA_END
                },
                value(n & 12));
        }
        ALIA_CATCH(...)
        {
            do_object(ctx, "error");
        }
        ALIA_END

        ALIA_IF(n & 16)
        {
            do_object(ctx, "bit4");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
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

    n = 0b000000;
    refresh_system(sys);
    check_log("traversing cached content; ");
    REQUIRE(root.object.to_string() == "root()");

    n = 0b000011;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();)");

    n = 0b000010;
    refresh_system(sys);
    check_log("removing bit0; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");

    n = 0b001111;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "traversing cached content; "
        "relocating bit2 into root after bit1; "
        "relocating error into root after bit1; "
        "removing bit2; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();error();)");

    n = 0b001110;
    refresh_system(sys);
    clear_log();
    REQUIRE(root.object.to_string() == "root(bit1();error();)");

    n = 0b001100;
    refresh_system(sys);
    clear_log();
    REQUIRE(root.object.to_string() == "root(error();)");

    n = 0b001101;
    refresh_system(sys);
    clear_log();
    REQUIRE(root.object.to_string() == "root(bit0();error();)");

    n = 0b000101;
    refresh_system(sys);
    check_log(
        "traversing cached content; "
        "relocating bit2 into root after bit0; "
        "removing error; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit2();)");

    n = 0b010100;
    refresh_system(sys);
    check_log("removing bit0; relocating bit4 into root after bit2; ");
    REQUIRE(root.object.to_string() == "root(bit2();bit4();)");
}
