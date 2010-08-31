#include <alia/color_control.hpp>
#include <alia/color_display.hpp>
#include <alia/context.hpp>
#include <alia/drop_down_list.hpp>
#include <alia/standard_colors.hpp>
#include <alia/control_macros.hpp>
#include <alia/text_display.hpp>
#include <alia/spacer.hpp>

namespace alia {

static int uninitialized = -2;
static int custom = -1;

struct color_control_data
{
    color_control_data() : valid(false) {}
    bool valid;
    rgb8 value;
    int index;
};

color_control_result
do_color_control(
    context& ctx,
    accessor<rgb8> const& accessor,
    layout const& layout_spec,
    flag_set flags)
{
    color_control_data& data = *get_data<color_control_data>(ctx);

    std::vector<named_color> const& colors = get_standard_colors();

    bool valid = accessor.is_valid();
    rgb8 value;
    if (valid)
        value = accessor.get();
    if (data.valid != valid || valid && data.value != value)
    {
        data.index = look_up_standard_color(value);
        data.valid = valid;
        data.value = value;
    }

    layout spec = layout_spec;
    if (!(spec.flags & Y_ALIGNMENT_MASK))
        spec.flags |= BASELINE_Y;

    // "custom..." is currently disabled because it crashes.
    // It's probably related to popup dismissal problems.

    drop_down_list<int> ddl(ctx, in(data.index, data.index >= 0), spec);
    alia_if(valid)
        do_color(ctx, value);
    alia_end
    do_text(ctx, valid ? (data.index >= 0 ? colors[data.index].name :
        std::string("custom")) : std::string(""), width(12, CHARS));
    if (ddl.do_list())
    {
        context& ctx = ddl.list_context();
        for (std::size_t i = 0; i < colors.size(); ++i)
        {
            named_color const& c = colors[i];
            ddl_item<int> item(ddl, int(i));
            row_layout r(ctx);
            do_color(ctx, c.color);
            do_text(ctx, c.name, width(12, CHARS));
        }
        //ddl_item<int> item(ddl, -1);
        //do_text(ctx, "custom...");
    }
    if (ddl.changed())
    {
        int index = ddl.selection();
        //if (index >= 0)
        {
            accessor.set(colors[index].color);
            color_control_result r;
            r.changed = true;
            r.new_value = colors[index].color;
            return r;
        }
        //else
        //{
        //    rgb8 custom;
        //    if (ctx.surface->ask_for_color(&custom, valid ? &value : 0))
        //    {
        //        accessor.set(custom);
        //        color_control_result r;
        //        r.changed = true;
        //        r.new_value = custom;
        //        return r;
        //    }
        //}
    }
    color_control_result r;
    r.changed = false;
    return r;
}

color_control_result
do_compressed_color_control(
    context& ctx,
    accessor<rgb8> const& accessor,
    layout const& layout_spec,
    flag_set flags)
{
    color_control_data& data = *get_data<color_control_data>(ctx);

    std::vector<named_color> const& colors = get_standard_colors();

    bool valid = accessor.is_valid();
    rgb8 value;
    if (valid)
        value = accessor.get();
    if (data.valid != valid || valid && data.value != value)
    {
        data.valid = valid;
        data.value = value;
        if (valid)
            data.index = look_up_standard_color(value);
    }

    layout spec = layout_spec;
    if (!(spec.flags & Y_ALIGNMENT_MASK))
        spec.flags |= BASELINE_Y;

    // "custom..." is currently disabled because it crashes.
    // It's probably related to popup dismissal problems.

    drop_down_list<int> ddl(ctx, in(data.index, data.valid), spec);
    alia_if(valid)
    {
        do_color(ctx, value);
    }
    alia_else
    {
        int color_size =
            get_font_metrics(ctx, ctx.pass_state.active_font).height +
            ctx.pass_state.padding_size[1] * 2;
        do_spacer(ctx, layout(size(float(color_size), float(color_size),
            PIXELS), PADDED));
    }
    alia_end
    if (ddl.do_list())
    {
        context& ctx = ddl.list_context();
        for (std::size_t i = 0; i < colors.size(); ++i)
        {
            named_color const& c = colors[i];
            ddl_item<int> item(ddl, i);
            row_layout r(ctx);
            do_color(ctx, c.color);
            do_text(ctx, c.name, width(12, CHARS));
        }
    }
    if (ddl.changed())
    {
        int index = ddl.selection();
        //if (index >= 0)
        {
            accessor.set(colors[index].color);
            color_control_result r;
            r.changed = true;
            r.new_value = colors[index].color;
            return r;
        }
    }
    color_control_result r;
    r.changed = false;
    return r;
}

//color_control_result
//do_compressed_color_control(
//    context& ctx,
//    accessor<rgb8> const& accessor,
//    layout const& layout_spec,
//    flag_set flags)
//{
//    color_control_data& data = *get_data<color_control_data>(ctx);
//
//    std::vector<named_color> const& colors = get_standard_colors();
//
//    bool valid = accessor.is_valid();
//    rgb8 value;
//    if (valid)
//        value = accessor.get();
//    if (data.valid != valid || valid && data.value != value)
//    {
//        data.index = look_up_standard_color(value);
//        data.valid = valid;
//        data.value = value;
//    }
//
//    layout spec = layout_spec;
//    if (!(spec.flags & Y_ALIGNMENT_MASK))
//        spec.flags |= BASELINE_Y;
//
//    drop_down_list ddl(ctx, in(unsigned(data.index), data.index >= 0),
//        colors.size(), spec);
//    alia_if(valid)
//        do_color(ctx, value);
//    alia_end
//    if (ddl.do_list())
//    {
//        context& ctx = ddl.get_list_context();
//        std::size_t n_colors = colors.size();
//        std::size_t const n_per_row = 5;
//        for (std::size_t row_n = 0; row_n < n_colors / n_per_row; ++row_n)
//        {
//            row_layout r(ctx);
//            std::size_t row_start = row_n * n_per_row;
//            std::size_t row_end = (std::min)(row_start + n_per_row, n_colors);
//            for (unsigned i = row_start; i != row_end; ++i)
//            {
//                named_color const& c = colors[i];
//                ddl_item item(ddl);
//                row_layout r(ctx);
//                do_color(ctx, c.color);
//            }
//        }
//    }
//    if (ddl.changed())
//    {
//        unsigned index = ddl.get_selection();
//        if (index < colors.size())
//        {
//            accessor.set(colors[index].color);
//            color_control_result r;
//            r.changed = true;
//            r.new_value = colors[index].color;
//            return r;
//        }
//    }
//    color_control_result r;
//    r.changed = false;
//    return r;
//}

}
