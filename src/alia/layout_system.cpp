#include <alia/layout_system.hpp>

namespace alia {

bool operator==(size const& a, size const& b)
{
    return a.width == b.width && a.height == b.height &&
        a.x_units == b.x_units && a.y_units == b.y_units;
}
bool operator!=(size const& a, size const& b)
{
    return !(a == b);
}

bool operator==(layout const& a, layout const& b)
{
    return a.size == b.size && a.flags == b.flags &&
        a.proportion == b.proportion;
}
bool operator!=(layout const& a, layout const& b)
{
    return !(a == b);
}

void fold_in_requirements(layout_requirements& current,
    layout_requirements const& additional)
{
    current.minimum_ascent = (std::max)(
        current.minimum_ascent, additional.minimum_ascent);
    current.minimum_descent = (std::max)(
        current.minimum_descent, additional.minimum_descent);
    current.minimum_size = (std::max)(
        current.minimum_size, additional.minimum_size);
}

layout default_layout;

static void initialize_traversal(
    layout_system& system, layout_traversal& traversal, data_traversal& data,
    bool is_refresh, geometry_context* geometry, layout_style_info* style,
    vector<2,float> const& ppi)
{
    traversal.data = &data;
    traversal.active_container = 0;
    traversal.next_ptr = &system.root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.refresh_counter = system.refresh_counter;
    traversal.geometry = geometry;
    traversal.style_info = style;
    traversal.ppi = ppi;

    style->font_size = 0;
    style->character_size = make_vector<float>(0, 0);
    style->x_height = 0;
}
void scoped_layout_traversal::begin(
    layout_system& system, layout_traversal& traversal, data_traversal& data,
    geometry_context& geometry, vector<2,float> const& ppi)
{
    initialize_traversal(system, traversal, data, false, &geometry,
        &dummy_style_info_, ppi);
}
void scoped_layout_refresh::begin(
    layout_system& system, layout_traversal& traversal, data_traversal& data,
    vector<2,float> const& ppi)
{
    ++system.refresh_counter;
    initialize_traversal(system, traversal, data, true, 0, &dummy_style_info_,
        ppi);
}

void scoped_layout_calculation_context::begin(
    data_graph& cache, layout_calculation_context& ctx)
{
    data_.begin(cache, ctx.data);
    ctx.map = retrieve_naming_map(ctx.data);
    ctx.for_measurement = false;
}
void scoped_layout_calculation_context::end()
{
    data_.end();
}

void resolve_layout(layout_node* root_node, data_graph& cache,
    layout_vector const& size)
{
    if (root_node)
    {
        layout_calculation_context ctx;
        scoped_layout_calculation_context slcc(cache, ctx);
        get_horizontal_requirements(ctx, *root_node);
        get_vertical_requirements(ctx, *root_node, size[0]);
        set_relative_assignment(ctx, *root_node,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0), size), 0));
    }
}

void resolve_layout(layout_system& system, layout_vector const& size)
{
    resolve_layout(system.root_node, system.calculation_cache, size);
}

layout_vector get_minimum_size(layout_node* root_node, data_graph& cache)
{
    if (root_node)
    {
        layout_calculation_context ctx;
        scoped_layout_calculation_context slcc(cache, ctx);
        ctx.for_measurement = true;
        layout_requirements horizontal =
            get_horizontal_requirements(ctx, *root_node);
        layout_requirements vertical =
            get_vertical_requirements(ctx, *root_node,
                horizontal.minimum_size);
        return make_layout_vector(horizontal.minimum_size,
            vertical.minimum_size);
    }
    else
        return make_layout_vector(0, 0);
}

layout_vector get_minimum_size(layout_system& system)
{
    return get_minimum_size(system.root_node, system.calculation_cache);
}

