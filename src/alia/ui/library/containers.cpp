#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

// COLLAPSIBLE CONTENT

struct collapsible_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    // expansion fraction (0 to 1)
    float expansion;

    // layout cacher
    layout_cacher cacher;

    // The following are filled in during layout...

    // actual content height
    layout_scalar content_height;

    // window through which the content is visible
    layout_box window;
};

layout_requirements
collapsible_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        layout_requirements r =
            alia::get_horizontal_requirements(ctx, *children);
        query.update(calculated_layout_requirements(r.minimum_size, 0, 0));
    }
    alia_end
    return query.result();
}

layout_requirements
collapsible_layout_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher, last_content_change,
        assigned_width);
    alia_if (query.update_required())
    {
        layout_scalar resolved_width =
            resolve_assigned_width(
                this->cacher.resolved_spec,
                assigned_width,
                this->get_horizontal_requirements(ctx));
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *children, resolved_width);
        layout_scalar content_height = y.minimum_size;
        layout_scalar visible_height =
            round_to_layout_scalar(float(content_height) * this->expansion);
        this->content_height = content_height;
        query.update(calculated_layout_requirements(visible_height, 0, 0));
    }
    alia_end
    return query.result();
}

void
collapsible_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher, last_content_change,
        assignment);
    alia_if (rra.update_required())
    {
        this->window = rra.resolved_assignment().region;

        layout_box const& region = rra.resolved_assignment().region;

        layout_requirements y = alia::get_vertical_requirements(
            ctx, *children, region.size[0]);

        layout_vector content_size =
            make_layout_vector(region.size[0], y.minimum_size);

        relative_layout_assignment assignment(
            layout_box(make_layout_vector(0, 0), content_size),
            y.minimum_size - y.minimum_descent);

        alia::set_relative_assignment(ctx, *children, assignment);
        rra.update();
    }
    alia_end
}

void collapsible_content::begin(
    ui_context& ctx, bool expanded, animated_transition const& transition,
    double const offset_factor, layout const& layout_spec)
{
    ctx_ = &ctx;

    collapsible_layout_container* layout;
    get_cached_data(ctx, &layout);

    container_.begin(get_layout_traversal(ctx), layout);

    float expansion = smooth_value(ctx, expanded ? 1.f : 0.f, transition);

    if (is_refresh_pass(ctx))
    {
        detect_layout_change(ctx, &layout->expansion, expansion);
        update_layout_cacher(get_layout_traversal(ctx), layout->cacher,
            layout_spec, FILL | UNPADDED);
    }
    else
    {
        if (expansion != 0 && expansion != 1)
        {
            clipper_.begin(*get_layout_traversal(ctx).geometry);
            clipper_.set(box<2,double>(layout->window));
        }

        layout_scalar offset = round_to_layout_scalar(
            offset_factor * (1 - expansion) * layout->content_height);

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(
            translation_matrix(make_vector<double>(0, -offset) +
                vector<2,double>(
                    layout->cacher.relative_assignment.region.corner)));
    }

    do_content_ = expansion != 0;

    layout_.begin(ctx);
}

void collapsible_content::end()
{
    if (ctx_)
    {
        layout_.end();

        transform_.end();
        clipper_.end();

        container_.end();

        ctx_ = 0;
    }
}

// TREE NODE

struct tree_node_data
{
    bool expanded;
};

void tree_node::begin(
    ui_context& ctx,
    layout const& layout_spec,
    ui_flag_set flags,
    optional_storage<bool> const& expanded,
    widget_id expander_id)
{
    ctx_ = &ctx;

    tree_node_data* data;
    if (get_data(ctx, &data))
    {
        if (flags & INITIALLY_EXPANDED)
            data->expanded = true;
        else
            data->expanded = false;
    }

    accessor_mux<indirect_accessor<bool>,inout_accessor<bool> > state =
        resolve_storage(expanded, &data->expanded);

    grid_.begin(ctx, layout_spec);
    row_.begin(grid_);

    is_expanded_ = state.is_gettable() ? state.get() : false;
    get_widget_id_if_needed(ctx, expander_id);
    expander_result_ =
        do_node_expander(ctx, state, default_layout, expander_id);

    label_region_.begin(ctx);
    hit_test_box_region(ctx, expander_id, label_region_.region());
}

