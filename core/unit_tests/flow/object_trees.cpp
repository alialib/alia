#include <alia/core/flow/object_trees.hpp>

#include <flow/testing.hpp>

TEST_CASE("simple object tree", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";
    REQUIRE(get_object(root).name == "root");

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

        ALIA_IF(n & 16)
        {
            do_object(ctx, "bit4");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system<context>(sys, [&](context vanilla_ctx) {
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

    n = 0;
    refresh_system(sys);
    check_log("");
    REQUIRE(root.object.to_string() == "root()");

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();)");

    n = 0;
    refresh_system(sys);
    check_log(
        "removing bit0; "
        "removing bit1; ");
    REQUIRE(root.object.to_string() == "root()");

    n = 2;
    refresh_system(sys);
    check_log("relocating bit1 into root; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");

    n = 15;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into root after bit1; "
        "relocating bit3 into root after bit2; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();bit2();bit3();)");

    n = 13;
    refresh_system(sys);
    check_log("removing bit1; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit2();bit3();)");

    n = 2;
    refresh_system(sys);
    check_log(
        "removing bit0; "
        "relocating bit1 into root; "
        "removing bit2; "
        "removing bit3; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");
}

TEST_CASE("multilevel object tree", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_container(ctx, "bit0", [&](test_context ctx) {
                ALIA_IF(n & 2)
                {
                    do_object(ctx, "bit1");
                }
                ALIA_END

                ALIA_IF(n & 4)
                {
                    do_container(ctx, "bit2", [&](test_context ctx) {
                        ALIA_IF(n & 8)
                        {
                            do_object(ctx, "bit3");
                        }
                        ALIA_END

                        ALIA_IF(n & 16)
                        {
                            do_object(ctx, "bit4");
                        }
                        ALIA_END
                    });
                }
                ALIA_END

                ALIA_IF(n & 32)
                {
                    do_object(ctx, "bit5");
                }
                ALIA_END
            });
        }
        ALIA_END

        ALIA_IF(n & 64)
        {
            do_object(ctx, "bit6");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system<context>(sys, [&](context vanilla_ctx) {
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

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0(bit1(););)");

    n = 64;
    refresh_system(sys);
    check_log(
        "removing bit1; "
        "removing bit0; "
        "relocating bit6 into root; ");
    REQUIRE(root.object.to_string() == "root(bit6();)");

    n = 125;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into bit0; "
        "relocating bit3 into bit2; "
        "relocating bit4 into bit2 after bit3; "
        "relocating bit5 into bit0 after bit2; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit2(bit3();bit4(););bit5(););bit6();)");

    n = 55;
    refresh_system(sys);
    check_log(
        "relocating bit1 into bit0; "
        "removing bit3; "
        "removing bit6; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit1();bit2(bit4(););bit5(););)");
}

TEST_CASE("fluid object tree", "[flow][object_trees]")
{
    clear_log();

    tree_node<test_object> root;
    root.object.name = "root";

    std::vector<std::string> a_team, b_team;

    auto controller = [&](test_context ctx) {
        naming_context nc(ctx);
        do_container(ctx, "a_team", [&](test_context ctx) {
            for (auto& name : a_team)
            {
                named_block nb(nc, make_id(name));
                do_object(ctx, name);
            }
        });
        do_container(ctx, "b_team", [&](test_context ctx) {
            for (auto& name : b_team)
            {
                named_block nb(nc, make_id(name));
                do_object(ctx, name);
            }
        });
    };

    alia::system sys;
    initialize_system<context>(sys, [&](context vanilla_ctx) {
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

    a_team = {"alf", "betty", "charlie", "dot"};
    b_team = {"edgar"};
    refresh_system(sys);
    check_log(
        "relocating a_team into root; "
        "relocating alf into a_team; "
        "relocating betty into a_team after alf; "
        "relocating charlie into a_team after betty; "
        "relocating dot into a_team after charlie; "
        "relocating b_team into root after a_team; "
        "relocating edgar into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(alf();betty();charlie();dot(););b_team(edgar(););)");

    a_team = {"betty", "charlie", "dot"};
    b_team = {"alf", "edgar"};
    refresh_system(sys);
    check_log(
        "relocating betty into a_team; "
        "relocating charlie into a_team after betty; "
        "relocating dot into a_team after charlie; "
        "removing alf; "
        "relocating alf into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();charlie();dot(););b_team(alf();edgar(););)");

    a_team = {"betty", "charlie"};
    b_team = {"alf", "edgar"};
    refresh_system(sys);
    check_log("removing dot; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();charlie(););b_team(alf();edgar(););)");

    a_team = {"betty", "edgar", "charlie"};
    b_team = {"alf"};
    refresh_system(sys);
    check_log("relocating edgar into a_team after betty; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();edgar();charlie(););b_team(alf(););)");

    a_team = {"charlie", "dot", "betty", "edgar"};
    b_team = {"alf"};
    refresh_system(sys);
    check_log(
        "relocating charlie into a_team; "
        "relocating dot into a_team after charlie; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(charlie();dot();betty();edgar(););b_team(alf(););)");

    a_team = {"edgar", "dot", "charlie", "alf"};
    b_team = {"betty"};
    refresh_system(sys);
    check_log(
        "relocating edgar into a_team; "
        "relocating dot into a_team after edgar; "
        "relocating alf into a_team after charlie; "
        "removing betty; "
        "relocating betty into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(edgar();dot();charlie();alf(););b_team(betty(););)");
}

TEST_CASE("piecewise containers", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_piecewise_container(ctx, "bit0", [&](test_context ctx) {
                ALIA_IF(n & 2)
                {
                    do_object(ctx, "bit1");
                }
                ALIA_END

                ALIA_IF(n & 4)
                {
                    do_piecewise_container(ctx, "bit2", [&](test_context ctx) {
                        ALIA_IF(n & 8)
                        {
                            do_object(ctx, "bit3");
                        }
                        ALIA_END

                        ALIA_IF(n & 16)
                        {
                            do_object(ctx, "bit4");
                        }
                        ALIA_END
                    });
                }
                ALIA_END

                ALIA_IF(n & 32)
                {
                    do_object(ctx, "bit5");
                }
                ALIA_END
            });
        }
        ALIA_END

        ALIA_IF(n & 64)
        {
            do_object(ctx, "bit6");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system<context>(sys, [&](context vanilla_ctx) {
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

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0(bit1(););)");

    n = 64;
    refresh_system(sys);
    check_log(
        "removing bit1; "
        "removing bit0; "
        "relocating bit6 into root; ");
    REQUIRE(root.object.to_string() == "root(bit6();)");

    n = 125;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into bit0; "
        "relocating bit3 into bit2; "
        "relocating bit4 into bit2 after bit3; "
        "relocating bit5 into bit0 after bit2; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit2(bit3();bit4(););bit5(););bit6();)");

    n = 55;
    refresh_system(sys);
    check_log(
        "relocating bit1 into bit0; "
        "removing bit3; "
        "removing bit6; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit1();bit2(bit4(););bit5(););)");
}

TEST_CASE("floating object tree root", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    tree_node<test_object> floating_root;
    floating_root.object.name = "floater";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_container(ctx, "bit0", [&](test_context ctx) {
                ALIA_IF(n & 2)
                {
                    do_object(ctx, "bit1");
                }
                ALIA_END

                {
                    scoped_tree_root<test_object> floating_subtree(
                        get<tree_traversal_tag>(ctx), floating_root);

                    ALIA_IF(n & 4)
                    {
                        do_container(ctx, "bit2", [&](test_context ctx) {
                            ALIA_IF(n & 8)
                            {
                                do_object(ctx, "bit3");
                            }
                            ALIA_END

                            ALIA_IF(n & 16)
                            {
                                do_object(ctx, "bit4");
                            }
                            ALIA_END
                        });
                    }
                    ALIA_END
                }

                ALIA_IF(n & 32)
                {
                    do_object(ctx, "bit5");
                }
                ALIA_END
            });
        }
        ALIA_END

        ALIA_IF(n & 64)
        {
            do_object(ctx, "bit6");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system<context>(sys, [&](context vanilla_ctx) {
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

    n = 127;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into bit0; "
        "relocating bit2 into floater; "
        "relocating bit3 into bit2; "
        "relocating bit4 into bit2 after bit3; "
        "relocating bit5 into bit0 after bit1; "
        "relocating bit6 into root after bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0(bit1();bit5(););bit6();)");
    REQUIRE(
        floating_root.object.to_string() == "floater(bit2(bit3();bit4(););)");
}
