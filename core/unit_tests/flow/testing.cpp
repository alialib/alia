#include <flow/testing.hpp>
#include <regex>

std::stringstream the_log;

void
clear_log()
{
    the_log.str(std::string());
}

void
check_log(std::string const& expected_log)
{
    auto actual_log = the_log.str();
    clear_log();

    REQUIRE(actual_log == expected_log);
}

void
match_log(std::string const& expected_pattern)
{
    auto actual_log = the_log.str();
    clear_log();

    CAPTURE(actual_log);
    CAPTURE(expected_pattern);

    REQUIRE(std::regex_match(actual_log, std::regex(expected_pattern)));
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