layout add_default_size(layout const& layout_spec, size const& size)
{
    layout adjusted_spec = layout_spec;
    if (adjusted_spec.size.width < 0)
    {
        adjusted_spec.size.x_units = size.x_units;
        adjusted_spec.size.width = size.width;
    }
    if (adjusted_spec.size.height < 0)
    {
        adjusted_spec.size.y_units = size.y_units;
        adjusted_spec.size.height = size.height;
    }
    return adjusted_spec;
}
layout add_default_padding(layout const& layout_spec, layout_flag_set flag)
{
    assert((flag.code & ~PADDING_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & PADDING_MASK_CODE) == 0)
        adjusted_spec.flags |= flag;
    return adjusted_spec;
}
layout add_default_x_alignment(layout const& layout_spec,
    layout_flag_set alignment)
{
    assert((alignment.code & ~X_ALIGNMENT_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & X_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= alignment;
    return adjusted_spec;
}
layout add_default_y_alignment(layout const& layout_spec,
    layout_flag_set alignment)
{
    assert((alignment.code & ~Y_ALIGNMENT_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & Y_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= alignment;
    return adjusted_spec;
}
layout add_default_alignment(layout const& layout_spec,
    layout_flag_set x_alignment, layout_flag_set y_alignment)
{
    assert((x_alignment.code & ~X_ALIGNMENT_MASK_CODE) == 0);
    assert((y_alignment.code & ~Y_ALIGNMENT_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & X_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= x_alignment;
    if ((adjusted_spec.flags.code & Y_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= y_alignment;
    return adjusted_spec;
}

void initialize(layout_traversal& traversal, layout_container& container)
{
    container.last_content_change = traversal.refresh_counter;
}

static void record_container_change(layout_traversal& traversal,
    layout_container* container)
{
    while (container && container->last_content_change !=
        traversal.refresh_counter)
    {
        container->last_content_change = traversal.refresh_counter;
        container = container->parent;
    }
}

static void set_next_node(layout_traversal& traversal, layout_node* node)
{
    if (*traversal.next_ptr != node)
    {
        record_layout_change(traversal);
        *traversal.next_ptr = node;
    }
}

void add_layout_node(layout_traversal& traversal, layout_node* node)
{
    set_next_node(traversal, node);
    traversal.next_ptr = &node->next;
}

void record_layout_change(layout_traversal& traversal)
{
    record_container_change(traversal, traversal.active_container);
}

void scoped_layout_container::begin(
    layout_traversal& traversal, layout_container* container)
{
    if (traversal.is_refresh_pass)
    {
        traversal_ = &traversal;

        set_next_node(traversal, container);
        container->parent = traversal.active_container;

        traversal.next_ptr = &container->children;
        traversal.active_container = container;
    }
}
void scoped_layout_container::end()
{
    if (traversal_)
    {
        set_next_node(*traversal_, 0);

        layout_container* container = traversal_->active_container;
        traversal_->next_ptr = &container->next;
        traversal_->active_container = container->parent;

        traversal_ = 0;
    }
}

void wrap_row(wrapping_state& state)
{
    state.active_row.requirements.minimum_size = (std::max)(
        state.active_row.requirements.minimum_size,
        state.active_row.requirements.minimum_ascent +
        state.active_row.requirements.minimum_descent);
    if (state.rows)
        state.rows->push_back(state.active_row);
    state.active_row.y += state.active_row.requirements.minimum_size;
    state.active_row.requirements = layout_requirements(0, 0, 0, 0);
    state.accumulated_width = 0;
}
void wrap_row(wrapping_assignment_state& state)
{
    state.x = 0;
    ++state.active_row;
}

layout_requirements layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    return this->get_horizontal_requirements(ctx);
}
void layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_state& state)
{
    layout_requirements x = this->get_horizontal_requirements(ctx);
    if (state.accumulated_width + x.minimum_size > assigned_width)
        wrap_row(state);
    layout_requirements y =
        this->get_vertical_requirements(ctx, x.minimum_size);
    state.accumulated_width += x.minimum_size;
    fold_in_requirements(state.active_row.requirements, y);
}
void layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_assignment_state& state)
{
    layout_requirements x = this->get_horizontal_requirements(ctx);
    if (state.x + x.minimum_size > assigned_width)
        wrap_row(state);
    layout_scalar row_height = state.active_row->requirements.minimum_size;
    this->set_relative_assignment(
        ctx,
        relative_layout_assignment(
            layout_box(
                make_layout_vector(state.x, state.active_row->y),
                make_layout_vector(x.minimum_size, row_height)),
            row_height - state.active_row->requirements.minimum_descent));
    state.x += x.minimum_size;
}

layout_scalar resolve_layout_width(layout_traversal& traversal, float width,
    layout_units units)
{
    switch (units)
    {
     case PIXELS:
     default:
        return as_layout_size(width);
     case INCHES:
        return as_layout_size(width * traversal.ppi[0]);
     case CM:
        return as_layout_size(width * traversal.ppi[0] / 2.54f);
     case MM:
        return as_layout_size(width * traversal.ppi[0] / 25.4f);
     case CHARS:
        return as_layout_size(width *
            traversal.style_info->character_size[0]);
     case EM:
        return as_layout_size(width * traversal.style_info->font_size);
     case EX:
        return as_layout_size(width * traversal.style_info->x_height);
    }
}

layout_scalar resolve_layout_height(layout_traversal& traversal, float height,
    layout_units units)
{
    switch (units)
    {
     case PIXELS:
     default:
        return as_layout_size(height);
        break;
     case INCHES:
        return as_layout_size(height * traversal.ppi[1]);
        break;
     case CM:
        return as_layout_size(height * traversal.ppi[1] / 2.54f);
        break;
     case MM:
        return as_layout_size(height * traversal.ppi[1] / 25.4f);
        break;
     case CHARS:
        return as_layout_size(height *
            traversal.style_info->character_size[1]);
        break;
     case EM:
        return as_layout_size(height * traversal.style_info->font_size);
        break;
     case EX:
        return as_layout_size(height * traversal.style_info->x_height);
        break;
    }
}

layout_vector resolve_layout_size(layout_traversal& traversal, size const& s)
{
    return make_vector(resolve_layout_width(traversal, s.width, s.x_units),
        resolve_layout_height(traversal, s.height, s.y_units));
}

bool operator==(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return a.size == b.size && a.flags == b.flags &&
        a.proportion == a.proportion && a.padding_size == b.padding_size;
}
bool operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b)
{ return !(a == b); }

void resolve_layout_spec(
    layout_traversal& traversal, resolved_layout_spec& resolved,
    layout const& spec, layout_flag_set default_flags)
{
    resolved.size = resolve_layout_size(traversal, spec.size);
    resolved.flags.code =
        ((spec.flags.code & X_ALIGNMENT_MASK_CODE) != 0 ?
            (spec.flags.code & X_ALIGNMENT_MASK_CODE) :
            (default_flags.code & X_ALIGNMENT_MASK_CODE)) |
        ((spec.flags.code & Y_ALIGNMENT_MASK_CODE) != 0 ?
            (spec.flags.code & Y_ALIGNMENT_MASK_CODE) :
            (default_flags.code & Y_ALIGNMENT_MASK_CODE)) |
        ((spec.flags.code & PADDING_MASK_CODE) != 0 ?
            (spec.flags.code & PADDING_MASK_CODE) :
            (default_flags.code & PADDING_MASK_CODE));
    if (resolved.flags & PADDED || !(resolved.flags & UNPADDED) &&
        traversal.style_info->is_padded)
    {
        resolved.padding_size = traversal.style_info->padding_size;
    }
    else
        resolved.padding_size = make_layout_vector(0, 0);
    resolved.proportion =
        spec.proportion == 0 &&
        (resolved.flags.code & (GROW_X_CODE | GROW_Y_CODE |
            PROPORTIONAL_GROW_CODE)) != 0 ?
        1 : spec.proportion;
}

void resolve_requirements(
    layout_requirements& requirements, resolved_layout_spec const& spec,
    unsigned axis, calculated_layout_requirements const& calculated)
{
    int padding = spec.padding_size[axis];
    requirements.minimum_size =
        (std::max)(
            (std::max)(
                calculated.minimum_size,
                calculated.minimum_ascent + calculated.minimum_descent),
            spec.size[axis]) +
        padding * 2;
    requirements.minimum_ascent = calculated.minimum_ascent + padding;
    requirements.minimum_descent = calculated.minimum_descent + padding;
    requirements.proportion = spec.proportion;
  }

static void resolve_axis_assignment(
    layout_scalar& offset, layout_scalar& size,
    unsigned alignment_code,
    layout_scalar assigned_size, layout_scalar baseline,
    layout_scalar required_size, layout_scalar ascent)
{
    switch (alignment_code)
    {
     case CENTER_X_CODE:
        offset = (assigned_size - required_size) / 2;
        size = required_size;
        break;
     case LEFT_CODE:
        offset = 0;
        size = required_size;
        break;
     case RIGHT_CODE:
        offset = assigned_size - required_size;
        size = required_size;
        break;
     case FILL_X_CODE:
     case GROW_X_CODE:
        offset = 0;
        size = assigned_size;
        break;
     case BASELINE_X_CODE:
        offset = baseline - ascent;
        size = required_size;
        break;
    }
}

static inline unsigned get_axis_alignment_code(
    resolved_layout_spec const& spec, unsigned axis)
{
    return (spec.flags.code >> (axis * X_TO_Y_SHIFT)) & X_ALIGNMENT_MASK_CODE;
}

layout_scalar resolve_assigned_width(resolved_layout_spec const& spec,
    layout_scalar assigned_width,
    layout_requirements const& horizontal_requirements)
{
    layout_scalar offset, size;
    resolve_axis_assignment(offset, size, get_axis_alignment_code(spec, 0),
        assigned_width, 0,
        horizontal_requirements.minimum_size, 0);
    return size - spec.padding_size[0] * 2;
}

void resolve_relative_assignment(
    relative_layout_assignment& resolved_assignment,
    resolved_layout_spec const& spec,
    relative_layout_assignment const& assignment,
    layout_requirements const& horizontal_requirements,
    layout_requirements const& vertical_requirements)
{
    layout_scalar x_offset, x_size;
    resolve_axis_assignment(x_offset, x_size, get_axis_alignment_code(spec, 0),
        assignment.region.size[0], 0,
        horizontal_requirements.minimum_size, 0);
    layout_scalar y_offset, y_size;
    resolve_axis_assignment(y_offset, y_size, get_axis_alignment_code(spec, 1),
        assignment.region.size[1], assignment.baseline_y,
        vertical_requirements.minimum_size,
        vertical_requirements.minimum_size -
            vertical_requirements.minimum_descent);
    resolved_assignment = relative_layout_assignment(
        layout_box(
            assignment.region.corner +
            make_layout_vector(x_offset, y_offset) + spec.padding_size,
            make_layout_vector(x_size, y_size) - spec.padding_size * 2),
        assignment.baseline_y - (y_offset + spec.padding_size[1]));
}

bool update(layout_traversal& traversal, layout_cacher& cacher,
    layout const& layout_spec, layout_flag_set default_flags)
{
    resolved_layout_spec resolved_spec;
    resolve_layout_spec(traversal, resolved_spec, layout_spec, default_flags);
    return detect_layout_change(traversal, &cacher.resolved_spec,
        resolved_spec);
}

horizontal_layout_query::horizontal_layout_query(
    layout_calculation_context& ctx,
    layout_cacher& cacher,
    counter_type last_content_change)
  : cacher_(&cacher), last_content_change_(last_content_change)
{
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(
        ctx, named_block_, make_id(&cacher));
}
void horizontal_layout_query::update(
    calculated_layout_requirements const& calculated)
{
    resolve_requirements(cacher_->horizontal_requirements,
        cacher_->resolved_spec, 0, calculated);
    cacher_->last_horizontal_query = last_content_change_;
}

vertical_layout_query::vertical_layout_query(
    layout_calculation_context& ctx,
    layout_cacher& cacher,
    counter_type last_content_change,
    layout_scalar assigned_width)
  : cacher_(&cacher), last_content_change_(last_content_change)
{
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, named_block_,
        combine_ids(make_id(&cacher), make_id(assigned_width)))
    update_required_ =
        get_data(ctx, &data_) ||
        data_->last_vertical_query != last_content_change_;
}
void vertical_layout_query::update(
    calculated_layout_requirements const& calculated)
{
    resolve_requirements(data_->vertical_requirements,
        cacher_->resolved_spec, 1, calculated);
    data_->last_vertical_query = last_content_change_;
}

relative_region_assignment::relative_region_assignment(
    layout_calculation_context& ctx,
    layout_node& node,
    layout_cacher& cacher,
    counter_type last_content_change,
    relative_layout_assignment const& assignment)
  : cacher_(&cacher), last_content_change_(last_content_change)
{
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(
        ctx, named_block_, make_id(&cacher));

    alia_if (cacher_->last_relative_assignment != last_content_change_ ||
        cacher_->relative_assignment != assignment)
    {
        relative_layout_assignment resolved_assignment;
        resolve_relative_assignment(
            resolved_assignment, cacher_->resolved_spec, assignment,
            cacher_->horizontal_requirements,
            get_vertical_requirements(ctx, node, assignment.region.size[0]));
        if (cacher_->resolved_relative_assignment.region.size !=
                resolved_assignment.region.size ||
            cacher_->resolved_relative_assignment.baseline_y !=
                resolved_assignment.baseline_y)
        {
            // This ensures that an update will be performed.
            cacher_->last_relative_assignment = 0;
        }
        cacher_->resolved_relative_assignment = resolved_assignment;
        cacher_->relative_assignment = assignment;
    }
    alia_end

    update_required_ =
        cacher_->last_relative_assignment != last_content_change_;
}
void relative_region_assignment::update()
{
    cacher_->last_relative_assignment = last_content_change_;
}

layout_requirements simple_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        query.update(logic->get_horizontal_requirements(ctx, children));
    }
    alia_end
    return query.result();
}
layout_requirements simple_layout_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher, last_content_change,
        assigned_width);
    alia_if (query.update_required())
    {
        query.update(
            logic->get_vertical_requirements(
                ctx, children,
                resolve_assigned_width(
                    this->cacher.resolved_spec,
                    assigned_width,
                    this->get_horizontal_requirements(ctx))));
    }
    alia_end
    return query.result();
}
void simple_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher, last_content_change,
        assignment);
    alia_if (rra.update_required())
    {
        this->assigned_size = rra.resolved_assignment().region.size;
        logic->set_relative_assignment(
            ctx, children,
            rra.resolved_assignment().region.size,
            rra.resolved_assignment().baseline_y);
        rra.update();
    }
    alia_end
}

