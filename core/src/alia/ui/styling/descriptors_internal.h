#pragma once

#include <alia/abi/ui/styling/environment.h>
#include <alia/ui/styling/style_system.h>

namespace alia {

// Copy core var and color descriptors into `styling`, then apply var defaults
// for light or dark mode into `styling.environment`.
void
style_descriptors_init(style_system& styling, bool dark = true);

// Apply core built-in var defaults into `out`. Leaves palette and seeds alone.
void
style_vars_apply_core_defaults(alia_style_environment* out, bool dark);

} // namespace alia
