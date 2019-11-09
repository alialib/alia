#ifdef ALIA_TEST_COMPILATION_FAILURE
#define ALIA_STRICT_CONDITIONALS
#endif

#include <alia/flow/macros.hpp>

using namespace alia;

void
f(data_traversal& ctx)
{
    ALIA_IF(1 > 0)
    {
    }
    ALIA_END
}
