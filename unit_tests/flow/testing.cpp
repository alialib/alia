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
do_object(test_context ctx, std::string name)
{
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
}
