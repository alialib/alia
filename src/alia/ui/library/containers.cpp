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

    collapsible_layout_container() : expansion(0) {}
};

layout_requirements
collapsible_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        query.update(fold_horizontal_child_requirements(ctx, children));
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
        layout_scalar content_height = y.size;
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
        layout_box const& region = rra.resolved_assignment().region;

        layout_requirements y = alia::get_vertical_requirements(
            ctx, *children, region.size[0]);

        layout_vector content_size =
            make_layout_vector(region.size[0], y.size);

        relative_layout_assignment assignment(
            layout_box(make_layout_vector(0, 0), content_size),
            y.size - y.descent);

        alia::set_relative_assignment(ctx, *children, assignment);
        rra.update();
    }
    alia_end
    this->window = rra.resolved_assignment().region;
}

void collapsible_content::begin(
    ui_context& ctx, bool expanded, animated_transition const& transition,
    double const offset_factor, layout const& layout_spec)
{
    float expansion = smooth_raw_value(ctx, expanded ? 1.f : 0.f, transition);
    begin(ctx, expansion, offset_factor, layout_spec);
}

void collapsible_content::begin(
    ui_context& ctx, float expansion,
    double const offset_factor, layout const& layout_spec)
{
    ctx_ = &ctx;

    collapsible_layout_container* layout;
    get_cached_data(ctx, &layout);

    widget_id id = get_widget_id(ctx);

    container_.begin(get_layout_traversal(ctx), layout);

    if (is_refresh_pass(ctx))
    {
        // If the widget is expanding, ensure that it's visible.
        if (expansion > layout->expansion)
            make_widget_visible(ctx, id, MAKE_WIDGET_VISIBLE_ABRUPTLY);
        detect_layout_change(ctx, &layout->expansion, expansion);
        update_layout_cacher(get_layout_traversal(ctx), layout->cacher,
            layout_spec, FILL | UNPADDED);
    }
    else
    {
        if (ctx.event->category == REGION_CATEGORY)
            do_box_region(ctx, id, layout->window);

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

// HORIZONTAL COLLAPSIBLE CONTENT

struct horizontal_collapsible_layout_container : layout_container
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

    // actual content width
    layout_scalar content_width;

    // window through which the content is visible
    layout_box window;

    horizontal_collapsible_layout_container() : expansion(0) {}
};

layout_requirements
horizontal_collapsible_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        layout_scalar content_width = get_max_child_width(ctx, children);
        layout_scalar visible_width =
            round_to_layout_scalar(float(content_width) * this->expansion);
        this->content_width = content_width;
        query.update(calculated_layout_requirements(visible_width, 0, 0));
    }
    alia_end
    return query.result();
}

layout_requirements
horizontal_collapsible_layout_container::get_vertical_requirements(
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
        query.update(fold_vertical_child_requirements(ctx, children,
            resolved_width));
    }
    alia_end
    return query.result();
}

void
horizontal_collapsible_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher, last_content_change,
        assignment);
    alia_if (rra.update_required())
    {
        this->window = rra.resolved_assignment().region;

        layout_box const& region = rra.resolved_assignment().region;

        layout_scalar content_width = get_max_child_width(ctx, children);

        layout_vector content_size =
            make_layout_vector(content_width, region.size[1]);

        relative_layout_assignment assignment(
            layout_box(make_layout_vector(0, 0), content_size),
            assignment.baseline_y);

        alia::set_relative_assignment(ctx, *children, assignment);
        rra.update();
    }
    alia_end
}

void horizontal_collapsible_content::begin(
    ui_context& ctx, bool expanded, animated_transition const& transition,
    double const offset_factor, layout const& layout_spec)
{
    float expansion = smooth_raw_value(ctx, expanded ? 1.f : 0.f, transition);
    begin(ctx, expansion, offset_factor, layout_spec);
}

void horizontal_collapsible_content::begin(
    ui_context& ctx, float expansion,
    double const offset_factor, layout const& layout_spec)
{
    ctx_ = &ctx;

    horizontal_collapsible_layout_container* layout;
    get_cached_data(ctx, &layout);

    widget_id id = get_widget_id(ctx);

    container_.begin(get_layout_traversal(ctx), layout);

    if (is_refresh_pass(ctx))
    {
        // If the widget is expanding, ensure that it's visible.
        if (expansion > layout->expansion)
            make_widget_visible(ctx, id, MAKE_WIDGET_VISIBLE_ABRUPTLY);
        detect_layout_change(ctx, &layout->expansion, expansion);
        update_layout_cacher(get_layout_traversal(ctx), layout->cacher,
            layout_spec, FILL | UNPADDED);
    }
    else
    {
        if (ctx.event->category == REGION_CATEGORY)
            do_box_region(ctx, id, layout->window);

        if (expansion != 0 && expansion != 1)
        {
            clipper_.begin(*get_layout_traversal(ctx).geometry);
            clipper_.set(box<2,double>(layout->window));
        }

        layout_scalar offset = round_to_layout_scalar(
            offset_factor * (1 - expansion) * layout->content_width);

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(
            translation_matrix(make_vector<double>(-offset, 0) +
                vector<2,double>(
                    layout->cacher.relative_assignment.region.corner)));
    }

    do_content_ = expansion != 0;

    layout_.begin(ctx);
}