layout_requirements layout_leaf::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    layout_requirements requirements;
    resolve_requirements(
        requirements, resolved_spec_, 0,
        calculated_layout_requirements(requirements_.size[0], 0, 0));
    return requirements;
}
layout_requirements layout_leaf::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    layout_requirements requirements;
    resolve_requirements(
        requirements, resolved_spec_, 1,
        calculated_layout_requirements(requirements_.size[1],
            requirements_.ascent, requirements_.descent));
    return requirements;
}
void layout_leaf::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    layout_requirements horizontal_requirements, vertical_requirements;
    resolve_requirements(
        horizontal_requirements, resolved_spec_, 0,
        calculated_layout_requirements(requirements_.size[0], 0, 0));
    resolve_requirements(
        vertical_requirements, resolved_spec_, 1,
        calculated_layout_requirements(requirements_.size[1],
            requirements_.ascent, requirements_.descent));
    resolve_relative_assignment(relative_assignment_, resolved_spec_,
        assignment, horizontal_requirements, vertical_requirements);
}

void layout_leaf::refresh_layout(
    layout_traversal& traversal, layout const& layout_spec,
    leaf_layout_requirements const& requirements,
    layout_flag_set default_flags)
{
    // TODO: Cache this.
    if (traversal.is_refresh_pass)
    {
        resolved_layout_spec resolved_spec;
        resolve_layout_spec(traversal, resolved_spec, layout_spec,
            default_flags);
        detect_layout_change(traversal, &resolved_spec_, resolved_spec);
    }
    requirements_ = requirements;
}

}
