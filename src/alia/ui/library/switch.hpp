#ifndef ALIA_UI_LIBRARY_SWITCH_HPP
#define ALIA_UI_LIBRARY_SWITCH_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

void
do_switch(
    ui_context ctx,
    duplex<bool> state,
    layout const& layout_spec = default_layout);

} // namespace alia

#endif
