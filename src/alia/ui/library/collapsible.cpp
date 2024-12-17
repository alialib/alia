#include <alia/ui/library/collapsible.hpp>

#include <alia/ui/events.hpp>
#include <alia/ui/layout/internals.hpp>

#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/widgets.hpp>

namespace alia {

template<class Content>
struct collapsible_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        return content.get_horizontal_requirements();
    }
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        auto const content_requirements
            = content.get_vertical_requirements(assigned_width);
        this->content_height = content_requirements.size;
        auto const visible_height = round_to_layout_scalar(
            float(this->content_height) * this->expansion);
        return layout_requirements{visible_height, 0, 0};
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        layout_box const& region = assignment.region;

        layout_requirements y
            = content.get_vertical_requirements(region.size[0]);

        content.set_relative_assignment(relative_layout_assignment{
            layout_box(
                region.corner, make_layout_vector(region.size[0], y.size)),
            y.size - y.descent});

        this->window = region;
    }

    void
    record_content_change() override
    {
        this->parent->record_content_change();
    }

    Content content;

    // expansion fraction (0 to 1)
    float expansion = 0;

    // The following are filled in during layout...

    // actual content height
    layout_scalar content_height;

    // window through which the content is visible
    layout_box window;
};

void
scoped_collapsible_content::begin(
    ui_context ctx,
    bool expanded,
    animated_transition const& transition,
    double const offset_factor,
    layout const& layout_spec)
{
    // TODO
    auto expansion = smooth(ctx, value(expanded ? 1.f : 0.f), transition);
    begin(ctx, read_signal(expansion), offset_factor, layout_spec);
}

void
scoped_collapsible_content::begin(
    ui_context ctx,
    float expansion,
    double const offset_factor,
    layout const& layout_spec)
{
    ctx_.reset(ctx);

    collapsible_layout_container<column_layout_container>* layout;
    get_cached_data(ctx, &layout);
    container_.begin(get_layout_traversal(ctx), layout);

    widget_id id = get_widget_id(ctx);

    if (is_refresh_pass(ctx))
    {
        // If the widget is expanding, ensure that it's visible.
        // TODO: Is this the correct place to do this?
        if (expansion > layout->expansion)
            make_widget_visible(ctx, id, ABRUPT);
        detect_layout_change(ctx, &layout->expansion, expansion);
    }
    else
    {
        if (get_event_category(ctx) == REGION_CATEGORY)
            do_box_region(ctx, id, layout->window);

        if (expansion != 0 && expansion != 1)
        {
            clipper_.begin(*get_layout_traversal(ctx).geometry);
            clipper_.set(box<2, double>(layout->window));
        }

        layout_scalar offset = round_to_layout_scalar(
            offset_factor * (1 - expansion) * layout->content_height);

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(
            make_vector<double>(0, -offset)
            + vector<2, double>(
                layout->cacher.relative_assignment.region.corner)));
    }

    do_content_ = expansion != 0;

    column_.begin(ctx, layout->content, layout_spec);
}

void
scoped_collapsible_content::end()
{
    if (ctx_)
    {
        column_.end();

        transform_.end();
        clipper_.end();

        container_.end();

        ctx_.reset();
    }
}

} // namespace alia
