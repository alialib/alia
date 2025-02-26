#include <alia/ui/text/control.hpp>

namespace alia {

text_control_result
do_unsafe_text_control(
    ui_context,
    duplex<std::string> const&,
    layout const&,
    text_control_flag_set,
    widget_id,
    std::optional<size_t>)
{
    return text_control_result{false, TEXT_CONTROL_NO_EVENT};
}

} // namespace alia
