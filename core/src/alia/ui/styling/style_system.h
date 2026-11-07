#pragma once

#include <alia/abi/ui/styling/descriptors.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/ui/styling/style_catalog.h>

#include <cstddef>

namespace alia {

// styling environment (palette, vars, seeds) plus style catalog and
// per-UI descriptor tables for vars and colors
struct style_system
{
    alia_style_environment environment{};
    style_catalog styles;

    alia_style_float_desc var_descs[ALIA_STYLE_VAR_COUNT]{};
    size_t var_desc_count = 0;
    alia_style_color_desc color_descs[ALIA_STYLE_COLOR_COUNT]{};
    size_t color_desc_count = 0;
};

} // namespace alia
