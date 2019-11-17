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

void
do_int(context ctx, int n)
{
    int_object* obj;
    if (get_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting int: " << n << ";";
    }
}

void
do_cached_int(context ctx, int n)
{
    int_object* obj;
    if (get_cached_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing cached int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting cached int: " << n << ";";
    }
}

void
do_keyed_int(context ctx, int n)
{
    keyed_data_signal<int_object> obj;
    if (get_keyed_data(ctx, make_id(n), &obj))
    {
        REQUIRE(!obj.is_readable());
        write_signal(obj, int_object(n * 2));
        the_log << "initializing keyed int: " << n << ";";
    }
    else
    {
        REQUIRE(read_signal(obj).n == n * 2);
        REQUIRE(obj.value_id() == make_id(n));
        the_log << "visiting keyed int: " << n << ";";
    }
}
