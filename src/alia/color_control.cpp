#include <alia/color_control.hpp>
#include <alia/color_display.hpp>
#include <alia/context.hpp>
#include <alia/drop_down_list.hpp>
#include <alia/standard_colors.hpp>
#include <alia/control_macros.hpp>
#include <alia/text_display.hpp>

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

color_control_result do_color_control(
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

    drop_down_list ddl(ctx, in(unsigned(data.index), data.index >= 0),
        colors.size()/* + 1*/, spec);
    alia_if(valid)
        do_color(ctx, value);
    alia_end
    do_text(ctx, valid ? (data.index >= 0 ? colors[data.index].name :
        std::string("custom")) : std::string(""), width(12, CHARS));
    if (ddl.do_list())
    {
        context& ctx = ddl.get_list_context();
        for (unsigned i = 0; i < colors.size(); ++i)
        {
            named_color const& c = colors[i];
            ddl_item item(ddl);
            row_layout r(ctx);
            do_color(ctx, c.color);
            do_text(ctx, c.name, width(12, CHARS));
        }
        //ddl_item item(ddl);
        //do_text(ctx, "custom...");
    }
    if (ddl.changed())
    {
        unsigned index = ddl.get_selection();
        if (index < colors.size())
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

}
