#include <alia/common.hpp>

using namespace alia;

struct foo : noncopyable
{
};

void
f()
{
    foo x;
#ifdef ALIA_TEST_COMPILATION_FAILURE
    foo y(x);
#endif
}
