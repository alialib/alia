#ifndef ALIA_UI_TEXT_CONTROL_HPP
#define ALIA_UI_TEXT_CONTROL_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/utilities/widgets.hpp>

namespace alia {

enum text_control_event_type
{
    TEXT_CONTROL_NO_EVENT,
    TEXT_CONTROL_ENTER_PRESSED,
    TEXT_CONTROL_FOCUS_LOST,
    TEXT_CONTROL_EDIT_CANCELED,
};

struct control_result
{
    bool changed;
    // allows use within if statements without other unintended conversions
    typedef bool control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    {
        return changed ? &control_result::changed : 0;
    }
};

struct text_control_result : control_result
{
    text_control_event_type event;
};

ALIA_DEFINE_FLAG_TYPE(text_control)
ALIA_DEFINE_FLAG(text_control, 0x01, TEXT_CONTROL_DISABLED)
ALIA_DEFINE_FLAG(text_control, 0x02, TEXT_CONTROL_MASK_CONTENTS)
ALIA_DEFINE_FLAG(text_control, 0x04, TEXT_CONTROL_SINGLE_LINE)
ALIA_DEFINE_FLAG(text_control, 0x08, TEXT_CONTROL_MULTILINE)
ALIA_DEFINE_FLAG(text_control, 0x10, TEXT_CONTROL_IMMEDIATE)

text_control_result
do_unsafe_text_control(
    ui_context ctx,
    duplex<std::string> const& value,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    std::optional<size_t> length_limit = std::nullopt);

inline void
do_text_control(
    ui_context ctx,
    duplex<std::string> const& value,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    std::optional<size_t> length_limit = std::nullopt)
{
    if (do_unsafe_text_control(
            ctx, value, layout_spec, flags, id, length_limit))
    {
        abort_traversal(ctx);
    }
}

} // namespace alia

#endif
