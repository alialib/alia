#ifndef ALIA_UI_LIBRARY_CHECKBOX_HPP
#define ALIA_UI_LIBRARY_CHECKBOX_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/ids.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

void
do_checkbox(
    ui_context ctx,
    duplex<bool> checked,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

} // namespace alia

#endif
