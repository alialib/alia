#include <alia/signals.hpp>

using namespace alia;

void
g(input<int> x)
{
}

void
h(inout<int> x)
{
}

void
f()
{
    auto s = value(0);
    g(s);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    h(s);
#endif
}
