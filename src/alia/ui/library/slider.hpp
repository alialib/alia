#ifndef ALIA_UI_LIBRARY_SLIDER_HPP
#define ALIA_UI_LIBRARY_SLIDER_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

ALIA_DEFINE_FLAG_TYPE(slider)
ALIA_DEFINE_FLAG(slider, 0x0, SLIDER_HORIZONTAL)
ALIA_DEFINE_FLAG(slider, 0x1, SLIDER_VERTICAL)

void
do_slider(
    ui_context ctx,
    duplex<double> value,
    double minimum,
    double maximum,
    double step,
    layout const& layout_spec = default_layout,
    slider_flag_set flags = NO_FLAGS);

} // namespace alia

#endif
