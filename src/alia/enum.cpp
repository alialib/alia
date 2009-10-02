#include <alia/enum.hpp>
#include <alia/drop_down_list.hpp>
#include <alia/text_display.hpp>

#include <alia/context.hpp>

namespace alia {

enum_result<unsigned> do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    std::vector<std::string> const& options,
    unsigned flags,
    layout const& layout_spec)
{
    layout spec = layout_spec;
    if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
        spec.flags |= BASELINE_Y;
    drop_down_list ddl(ctx, value, options.size(), flags, spec);
    do_text(ctx, value.is_valid() && value.get() < options.size() ?
        options[value.get()] : std::string(""), 0, width(12, CHARS));
    if (ddl.do_list())
    {
        context& ctx = ddl.get_list_context();
        for (unsigned i = 0; i < options.size(); ++i)
        {
            ddl_item item(ddl);
            do_text(ctx, options[i], 0, width(12, CHARS));
        }
    }
    enum_result<unsigned> r;
    r.changed = ddl.changed();
    if (r.changed)
        r.new_value = ddl.get_selection();
    return r;
}

enum_result<unsigned> do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    char const* const* options,
    unsigned n_options,
    unsigned flags,
    layout const& layout_spec)
{
    layout spec = layout_spec;
    if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
        spec.flags |= BASELINE_Y;
    drop_down_list ddl(ctx, value, n_options, flags, spec);
    do_text(ctx, value.is_valid() && value.get() < n_options ?
        options[value.get()] : std::string(""), 0, width(12, CHARS));
    if (ddl.do_list())
    {
        context& ctx = ddl.get_list_context();
        for (unsigned i = 0; i < n_options; ++i)
        {
            ddl_item item(ddl);
            do_text(ctx, options[i], 0, width(12, CHARS));
        }
    }
    enum_result<unsigned> r;
    r.changed = ddl.changed();
    if (r.changed)
        r.new_value = ddl.get_selection();
    return r;
}

}
