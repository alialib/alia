#ifndef ALIA_UI_TEXT_COMPONENTS_HPP
#define ALIA_UI_TEXT_COMPONENTS_HPP

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>

namespace alia {

void
do_text(
    ui_context ctx,
    readable<std::string> text,
    layout const& layout_spec = default_layout);

void
do_wrapped_text(
    ui_context ctx,
    readable<std::string> text,
    layout const& layout_spec = default_layout);

} // namespace alia

#endif