void tree_node::end_header()
{
    label_region_.end();
}

bool tree_node::do_children()
{
    ui_context& ctx = *ctx_;
    end_header();
    row_.end();
    content_.begin(ctx, is_expanded_);
    bool do_content = content_.do_content();
    alia_if (do_content)
    {
        row_.begin(grid_, layout(GROW));
        do_spacer(ctx);
        column_.begin(ctx, layout(GROW));
    }
    alia_end
    return do_content;
}

void tree_node::end()
{
    column_.end();
    row_.end();
    content_.end();
    grid_.end();
}

// RESIZABLE CONTENT

struct draggable_separator_data
{
    keyed_data<layout_vector> size;
    layout_leaf layout_node;
    caching_renderer_data rendering;
    int drag_start_delta;
};

bool do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec, ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(draggable_separator_data)

    get_widget_id_if_needed(ctx, id);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data.size, *ctx.style.id);
        if (!is_valid(data.size))
        {
            absolute_length spec =
                get_property(ctx, "separator-width", INHERITED_PROPERTY,
                    absolute_length(0.1f, EM));
            set(data.size, as_layout_size(make_vector(
                resolve_absolute_length(get_layout_traversal(ctx), 0, spec),
                resolve_absolute_length(get_layout_traversal(ctx), 1, spec))));
        }
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(get(data.size), 0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStrokeWidth(2);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            set_color(paint, get_color_property(ctx, "separator-color"));
            renderer.canvas().drawLine(
                SkIntToScalar(1), SkIntToScalar(1),
                layout_scalar_as_skia_scalar(region.size[0] - 1),
                layout_scalar_as_skia_scalar(region.size[1] - 1),
                paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }

     case REGION_CATEGORY:
      {
        layout_box region = data.layout_node.assignment().region;
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;
        // Add a couple of pixels to make it easier to click on.
        region.corner[drag_axis] -= 1;
        region.size[drag_axis] += 2;
        do_box_region(ctx, id, region, flags & HORIZONTAL ?
            UP_DOWN_ARROW_CURSOR : LEFT_RIGHT_ARROW_CURSOR);
        break;
      }

     case INPUT_CATEGORY:
      {
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;

        if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            int current_width = width.is_gettable() ? get(width) : 0;
            data.drag_start_delta = (flags & FLIPPED) ?
                current_width + position : position - current_width;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            set(width, (flags & FLIPPED) ?
                data.drag_start_delta - position :
                position - data.drag_start_delta);
            return true;
        }
        break;
      }
    }
    return false;
}

void resizable_content::begin(
    ui_context& ctx, accessor<int> const& size, ui_flag_set flags)
{
    ctx_ = &ctx;
    id_ = get_widget_id(ctx);
    flags_ = flags;
    size_ = get(size);

    if (flags & PREPEND)
        do_draggable_separator(ctx, size, UNPADDED, flags | FLIPPED);

    if (flags & HORIZONTAL)
        layout_.begin(ctx, 1, alia::height(float(size_), PIXELS));
    else
        layout_.begin(ctx, 0, alia::width(float(size_), PIXELS));

    // It's possible that the content will be too big for the requested size
    // and the layout engine will force the container to a larger size.
    // If this happens, we have to record that as the real size.
    if (detect_event(ctx, MOUSE_HIT_TEST_EVENT))
    {
        unsigned drag_axis = (flags & HORIZONTAL) ? 1 : 0;
        int new_size = layout_.region().size[drag_axis];
        if (new_size != size_)
            set(size, new_size);
    }

    handle_set_value_events(ctx, id_, size);
}
void resizable_content::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        layout_.end();
        if (!(flags_ & PREPEND) && !ctx.pass_aborted)
        {
            if (do_draggable_separator(ctx, inout(&size_), UNPADDED, flags_))
                issue_set_value_event(ctx, id_, size_);
        }
        ctx_ = 0;
    }
}

// ACCORDIONS

