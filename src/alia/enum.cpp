#include <alia/enum.hpp>
#include <alia/drop_down_list.hpp>
#include <alia/text_display.hpp>
#include <alia/radio_button.hpp>
#include <alia/control_macros.hpp>
#include <alia/data.hpp>
#include <alia/context.hpp>

namespace alia {

enum_result<unsigned>
do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    std::vector<std::string> const& options,
    layout const& layout_spec,
    flag_set flags)
{
    if ((flags & USE_RADIO_GROUP) != 0)
    {
        alia_for (unsigned i = 0; i < options.size(); ++i)
        {
            if (do_radio_button(ctx, value, i, options[i]))
            {
                enum_result<unsigned> r;
                r.changed = true;
                r.new_value = i;
                return r;
            }
        }
        alia_end
        enum_result<unsigned> r;
        r.changed = false;
        return r;
    }
    else
    {
        layout spec = layout_spec;
        if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
            spec.flags |= BASELINE_Y;
        drop_down_list<unsigned> ddl(ctx, value, spec, flags);
        do_text(ctx, value.is_valid() && value.get() < options.size() ?
            options[value.get()] : std::string(""), width(12, CHARS));
        alia_if (ddl.do_list())
        {
            context& ctx = ddl.list_context();
            for (unsigned i = 0; i != unsigned(options.size()); ++i)
            {
                ddl_item<unsigned> item(ddl, i);
                do_text(ctx, options[i], width(12, CHARS));
            }
        }
        alia_end
        enum_result<unsigned> r;
        r.changed = ddl.changed();
        if (r.changed && ddl.has_selection())
            r.new_value = ddl.selection();
        return r;
    }
}

enum_result<unsigned>
do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    char const* const* options,
    unsigned n_options,
    layout const& layout_spec,
    flag_set flags)
{
    if ((flags & USE_RADIO_GROUP) != 0)
    {
        alia_for (unsigned i = 0; i < n_options; ++i)
        {
            if (do_radio_button(ctx, value, i, options[i]))
            {
                enum_result<unsigned> r;
                r.changed = true;
                r.new_value = i;
                return r;
            }
        }
        alia_end
        enum_result<unsigned> r;
        r.changed = false;
        return r;
    }
    else
    {
        layout spec = layout_spec;
        if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
            spec.flags |= BASELINE_Y;
        drop_down_list<unsigned> ddl(ctx, value, spec, flags);
        do_text(ctx, value.is_valid() && value.get() < n_options ?
            options[value.get()] : std::string(""), width(12, CHARS));
        if (ddl.do_list())
        {
            context& ctx = ddl.list_context();
            for (unsigned i = 0; i < n_options; ++i)
            {
                ddl_item<unsigned> item(ddl, i);
                do_text(ctx, options[i], width(12, CHARS));
            }
        }
        enum_result<unsigned> r;
        r.changed = ddl.changed();
        if (r.changed && ddl.has_selection())
            r.new_value = ddl.selection();
        return r;
    }
}

}
