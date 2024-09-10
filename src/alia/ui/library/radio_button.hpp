#ifndef ALIA_UI_LIBRARY_RADIO_BUTTON_HPP
#define ALIA_UI_LIBRARY_RADIO_BUTTON_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/ids.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

void
do_radio_button(
    ui_context ctx,
    duplex<bool> selected,
    layout const& layout_spec = layout(TOP | LEFT | PADDED),
    widget_id id = auto_id);

}

#endif
