#include <alia/signals.hpp>

using namespace alia;

void
f()
{
    signal_direction_intersection<read_only_signal, two_way_signal>::type();
#ifdef ALIA_TEST_COMPILATION_FAILURE
    signal_direction_intersection<read_only_signal, write_only_signal>::type();
#endif
}
