#include <alia/flow/object_trees.hpp>

#include <alia/flow/events.hpp>

#include <flow/testing.hpp>

namespace {

struct test_object : tree_object<test_object>
{
    ~test_object()
    {
        the_log << "removing " << name << "; ";
    }

    void
    relocate(test_object& parent, test_object* after)
    {
        the_log << "relocating " << name << " into " << parent.name;
        if (after)
            the_log << " after " << after->name;
        the_log << "; ";
    }

    void
    truncate(test_object* after)
    {
        the_log << "truncating " << name;
        if (after)
            the_log << " after " << after->name;
        the_log << "; ";
    }

    std::string name;
};

ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<test_object>&)

typedef alia::extend_context_type_t<alia::context, tree_traversal_tag>
    test_context;

void
do_object(test_context ctx, std::string name)
{
    test_object* object;
    if (get_cached_data(ctx, &object))
        object->name = name;
    if (is_refresh_event(ctx))
        refresh_tree_object(get<tree_traversal_tag>(ctx), *object);
}

template<class Contents>
void
do_container(test_context ctx, std::string name, Contents contents)
{
    scoped_tree_object<test_object> scoped;
    test_object* object;
    if (get_cached_data(ctx, &object))
        object->name = name;
    if (is_refresh_event(ctx))
        scoped.begin(get<tree_traversal_tag>(ctx), *object);
    contents(ctx);
}

} // namespace

TEST_CASE("some object tree", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    test_object root;
    root.name = "root";

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

    n = 0;
    refresh_system(sys);
    check_log("truncating root; ");

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; "
        "truncating root after bit1; ");

    n = 0;
    refresh_system(sys);
    check_log(
        "removing bit0; "
        "removing bit1; "
        "truncating root; ");

    n = 2;
    refresh_system(sys);
    check_log(
        "relocating bit1 into root; "
        "truncating root after bit1; ");

    n = 15;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit0 into root; "
        "relocating bit2 into root; "
        "relocating bit3 into root; "
        "truncating root after bit3; ");
}
