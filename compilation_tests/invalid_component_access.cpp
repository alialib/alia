#include <alia/components/typing.hpp>

using namespace alia;

struct foo
{
    bool b = false;
};
ALIA_DEFINE_COMPONENT_TYPE(foo_tag, foo)
struct bar
{
    int i = 0;
};
ALIA_DEFINE_COMPONENT_TYPE(bar_tag, bar)

using storage_type = generic_component_storage<any_value>;
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
