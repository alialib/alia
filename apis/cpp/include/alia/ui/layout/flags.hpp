#pragma once

#include <alia/abi/ui/layout/flags.h>
#include <alia/base/flags.hpp>

namespace alia {

// placement / leaf-oriented layout flags
ALIA_DEFINE_FLAG_TYPE(alia_layout_flags_t, layout)

#define ALIA_DEFINE_LAYOUT_FLAG(value, id) ALIA_DEFINE_FLAG(layout, value, id)
ALIA_LAYOUT_PLACEMENT_FLAGS(ALIA_DEFINE_LAYOUT_FLAG)
#undef ALIA_DEFINE_LAYOUT_FLAG

// container-policy layout flags
ALIA_DEFINE_FLAG_TYPE(alia_layout_flags_t, container_layout)

#define ALIA_DEFINE_CONTAINER_LAYOUT_FLAG(value, id)                          \
    ALIA_DEFINE_FLAG(container_layout, value, id)
ALIA_LAYOUT_CONTAINER_FLAGS(ALIA_DEFINE_CONTAINER_LAYOUT_FLAG)
#undef ALIA_DEFINE_CONTAINER_LAYOUT_FLAG

} // namespace alia