void horizontal_collapsible_content::end()
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
    tree_node_flag_set flags,
    optional_storage<bool> const& expanded,
    widget_id expander_id)
{
    ctx_ = &ctx;

    tree_node_data* data;
    if (get_data(ctx, &data))
    {
        if (flags & TREE_NODE_INITIALLY_EXPANDED)
            data->expanded = true;
        else
            data->expanded = false;
    }

    accessor_mux<input_accessor<bool>,indirect_accessor<bool>,
        inout_accessor<bool> > state =
        resolve_storage(expanded, &data->expanded);

    grid_.begin(ctx, layout_spec);
    row_.begin(grid_);

    is_expanded_ = state.is_gettable() ? state.get() : false;
    get_widget_id_if_needed(ctx, expander_id);
    expander_result_ =
        do_unsafe_node_expander(ctx, state, default_layout,
            (flags & TREE_NODE_DISABLED ? SIMPLE_CONTROL_DISABLED : NO_FLAGS),
            expander_id);

    label_region_.begin(ctx, BASELINE_Y | GROW_X);
    hit_test_box_region(ctx, expander_id, label_region_.region());
}

bool tree_node::do_children()
{
    ui_context& ctx = *ctx_;
    label_region_.end();
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

ALIA_DEFINE_FLAG_TYPE(draggable_separator)
ALIA_DEFINE_FLAG(draggable_separator, 0x1, DRAGGABLE_SEPARATOR_HORIZONTAL)
ALIA_DEFINE_FLAG(draggable_separator, 0x2, DRAGGABLE_SEPARATOR_VERTICAL)
ALIA_DEFINE_FLAG(draggable_separator, 0x4, DRAGGABLE_SEPARATOR_FLIPPED)

static bool
do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec, unsigned axis,
    draggable_separator_flag_set flags, widget_id id)
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
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "draggable-separator");
            absolute_length spec =
                get_property(path, "width", UNINHERITED_PROPERTY,
                    absolute_length(1, PIXELS));
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
            paint.setStrokeWidth(layout_scalar_as_skia_scalar(
                get(data.size)[0]));
            paint.setStrokeCap(SkPaint::kSquare_Cap);
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "draggable-separator");
            set_color(paint, get_color_property(path, "color"));
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
        unsigned const drag_axis = 1 - axis;
        // Add a couple of pixels to make it easier to click on.
        region.corner[drag_axis] -= 1;
        region.size[drag_axis] += 2;
        do_box_region(ctx, id, region, drag_axis != 0 ?
            UP_DOWN_ARROW_CURSOR : LEFT_RIGHT_ARROW_CURSOR);
        break;
      }

     case INPUT_CATEGORY:
      {
        unsigned const drag_axis = 1 - axis;
        if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            int position = ctx.system->input.mouse_position[drag_axis];
            int current_width = width.is_gettable() ? get(width) : 0;
            data.drag_start_delta = (flags & DRAGGABLE_SEPARATOR_FLIPPED) ?
                current_width + position : position - current_width;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            int position = ctx.system->input.mouse_position[drag_axis];
            set(width, (flags & DRAGGABLE_SEPARATOR_FLIPPED) ?
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
    ui_context& ctx, accessor<int> const& size,
    resizable_content_flag_set flags)
{
    ctx_ = &ctx;
    id_ = get_widget_id(ctx);
    flags_ = flags;
    active_ = true;

    unsigned drag_axis = (flags & RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR) ? 1 : 0;

    // Clamp the content size to be no bigger than half the size of the surface.
    // This prevents the situation where the content is so large that the user can't
    // actually grab the edge of it to shrink it.
    // (It also happens that this limit is more-than-enough for our use cases.)
    layout_vector surface_size = layout_vector(ctx.system->surface_size);
    size_ = (std::min)(size.is_gettable() ? get(size) : 0, surface_size[drag_axis] / 2);

    if (flags & RESIZABLE_CONTENT_PREPEND_SEPARATOR)
    {
        state_proxy<int> size_proxy(size_);
        do_draggable_separator(ctx, make_accessor(size_proxy), UNPADDED,
            (flags & RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR) ? 0 : 1,
            DRAGGABLE_SEPARATOR_FLIPPED, id_);
        if (size_proxy.was_set())
        {
            // Apply clamping again.
            set(size, (std::min)(size_proxy.get(), surface_size[drag_axis] / 2));
            end_pass(ctx);
        }
    }

    if (flags & RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR)
    {
        layout_.begin(ctx, VERTICAL_LAYOUT,
            height(float(size_), UNMAGNIFIED_PIXELS));
    }
    else
    {
        layout_.begin(ctx, HORIZONTAL_LAYOUT,
            width(float(size_), UNMAGNIFIED_PIXELS));
    }

    // It's possible that the content will be too big for the requested size
    // and the layout engine will force the container to a larger size.
    // If this happens, we have to record that as the real size.
    if (detect_event(ctx, MOUSE_HIT_TEST_EVENT))
    {
        int new_size = layout_.region().size[drag_axis];
        if (new_size != size_)
            set(size, new_size);
    }

    handle_set_value_events(ctx, id_, size);
}
void resizable_content::end()
{
    ui_context& ctx = *ctx_;
    if (ctx.pass_aborted)
        return;
    alia_if (active_)
    {
        layout_.end();
        if (!(flags_ & RESIZABLE_CONTENT_PREPEND_SEPARATOR))
        {
            state_proxy<int> size_proxy(size_);
            do_draggable_separator(ctx, make_accessor(size_proxy), UNPADDED,
                (flags_ & RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR) ? 0 : 1,
                NO_FLAGS, id_);
            if (size_proxy.was_set())
            {
                // Apply clamping again.
                layout_vector surface_size = layout_vector(ctx.system->surface_size);
                unsigned drag_axis =
                    (flags_ & RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR) ? 1 : 0;
                issue_set_value_event(ctx, id_,
                    (std::min)(size_proxy.get(), surface_size[drag_axis] / 2));
             }
        }
        active_ = false;
    }
    alia_end
}

// ACCORDIONS

// vertical

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
        is_selected_ ? PANEL_SELECTED : NO_FLAGS);
    clicked_ = panel_.clicked();
    if (clicked_)
        selected.set(true);
}
void accordion_section::begin(accordion& parent)
{
    begin(*parent.ctx_,
        make_radio_accessor(inout(parent.selection_), in(parent.index_++)));
}
bool accordion_section::do_content()
{
    ui_context& ctx = *ctx_;
    panel_.end();
    content_.begin(ctx, is_selected_,
        animated_transition(default_curve, 400), 0.9);
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

// horizontal

void horizontal_accordion::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    if (get_data(ctx, &selection_))
        *selection_ = 0;
    index_ = 0;
    layout_.begin(ctx, layout_spec);
}

