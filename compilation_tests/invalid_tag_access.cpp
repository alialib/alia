#include <alia/context/typing.hpp>

#include <alia/context/storage.hpp>

using namespace alia::impl;

ALIA_DEFINE_TAGGED_TYPE(foo_tag, int)
ALIA_DEFINE_TAGGED_TYPE(bar_tag, int)

using storage_type = generic_tagged_storage<int>;
using cc_empty = empty_structural_collection<storage_type>;

void
f()
{
    storage_type storage;
    cc_empty mc_empty(&storage);
    auto mc_b = add_tagged_data<bar_tag>(mc_empty, 0);
    auto mc_fb = add_tagged_data<foo_tag>(mc_b, 1);
    auto mc_f = remove_tagged_data<bar_tag>(mc_fb);
    get_tagged_data<foo_tag>(mc_f);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    get_tagged_data<bar_tag>(mc_f);
#endif
}
