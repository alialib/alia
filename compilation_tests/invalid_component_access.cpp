#include <alia/component_collection.hpp>

using namespace alia;

struct foo_tag
{
};
struct foo
{
    bool b = false;
};
struct bar_tag
{
};
struct bar
{
    int i = 0;
};

using storage_type = generic_component_storage<any>;
using cc_empty = empty_component_collection<storage_type>;

void
f()
{
    storage_type storage_empty;
    cc_empty mc_empty(&storage_empty);

    storage_type storage_b;
    auto mc_b = add_component<bar_tag>(&storage_b, mc_empty, bar());

    storage_type storage_fb;
    auto mc_fb = add_component<foo_tag>(&storage_fb, mc_b, foo());

    storage_type storage_f;
    auto mc_f = remove_component<bar_tag>(&storage_f, mc_fb);
    get_component<foo_tag>(mc_f);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    get_component<bar_tag>(mc_f);
#endif
}
