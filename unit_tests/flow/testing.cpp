#include <flow/testing.hpp>

std::stringstream the_log;

void
clear_log()
{
    the_log.str(std::string());
}

void
check_log(std::string const& expected_contents)
{
    REQUIRE(the_log.str() == expected_contents);
    clear_log();
}

void
do_object(test_context ctx, readable<std::string> name)
{
    tree_node<test_object>* node;
    get_cached_data(ctx, &node);
    if (is_refresh_event(ctx))
    {
        if (signal_has_value(name))
            node->object.name = read_signal(name);
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
    }
}

void
do_object(test_context ctx, std::string name)
{
    do_object(ctx, value(name));
}
