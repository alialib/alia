#ifndef ALIA_UI_TEXT_COMPONENTS_HPP
#define ALIA_UI_TEXT_COMPONENTS_HPP

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>

namespace alia {

struct text_style
{
    std::string font_name;
    float font_size;
    rgba8 color;

    auto
    operator<=>(text_style const&) const
        = default;
};

void
do_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec = default_layout);

void
do_wrapped_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec = default_layout);

} // namespace alia

#endif
