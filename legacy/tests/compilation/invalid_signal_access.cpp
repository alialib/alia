#include <alia/core/signals/basic.hpp>

using namespace alia;

void
f()
{
    auto s = value(0);
    signal_has_value(s);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    signal_ready_to_write(s);
#endif
}
