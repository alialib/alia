#ifdef ALIA_TEST_COMPILATION_FAILURE
#define ALIA_STRICT_OPERATORS
#endif

#include <alia/flow/actions.hpp>

using namespace alia;

void
f()
{
    int x = 0;
    auto a = (direct(x) <<= 1);
}
