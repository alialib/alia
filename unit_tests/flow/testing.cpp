#include "flow/testing.hpp"

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

data_traversal&
get_data_traversal(custom_context& ctx)
{
    return ctx.traversal;
}
