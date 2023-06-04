#include <alia/common.hpp>

using namespace alia;

struct foo : noncopyable
{
};

void
f()
{
    foo x;
    foo y;
#ifdef ALIA_TEST_COMPILATION_FAILURE
    y = x;
#endif
}
