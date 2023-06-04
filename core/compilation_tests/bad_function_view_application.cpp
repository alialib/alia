#include <alia/core/common.hpp>

using namespace alia;

int
invoke_with_bool(function_view<int(bool)> a)
{
    return a(false);
}

int
invoke_with_string(function_view<int(std::string)> a)
{
    return a(std::string("foo"));
}

void
f()
{
    invoke_with_bool([](bool x) { return x ? 3 : 1; });
    invoke_with_string([](std::string x) { return x.empty() ? 3 : 1; });
#ifdef ALIA_TEST_COMPILATION_FAILURE
    invoke_with_bool([](std::string x) { return x.empty() ? 3 : 1; });
#endif
}
