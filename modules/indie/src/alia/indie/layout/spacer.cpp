#include <alia/indie/layout/spacer.hpp>

#include <alia/indie/layout/utilities.hpp>

namespace alia { namespace indie {

struct spacer_node : layout_leaf
{
    void
    render(render_event&) override
    {
    }

    void
    hit_test(hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(event_context) override
    {
    }
};

void
do_spacer(indie::context ctx, layout const& layout_spec)
{
    spacer_node* node;
    get_cached_data(ctx, &node);

    if (get<indie::traversal_tag>(ctx).layout.is_refresh_pass)
    {
        node->refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, node);
    }
}

}} // namespace alia::indie
