#include <alia/signals/basic.hpp>

using namespace alia;

void
f_input(input<int> x)
{
}

void
f_bidirectional(bidirectional<int> x)
{
}

void
f()
{
    auto read_only = value(0);
    f_input(read_only);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    f_bidirectional(read_only);
#endif
}
