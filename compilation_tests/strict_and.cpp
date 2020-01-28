#ifdef ALIA_TEST_COMPILATION_FAILURE
#define ALIA_STRICT_OPERATORS
#endif

#include <alia/signals/operators.hpp>

using namespace alia;

void
f()
{
    auto x = value(true) && false;
}
