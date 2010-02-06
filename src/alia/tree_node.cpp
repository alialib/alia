#include <alia/tree_node.hpp>
#include <alia/context.hpp>
#include <alia/node_expander.hpp>
#include <alia/spacer.hpp>
#include <alia/flags.hpp>
#include <alia/control_macros.hpp>
#include <alia/input_utils.hpp>

namespace alia {

void tree_node::begin(context& ctx, layout const& layout_spec, flag_set flags,
    accessor<int> const* expanded_accessor, region_id expander_id)
{
    if (!expanded_accessor)
    {
        int* expanded;
        if (get_data(ctx, &expanded))
            *expanded = (flags & INITIALLY_EXPANDED) != 0 ? 2 : 0;
        begin(ctx, layout_spec, flags, &inout(expanded), expander_id);
        return;
    }

    ctx_ = &ctx;
    flags_ = flags;

    if ((flags & NO_INDENT) == 0)
    {
        full_grid_.begin(ctx, layout_spec);
        top_row_.begin(full_grid_);
        do_children_ =
            (expanded_accessor->is_valid() ? expanded_accessor->get() : 0)
            && full_grid_.is_relevant();
        expander_result_ = do_tristate_node_expander(ctx, *expanded_accessor,
            NOT_PADDED, NO_FLAGS, expander_id);
        label_region_.begin(ctx, layout(GROW));
    }
    else
    {
        column_.begin(ctx, layout_spec);
        do_children_ =
            (expanded_accessor->is_valid() ? expanded_accessor->get() : 0)
            && column_.is_relevant();
        label_region_.begin(ctx);
        expander_result_ = do_tristate_node_expander(ctx, *expanded_accessor,
            NOT_PADDED, NO_FLAGS, expander_id);
    }
}

void tree_node::end_header()
{
    label_region_.end();
}

bool tree_node::do_children()
{
    end_header();
    if ((flags_ & NO_INDENT) == 0)
    {
        top_row_.end();
        alia_if_(*ctx_, do_children_)
        {
            bottom_row_.begin(full_grid_, layout(GROW));
            do_spacer(*ctx_);
            column_.begin(*ctx_, layout(GROW));
            return true;
        }
        alia_end
        return false;
    }
    else
        return do_children_;
}

void tree_node::end()
{
    column_.end();
    if ((flags_ & NO_INDENT) == 0)
    {
        bottom_row_.end();
        full_grid_.end();
    }
}

}
