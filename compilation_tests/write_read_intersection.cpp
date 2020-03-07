#include <alia/signals/basic.hpp>

using namespace alia;

void
f()
{
    signal_direction_intersection<write_only_signal, duplex_signal>::
        type();
#ifdef ALIA_TEST_COMPILATION_FAILURE
    signal_direction_intersection<write_only_signal, read_only_signal>::type();
#endif
}
