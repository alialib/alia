#ifndef ALIA_TEXT_CONTROL_HPP
#define ALIA_TEXT_CONTROL_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/text_utils.hpp>
#include <alia/flags.hpp>

namespace alia {

enum text_control_event_type
{
    TEXT_CONTROL_NO_EVENT,
    TEXT_CONTROL_ENTER_PRESSED,
    TEXT_CONTROL_FOCUS_LOST,
    TEXT_CONTROL_INVALID_VALUE,
    TEXT_CONTROL_EDIT_CANCELED,
};

template<class T>
struct text_control_result : control_result<T>
{
    text_control_event_type event;
};

text_control_result<std::string> do_text_control(
    context& ctx,
    accessor<std::string> const& value,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS,
    region_id id = auto_id,
    int max_chars = -1);

template<class T>
struct converted_text_control_data
{
    converted_text_control_data() : valid(false) {}
    bool valid;
    T value;
    std::string text;
    // associated error message (if text doesn't parse)
    std::string message;
};

template<class T>
text_control_result<T> do_text_control(
    context& ctx,
    accessor<T> const& accessor,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS,
    region_id id = auto_id,
    int max_chars = -1)
{
    // TODO: Is this the right way to handle layout?
    layout spec = layout_spec;
    if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
        spec.flags |= BASELINE_Y;
    if ((spec.flags & X_ALIGNMENT_MASK) == 0)
        spec.flags |= LEFT;
    column_layout c(ctx, spec);
    converted_text_control_data<T>* data;
    get_data(ctx, &data);
    if (ctx.event->type == REFRESH_EVENT)
    {
        bool valid = accessor.is_valid();
        T value;
        if (valid)
            value = accessor.get();
        // The external value has changed, so update the internal one to
        // reflect it.
        if (data->valid != valid || valid && data->value != value)
        {
            data->valid = valid;
            if (valid)
                data->value = value;
            data->text = valid ? to_string(value) : "";
            data->message = "";
        }
    }
    text_control_result<std::string> r = do_text_control(
        ctx, inout(&data->text), spec, flags, id, max_chars);
    text_control_result<T> result;
    switch (r.event)
    {
     case TEXT_CONTROL_FOCUS_LOST:
     case TEXT_CONTROL_ENTER_PRESSED:
      {
        T new_value;
        if (from_string(&new_value, data->text, &data->message))
        {
            data->message = "";
            data->value = new_value;
            result.event = r.event;
            result.changed = true;
            result.new_value = new_value;
            accessor.set(new_value);
        }
        else
        {
            result.event = TEXT_CONTROL_INVALID_VALUE;
            result.changed = false;
        }
        break;
      }
     case TEXT_CONTROL_EDIT_CANCELED:
        result.event = TEXT_CONTROL_EDIT_CANCELED;
        result.changed = false;
        data->text = to_string(data->value);
        data->message = "";
        break;
     case TEXT_CONTROL_NO_EVENT:
     default:
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        break;
    }
    alia_if(!data->message.empty())
    {
        do_paragraph(ctx, data->message);
    }
    alia_end
    return result;
}

}

#endif
