#include <alia/signals/basic.hpp>

using namespace alia;

void
f()
{
    auto s = val(0);
    signal_is_readable(s);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    signal_is_writable(s);
#endif
}
