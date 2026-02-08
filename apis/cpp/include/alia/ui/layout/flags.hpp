#pragma once

#include <alia/abi/ui/layout/flags.h>
#include <alia/flags.hpp>

namespace alia {

ALIA_DEFINE_FLAG_TYPE(alia_layout_flags_t, layout)

#define ALIA_DEFINE_LAYOUT_FLAG(value, id) ALIA_DEFINE_FLAG(layout, value, id)
ALIA_LAYOUT_FLAGS(ALIA_DEFINE_LAYOUT_FLAG)
#undef ALIA_DEFINE_LAYOUT_FLAG

} // namespace alia
