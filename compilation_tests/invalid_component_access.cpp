#include <alia/components/typing.hpp>

#include <alia/components/storage.hpp>

using namespace alia;

ALIA_DEFINE_COMPONENT_TYPE(foo_tag, int)
ALIA_DEFINE_COMPONENT_TYPE(bar_tag, int)

using storage_type = generic_component_storage<int>;
using cc_empty = empty_component_collection<storage_type>;

void
f()
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    auto mc_b = add_component<bar_tag>(mc_empty, 0);
    auto mc_fb = add_component<foo_tag>(mc_b, 1);
    auto mc_f = remove_component<bar_tag>(mc_fb);
    get_component<foo_tag>(mc_f);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    get_component<bar_tag>(mc_f);
#endif
}