void accordion::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    if (get_data(ctx, &selection_))
        *selection_ = 0;
    index_ = 0;
    layout_.begin(ctx, layout_spec);
}

void accordion::end()
{
    if (ctx_)
    {
        layout_.end();
        ctx_ = 0;
    }
}

void accordion_section::begin(ui_context& ctx, accessor<bool> const& selected)
{
    ctx_ = &ctx;
    is_selected_ = is_gettable(selected) ? get(selected) : false;
    panel_.begin(ctx, text("accordion-header"), default_layout,
        is_selected_ ? SELECTED : NO_FLAGS);
    if (panel_.clicked())
        selected.set(true);
}
void accordion_section::begin(accordion& parent)
{
    begin(*parent.ctx_,
        make_indexed_accessor(inout(parent.selection_), parent.index_++));
}
bool accordion_section::do_content()
{
    ui_context& ctx = *ctx_;
    panel_.end();
    content_.begin(ctx, is_selected_, animated_transition(default_curve, 400),
        0.1);
    return content_.do_content();
}
void accordion_section::end()
{
    if (ctx_)
    {
        content_.end();
        ctx_ = 0;
    }
}

// CLAMPED CONTENT / HEADER

void clamped_content::begin(
    ui_context& ctx,
    getter<string> const& background_style,
    getter<string> const& content_style,
    absolute_size const& max_size,
    layout const& layout_spec,
    ui_flag_set flags)
{
    ctx_ = &ctx;
    background_.begin(ctx, background_style, layout_spec,
        NO_INTERNAL_PADDING |
        (max_size[0].length > 0 ? RESERVE_VERTICAL : NO_FLAGS) |
        (max_size[1].length > 0 ? RESERVE_HORIZONTAL : NO_FLAGS));
    clamp_.begin(ctx, max_size, GROW | UNPADDED);
    content_.begin(ctx, content_style, UNPADDED, flags);
}
void clamped_content::end()
{
    if (ctx_)
    {
        content_.end();
        clamp_.end();
        background_.end();
        ctx_ = 0;
    }
}

void clamped_header::begin(
    ui_context& ctx,
    getter<string> const& background_style,
    getter<string> const& header_style,
    absolute_size const& max_size,
    layout const& layout_spec,
    ui_flag_set flags)
{
    ctx_ = &ctx;
    background_.begin(ctx, background_style, layout_spec,
        NO_INTERNAL_PADDING |
        (max_size[0].length > 0 ?
            NO_VERTICAL_SCROLL | RESERVE_VERTICAL :
            NO_FLAGS) |
        (max_size[1].length > 0 ?
            NO_HORIZONTAL_SCROLL | RESERVE_HORIZONTAL :
            NO_FLAGS));
    clamp_.begin(ctx, max_size, GROW | UNPADDED);
    header_.begin(ctx, header_style, UNPADDED, flags);
}
void clamped_header::end()
{
    if (ctx_)
    {
        header_.end();
        clamp_.end();
        background_.end();
        ctx_ = 0;
    }
}

// TABS

void tab_strip::begin(ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags)
{
    ctx_ = &ctx;
    style_.begin(ctx, text("tab-strip"));
    layering_.begin(ctx, layout_spec);
    {
        panel background(ctx, text("tab"));
    }
    tab_container_.begin(ctx, (flags & VERTICAL) ? 1 : 0);
}

void tab_strip::end()
{
    if (ctx_)
    {
        tab_container_.end();
        layering_.end();
        style_.end();
        ctx_ = 0;
    }
}

void tab::begin(ui_context& ctx, accessor<bool> const& selected)
{
    ctx_ = &ctx;
    is_selected_ = is_gettable(selected) ? get(selected) : false;
    panel_.begin(ctx, text("tab"), default_layout,
        is_selected_ ? SELECTED : NO_FLAGS);
    if (panel_.clicked())
    {
        selected.set(true);
        end_pass(ctx);
    }
}
void tab::end()
{
    if (ctx_)
    {
        panel_.end();
        ctx_ = 0;
    }
}

void do_tab(ui_context& ctx, accessor<bool> const& selected,
    getter<string> const& label)
{
    tab t(ctx, selected);
    do_text(ctx, label);
}

}