void horizontal_accordion::end()
{
    if (ctx_)
    {
        layout_.end();
        ctx_ = 0;
    }
}

void horizontal_accordion_section::begin(
    ui_context& ctx, accessor<bool> const& selected)
{
    ctx_ = &ctx;
    is_selected_ = is_gettable(selected) ? get(selected) : false;
    panel_.begin(ctx, text("horizontal-accordion-header"), default_layout,
        is_selected_ ? PANEL_SELECTED : NO_FLAGS);
    clicked_ = panel_.clicked();
    if (clicked_)
        selected.set(true);
}
void horizontal_accordion_section::begin(horizontal_accordion& parent)
{
    begin(*parent.ctx_,
        make_radio_accessor(inout(parent.selection_), in(parent.index_++)));
}
bool horizontal_accordion_section::do_content()
{
    ui_context& ctx = *ctx_;
    panel_.end();
    content_.begin(ctx, is_selected_,
        animated_transition(default_curve, 400), 0.9);
    return content_.do_content();
}
void horizontal_accordion_section::end()
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
    accessor<string> const& background_style,
    accessor<string> const& content_style,
    absolute_size const& max_size,
    layout const& layout_spec,
    panel_flag_set flags)
{
    ctx_ = &ctx;
    background_.begin(ctx, background_style, layout_spec,
        PANEL_NO_INTERNAL_PADDING |
        (max_size[0].length > 0 ?
            PANEL_RESERVE_VERTICAL_SCROLLBAR : NO_FLAGS) |
        (max_size[1].length > 0 ?
            PANEL_RESERVE_HORIZONTAL_SCROLLBAR : NO_FLAGS));
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
    accessor<string> const& background_style,
    accessor<string> const& header_style,
    absolute_size const& max_size,
    layout const& layout_spec,
    panel_flag_set flags)
{
    ctx_ = &ctx;
    background_.begin(ctx, background_style, layout_spec,
        PANEL_NO_INTERNAL_PADDING |
        (max_size[0].length > 0 ?
            PANEL_NO_VERTICAL_SCROLLING | PANEL_RESERVE_VERTICAL_SCROLLBAR :
            NO_FLAGS) |
        (max_size[1].length > 0 ?
            PANEL_NO_HORIZONTAL_SCROLLING |
                PANEL_RESERVE_HORIZONTAL_SCROLLBAR :
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
    tab_strip_flag_set flags)
{
    ctx_ = &ctx;
    style_.begin(ctx, text("tab-strip"));
    alia_if (get_cached_property(ctx, "add-background-tab",
        UNINHERITED_PROPERTY, false))
    {
        layering_.begin(ctx, layout_spec);
        panel background(ctx, text("tab"));
    }
    alia_end
    tab_container_.begin(ctx,
        (flags & TAB_STRIP_VERTICAL) ? VERTICAL_LAYOUT : HORIZONTAL_LAYOUT);
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
        is_selected_ ? PANEL_SELECTED : NO_FLAGS);
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
    accessor<string> const& label)
{
    tab t(ctx, selected);
    do_text(ctx, label);
}

void form::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    grid_.begin(ctx, layout_spec, absolute_length(1, CHARS));
}
void form::end()
{
    if (ctx_)
    {
        grid_.end();
        ctx_ = 0;
    }
}

