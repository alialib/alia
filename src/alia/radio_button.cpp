#include <alia/radio_button.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/linear_layout.hpp>
#include <alia/text_display.hpp>
#include <alia/flags.hpp>
#include <alia/widget_state.hpp>

namespace alia {

struct radio_button_data
{
    radio_button_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    radio_button_data& data = *get_data<radio_button_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_radio_button_size(data.artist_data,
            value.is_valid() ? value.get() == index : false);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_radio_button(data.artist_data,
            value.is_valid() ? value.get() == index : false,
            data.layout_data.assigned_region.corner,
            get_widget_state(ctx, id, true, data.key_state == 1));
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        add_to_focus_order(ctx, id);
        bool selected = detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        if (selected)
        {
            value.set(index);
            return true;
        }
        break;
      }
    }
    return false;
}

bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    char const* text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    layout ls = layout_spec;
    if ((ls.flags & Y_ALIGNMENT_MASK) == 0)
        ls.flags |= BASELINE_Y;
    if ((ls.flags & X_ALIGNMENT_MASK) == 0)
        ls.flags |= LEFT;
    // TODO: fix alignment of radio button and text
    row_layout row(ctx, ls);
    bool result =
        do_radio_button(ctx, value, index, default_layout, flags, id);
    do_text(ctx, text);
    do_region(ctx, id, add_border(row.get_region(),
        -ctx.pass_state.style->padding_size));
    return result;
}

bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    std::string const& text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    return do_radio_button(ctx, value, index, text.c_str(), layout_spec, flags,
        id);
}

//radio_button_group::radio_button_group(context& context,
//    value_parameter<unsigned> const& value)
//{
//    init(context, value);
//}
//radio_button_group::radio_button_group(value_parameter<int> const& value)
//{
//    init(get_active_context(), value);
//}
//
//radio_button_group::init(context& context, value_parameter<bool> const& value)
//{
//    context_ = &context;
//    value_ = value;
//    changed_ = false;
//    index_ = 0;
//}
//
//bool radio_button_group::do_button(radio_button_data& data,
//    std::string const& text, layout const& layout_spec, bool enabled)
//{
//    bool changed = do_radio_button(*context_, data, value_, index_++,
//        text, layout_spec, enabled);
//
//    if (changed)
//        changed_ = true;
//
//    return changed;
//}
//
//bool radio_button_group::do_button(std::string const& text,
//    layout const& layout_spec, bool enabled)
//{
//    do_button(get_widget_data<radio_button_data>(*context_), text,
//        layout_spec, enabled);
//}

}
