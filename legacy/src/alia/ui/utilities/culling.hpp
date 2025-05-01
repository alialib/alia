#ifndef ALIA_UI_UTILITIES_CULLING_HPP
#define ALIA_UI_UTILITIES_CULLING_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>

namespace alia {

template<class Layout>
struct scoped_culling_block
{
    scoped_culling_block()
    {
    }
    scoped_culling_block(
        ui_context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }
    ~scoped_culling_block()
    {
        end();
    }

    void
    begin(ui_context ctx, layout const& layout_spec = default_layout);

    void
    end();

    bool
    is_relevant() const
    {
        return is_relevant_;
    }

 private:
    optional_context<ui_context> ctx_;
    scoped_component_container scc_;
    Layout layout_;
    bool is_relevant_;
};

template<class Layout>
void
scoped_culling_block<Layout>::begin(ui_context ctx, layout const& layout_spec)
{
    ctx_.reset(ctx);
    scc_.begin(ctx);
    layout_.begin(ctx, layout_spec);
    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            is_relevant_ = true;
            break;
        case RENDER_CATEGORY:
            is_relevant_ = is_visible(
                get_geometry_context(ctx), box<2, double>(layout_.region()));
            break;
        case REGION_CATEGORY:
            if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT
                || get_event_type(ctx) == WHEEL_HIT_TEST_EVENT)
            {
                is_relevant_ = is_mouse_inside_box(
                    ctx, box<2, double>(layout_.region()));
                break;
            }
            [[fallthrough]];
        default:
            is_relevant_ = scc_.is_on_route();
    }
}

template<class Layout>
void
scoped_culling_block<Layout>::end()
{
    if (ctx_)
    {
        layout_.end();
        scc_.end();
        ctx_.reset();
    }
}

template<class Layout, class Content>
void
culling_block(ui_context ctx, layout const& layout_spec, Content&& content)
{
    scoped_culling_block<Layout> block(ctx, layout_spec);
    {
        event_dependent_if_block if_block(
            get_data_traversal(ctx), block.is_relevant());
        if (block.is_relevant())
        {
            content();
        }
    }
}

} // namespace alia

#endif