void do_form_section_heading(form& form, accessor<string> const& label)
{
    ui_context& ctx = form.context();
    grid_row row(form.grid());
    do_heading(ctx, text("form-section-heading"), label, RIGHT);
}

void form_field::begin(form& form, accessor<string> const& label)
{
    ui_context& ctx = form.context();
    form_ = &form;
    row_.begin(form.grid());
    {
        auto label_size =
            get_cached_property(ctx, "form-label-size",
                INHERITED_PROPERTY, size(15, 2, CHARS));
        column_layout label_region(ctx, layout(label_size, BASELINE_Y));
        alia_if (is_gettable(label) && !get(label).empty())
        {
            do_styled_text(ctx, text("form-label"), printf(ctx, "%s:", label), RIGHT);
        }
        alia_end
    }
    contents_.begin(ctx, GROW);
}
void form_field::end()
{
    if (form_)
    {
        contents_.end();
        row_.end();
        form_ = 0;
    }
}

// This is the "empty form_field"
void empty_form_field::begin(form& form)
{
    ui_context& ctx = form.context();
    form_ = &form;
    row_.begin(form.grid());
    do_spacer(ctx);
    contents_.begin(ctx, GROW);
}
void empty_form_field::end()
{
    if (form_)
    {
        contents_.end();
        row_.end();
        form_ = 0;
    }
}

void form_buttons::begin(form& form)
{
    ui_context& ctx = form.context();
    form_ = &form;
    row_.begin(form.grid());
    do_spacer(ctx);
    contents_.begin(ctx, GROW);
}
void form_buttons::end()
{
    if (form_)
    {
        contents_.end();
        row_.end();
        form_ = 0;
    }
}

// TRANSITIONING CONTAINERS

struct transitioning_layout_content_data
{
    layout_scalar content_height;
    float presence;
    transitioning_layout_content_data* next;
};

struct transitioning_layout_container : layout_container
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

    // list of nodes that this container transitions between
    transitioning_layout_content_data* nodes;

    // layout cacher
    layout_cacher cacher;

    // The following are filled in during layout...

    // window through which the content is visible
    layout_box window;

    transitioning_layout_container() : nodes(0) {}
};

layout_requirements
transitioning_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        query.update(fold_horizontal_child_requirements(ctx, children));
    }
    alia_end
    return query.result();
}

layout_scalar static
get_container_height(transitioning_layout_content_data* nodes)
{
    float weighted_height = 0;
    float total_weight = 0;
    for (auto* i = nodes; i; i = i->next)
    {
        weighted_height += i->presence * i->content_height;
        total_weight += i->presence;
    }
    return
        total_weight > 0.0001
      ? layout_scalar(weighted_height / total_weight + 0.5)
      : as_layout_size(0);
}

float static
get_total_presence(transitioning_layout_content_data* nodes)
{
    float sum = 0;
    for (auto* i = nodes; i; i = i->next)
        sum += i->presence;
    return sum;
}

