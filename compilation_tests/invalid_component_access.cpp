#include <alia/components/typing.hpp>

#include <boost/any.hpp>

using namespace alia;

struct foo
{
    bool b = false;
};
struct foo_tag
{
};

struct bar
{
    int i = 0;
};
struct bar_tag
{
};

using storage_type = generic_component_storage<boost::any>;
using cc_empty = empty_component_collection<storage_type>;

void
f()
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    auto mc_b = add_component<bar_tag>(mc_empty, bar());
    auto mc_fb = add_component<foo_tag>(mc_b, foo());
    auto mc_f = remove_component<bar_tag>(mc_fb);
    get_component<foo_tag>(mc_f);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    get_component<bar_tag>(mc_f);
#endif
}
