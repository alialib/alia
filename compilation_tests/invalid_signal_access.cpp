#include <alia/signals.hpp>

using namespace alia;

void
f()
{
    auto s = value(0);
    signal_is_readable(s);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    signal_is_writable(s);
#endif
}