float static
get_presence_through_node(transitioning_layout_content_data* nodes,
    transitioning_layout_content_data* node)
{
    float sum = 0;
    for (auto* i = nodes; i; i = i->next)
    {
        sum += i->presence;
        if (i == node)
            break;
    }
    return sum;
}

bool static
is_in_transition(transitioning_layout_content_data* nodes)
{
    for (auto* i = nodes; i; i = i->next)
    {
        if (i->presence > 0 && i->presence < 1)
            return true;
    }
    return false;
}

layout_requirements
transitioning_layout_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query
        query(ctx, cacher, last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        layout_scalar resolved_width =
            resolve_assigned_width(
                this->cacher.resolved_spec,
                assigned_width,
                this->get_horizontal_requirements(ctx));

        // Update child content heights.
        transitioning_layout_content_data* node = this->nodes;
        for (layout_node* i = this->children; i; i = i->next)
        {
            // If you trigger this assert, then you have widgets (or other
            // layout elements) inside your transitioning_container that
            // aren't transitioning_container_content.
            assert(node);
            layout_requirements y =
                alia::get_vertical_requirements(ctx, *i, resolved_width);
            node->content_height = y.size;
            node = node->next;
        }

        query.update(calculated_layout_requirements(
            get_container_height(nodes), 0, 0));
    }
    alia_end
    return query.result();
}

void
transitioning_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher, last_content_change,
        assignment);
    alia_if (rra.update_required())
    {
        layout_box const& region = rra.resolved_assignment().region;

        for (layout_node* i = this->children; i; i = i->next)
        {
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *i, region.size[0]);

            layout_vector content_size =
                make_layout_vector(region.size[0], y.size);

            relative_layout_assignment assignment(
                layout_box(make_layout_vector(0, 0), content_size),
                y.size - y.descent);

            alia::set_relative_assignment(ctx, *i, assignment);
        }

        rra.update();
    }
    alia_end
    this->window = rra.resolved_assignment().region;
}

void transitioning_container::begin(
    ui_context& ctx, animated_transition const& transition,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    transition_ = transition;

    get_cached_data(ctx, &layout_);

    container_.begin(get_layout_traversal(ctx), layout_);

    id_ = get_widget_id(ctx);

    if (is_refresh_pass(ctx))
    {
        update_layout_cacher(get_layout_traversal(ctx), layout_->cacher,
            layout_spec, FILL | UNPADDED);
        // Clear out node list.
        layout_->nodes = 0;
        next_ptr_ = &layout_->nodes;
    }
    else
    {
        if (ctx.event->category == REGION_CATEGORY)
            do_box_region(ctx, id_, layout_->window);

        if (is_in_transition(layout_->nodes))
        {
            clipper_.begin(*get_layout_traversal(ctx).geometry);
            clipper_.set(box<2,double>(layout_->window));
        }

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(
            translation_matrix(
                vector<2,double>(
                    layout_->cacher.relative_assignment.region.corner)));
    }
}

void transitioning_container::end()
{
    if (ctx_)
    {
        clipper_.end();
        transform_.end();
        container_.end();
        ctx_ = 0;
    }
}

void transitioning_container_content::begin(
    ui_context& ctx, transitioning_container& container, bool active)
{
    ctx_ = &ctx;
    container_ = &container;

    float presence =
        smooth_raw_value(ctx, active ? 1.f : 0.f, container.transition_);
    do_content_ = presence > 0;

    transitioning_layout_content_data* node;
    if (get_cached_data(ctx, &node))
    {
        // Initialize it as an empty/absent node.
        node->content_height = 0;
        node->presence = 0;
        node->next = 0;
    }

    widget_id id = get_widget_id(ctx);

    alia_untracked_if (is_refresh_pass(ctx))
    {
        // Detect changes.
        detect_layout_change(ctx, &node->presence, presence);
        // If the node is present, insert it into the container's list.
        if (do_content_)
        {
            *container.next_ptr_ = node;
            container.next_ptr_ = &node->next;
        }
        node->next = 0;
    }
    alia_end

    alia_if (is_in_transition(container.layout_->nodes) && presence > 0)
    {
        auto relative_presence = presence /
            get_presence_through_node(container.layout_->nodes, node);
        transparency_.begin(ctx, relative_presence);
    }
    alia_end

    alia_if (do_content_)
    {
        content_holder_.begin(ctx);
    }
    alia_end
}

void transitioning_container_content::end()
{
    if (ctx_)
    {
        content_holder_.end();
        transparency_.end();
        ctx_ = 0;
    }
}

}
