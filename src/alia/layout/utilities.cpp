#include <alia/layout/utilities.hpp>

#include <algorithm>

namespace alia {

void fold_in_requirements(layout_requirements& current,
    layout_requirements const& additional)
{
    current.ascent = (std::max)(current.ascent, additional.ascent);
    current.descent = (std::max)(current.descent, additional.descent);
    current.size = (std::max)((std::max)(current.size, additional.size),
        current.ascent + current.descent);
}

void fold_in_requirements(calculated_layout_requirements& current,
    layout_requirements const& additional)
{
    current.ascent = (std::max)(current.ascent, additional.ascent);
    current.descent = (std::max)(current.descent, additional.descent);
    current.size = (std::max)((std::max)(current.size, additional.size),
        current.ascent + current.descent);
}

void set_next_node(layout_traversal& traversal, layout_node* node)
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

void layout_container::record_change(layout_traversal& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_change(traversal);
    }
}

void record_layout_change(layout_traversal& traversal)
{
    if (traversal.active_container)
        traversal.active_container->record_change(traversal);
}

layout add_default_size(layout const& layout_spec, absolute_size const& size)
{
    layout adjusted_spec = layout_spec;
    if (adjusted_spec.size[0].length <= 0)
        adjusted_spec.size[0] = size[0];
    if (adjusted_spec.size[1].length <= 0)
        adjusted_spec.size[1] = size[1];
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

void wrap_row(wrapping_state& state)
{
    state.active_row.width = state.visible_width;
    state.active_row.requirements.size = (std::max)(
        state.active_row.requirements.size,
        state.active_row.requirements.ascent +
        state.active_row.requirements.descent);
    if (state.rows)
        state.rows->push_back(state.active_row);
    state.active_row.y += state.active_row.requirements.size;
    state.active_row.requirements = layout_requirements(0, 0, 0, 0);
    state.accumulated_width = state.visible_width = 0;
}
layout_scalar
calculate_initial_x(
    layout_scalar assigned_width,
    layout_flag_set x_alignment,
    wrapped_row const& row)
{
    switch (x_alignment.code)
    {
     case RIGHT_CODE:
        return assigned_width - row.width;
     case CENTER_X_CODE:
        return (assigned_width - row.width) / 2;
     default:
        return 0;
    }
}
void wrap_row(wrapping_assignment_state& state)
{
    ++state.active_row;
    state.x = state.active_row != state.end_row
      ? calculate_initial_x(state.assigned_width, state.x_alignment,
            *state.active_row)
      : 0;
}

layout_requirements layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    return this->get_horizontal_requirements(ctx);
}
void layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    wrapping_state& state)
{
    layout_requirements x = this->get_horizontal_requirements(ctx);
    if (state.accumulated_width + x.size > state.assigned_width)
        wrap_row(state);
    layout_requirements y =
        this->get_vertical_requirements(ctx, x.size);
    state.visible_width += x.size;
    state.accumulated_width += x.size;
    fold_in_requirements(state.active_row.requirements, y);
}
void layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    wrapping_assignment_state& state)
{
    layout_requirements x = this->get_horizontal_requirements(ctx);
    if (state.x + x.size > state.assigned_width)
        wrap_row(state);
    layout_scalar row_height = state.active_row->requirements.size;
    this->set_relative_assignment(
        ctx,
        relative_layout_assignment(
            layout_box(
                make_layout_vector(state.x, state.active_row->y),
                make_layout_vector(x.size, row_height)),
            state.active_row->requirements.ascent));
    state.x += x.size;
}

float resolve_absolute_length(
    vector<2,float> const& ppi, layout_style_info const& style_info,
    unsigned axis, absolute_length const& length)
{
    float scale_factor;
    switch (length.units)
    {
     case PIXELS:
     default:
        scale_factor = style_info.magnification;
        break;
     case UNMAGNIFIED_PIXELS:
        scale_factor = 1;
        break;
     case INCHES:
        scale_factor = style_info.magnification * ppi[axis];
        break;
     case CM:
        scale_factor = style_info.magnification * ppi[axis] / 2.54f;
        break;
     case MM:
        scale_factor = style_info.magnification * ppi[axis] / 25.4f;
        break;
     case POINT:
        scale_factor = style_info.magnification * ppi[axis] / 72.f;
        break;
     case PICA:
        scale_factor = style_info.magnification * ppi[axis] / 6.f;
        break;
     case CHARS:
        scale_factor = style_info.character_size[axis];
        break;
     case EM:
        scale_factor = style_info.font_size;
        break;
     case EX:
        scale_factor = style_info.x_height;
        break;
    }
    return length.length * scale_factor;
}

vector<2,float>
resolve_absolute_size(
    vector<2,float> const& ppi, layout_style_info const& style_info,
    absolute_size const& size)
{
    return make_vector(
        resolve_absolute_length(ppi, style_info, 0, size[0]),
        resolve_absolute_length(ppi, style_info, 1, size[1]));
}

float resolve_absolute_length(
    layout_traversal& traversal, unsigned axis,
    absolute_length const& length)
{
    return resolve_absolute_length(
        traversal.ppi, *traversal.style_info, axis, length);
}

vector<2,float>
resolve_absolute_size(layout_traversal& traversal, absolute_size const& size)
{
    return resolve_absolute_size(
        traversal.ppi, *traversal.style_info, size);
}

float resolve_relative_length(
    vector<2,float> const& ppi, layout_style_info const& style_info,
    unsigned axis, relative_length const& length, float full_length)
{
    return length.is_relative ?
        length.length * full_length :
        resolve_absolute_length(ppi, style_info, axis,
            absolute_length(length.length, length.units));
}

vector<2,float>
resolve_relative_size(
    vector<2,float> const& ppi, layout_style_info const& style_info,
    relative_size const& size, vector<2,float> const& full_size)
{
    return make_vector(
        resolve_relative_length(ppi, style_info, 0, size[0], full_size[0]),
        resolve_relative_length(ppi, style_info, 1, size[1], full_size[1]));
}

float resolve_relative_length(
    layout_traversal& traversal, unsigned axis,
    relative_length const& length, float full_length)
{
    return resolve_relative_length(
        traversal.ppi, *traversal.style_info, axis, length, full_length);
}

vector<2,float>
resolve_relative_size(layout_traversal& traversal, relative_size const& size,
    vector<2,float> const& full_size)
{
    return resolve_relative_size(
        traversal.ppi, *traversal.style_info, size, full_size);
}

box_border_width<float>
resolve_box_border_width(layout_traversal& traversal,
    box_border_width<absolute_length> const& border)
{
    return box_border_width<float>(
        resolve_absolute_length(traversal, 1, border.top),
        resolve_absolute_length(traversal, 0, border.right),
        resolve_absolute_length(traversal, 1, border.bottom),
        resolve_absolute_length(traversal, 0, border.left));
}

box_border_width<layout_scalar>
as_layout_size(box_border_width<float> const& border)
{
    return box_border_width<layout_scalar>(
        as_layout_size(border.top),
        as_layout_size(border.right),
        as_layout_size(border.bottom),
        as_layout_size(border.left));
}

bool operator==(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return a.size == b.size && a.flags == b.flags &&
        a.growth_factor == b.growth_factor && a.padding_size == b.padding_size;
}
bool operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b)
{ return !(a == b); }

void resolve_layout_spec(
    layout_traversal& traversal, resolved_layout_spec& resolved,
    layout const& spec, layout_flag_set default_flags)
{
    resolved.size =
        as_layout_size(resolve_absolute_size(traversal, spec.size));
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
    if ((resolved.flags & PADDED ||
         !(resolved.flags & UNPADDED) && (default_flags & PADDED)))
    {
        resolved.padding_size = traversal.style_info->padding_size;
    }
    else
        resolved.padding_size = make_layout_vector(0, 0);
    resolved.growth_factor =
        spec.growth_factor == 0 &&
        (resolved.flags.code & (GROW_X_CODE | GROW_Y_CODE |
            PROPORTIONAL_GROW_CODE)) != 0 ?
        1 : spec.growth_factor;
}

void resolve_requirements(
    layout_requirements& requirements, resolved_layout_spec const& spec,
    unsigned axis, calculated_layout_requirements const& calculated)
{
    layout_scalar padding = spec.padding_size[axis];
    requirements.size =
        (std::max)(
            (std::max)(
                calculated.size,
                calculated.ascent + calculated.descent),
            spec.size[axis]) +
        padding * 2;
    requirements.ascent = calculated.ascent + padding;
    requirements.descent = calculated.descent + padding;
    requirements.growth_factor = spec.growth_factor;
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
     default:
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
    //assert(offset >= 0 && offset + size <= assigned_size);
}

static inline unsigned get_axis_alignment_code(
    resolved_layout_spec const& spec, unsigned axis)
{
    return (spec.flags.code >> (axis * X_TO_Y_SHIFT)) & X_ALIGNMENT_MASK_CODE;
}

layout_scalar resolve_assigned_width(
    resolved_layout_spec const& spec, layout_scalar assigned_width,
    layout_requirements const& horizontal_requirements)
{
    layout_scalar offset, size;
    resolve_axis_assignment(offset, size,
        get_axis_alignment_code(spec, 0),
        assigned_width, 0,
        horizontal_requirements.size, 0);
    return size - spec.padding_size[0] * 2;
}

void resolve_relative_assignment(
    relative_layout_assignment& resolved_assignment,
    resolved_layout_spec const& spec,
    relative_layout_assignment const& assignment,
    layout_requirements const& horizontal_requirements,
    layout_requirements const& vertical_requirements)
{
    //assert(assignment.baseline_y >= vertical_requirements.ascent);
    //assert(assignment.baseline_y + vertical_requirements.descent <=
    //    assignment.region.size[1]);
    layout_scalar x_offset, x_size;
    resolve_axis_assignment(x_offset, x_size,
        get_axis_alignment_code(spec, 0),
        assignment.region.size[0], 0,
        horizontal_requirements.size,
        0);
    layout_scalar y_offset, y_size;
    resolve_axis_assignment(y_offset, y_size,
        get_axis_alignment_code(spec, 1),
        assignment.region.size[1], assignment.baseline_y,
        vertical_requirements.size, vertical_requirements.ascent);
    resolved_assignment = relative_layout_assignment(
        layout_box(
            assignment.region.corner +
                make_layout_vector(x_offset, y_offset) + spec.padding_size,
            make_layout_vector(x_size, y_size) - spec.padding_size * 2),
        vertical_requirements.ascent - spec.padding_size[1]);
}

bool update_layout_cacher(
    layout_traversal& traversal, layout_cacher& cacher,
    layout const& layout_spec, layout_flag_set default_flags)
{
    if (cacher.id == 0)
        cacher.id = get_cacher_id(*traversal.system);
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
        ctx, named_block_, make_id(cacher.id));
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
        combine_ids(make_id(cacher.id), make_id(assigned_width)))
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
        ctx, named_block_, make_id(cacher.id));

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

void initialize(layout_traversal& traversal, layout_container& container)
{
    container.last_content_change = traversal.refresh_counter;
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

bool operator==(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return a.size == b.size && a.ascent == b.ascent && a.descent == b.descent;
}

bool operator!=(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return !(a == b);
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
    if (traversal.is_refresh_pass)
    {
        // TODO: Cache this?
        resolved_layout_spec resolved_spec;
        resolve_layout_spec(traversal, resolved_spec, layout_spec,
            default_flags);
        detect_layout_change(traversal, &resolved_spec_, resolved_spec);

        detect_layout_change(traversal, &requirements_, requirements);
    }
}

// GEOMETRY CONTEXT

void set_subscriber(geometry_context& ctx,
    geometry_context_subscriber& subscriber)
{
    ctx.subscriber = &subscriber;
    if (ctx.subscriber)
    {
        ctx.subscriber->set_transformation_matrix(ctx.transformation_matrix);
        ctx.subscriber->set_clip_region(ctx.clip_region);
    }
}

void initialize(geometry_context& ctx, box<2,double> const& full_region)
{
    ctx.full_region = ctx.clip_region = full_region;
    ctx.transformation_matrix = identity_matrix<3,double>();
    ctx.subscriber = 0;
}

void set_clip_region(geometry_context& ctx, box<2,double> const& clip_region)
{
    ctx.clip_region = clip_region;
    if (ctx.subscriber)
        ctx.subscriber->set_clip_region(clip_region);
}

void set_transformation_matrix(geometry_context& ctx,
    matrix<3,3,double> const& matrix)
{
    ctx.transformation_matrix = matrix;
    if (ctx.subscriber)
        ctx.subscriber->set_transformation_matrix(matrix);
}

void scoped_clip_region::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_region_ = ctx.clip_region;
}
void scoped_clip_region::set(box<2,double> const& region)
{
    vector<2,double> corner0 =
        transform(ctx_->transformation_matrix, region.corner);
    vector<2,double> corner1 =
        transform(ctx_->transformation_matrix, get_high_corner(region));
    box<2,double> region_in_root_frame;
    for (unsigned i = 0; i != 2; ++i)
    {
        region_in_root_frame.corner[i] =
            corner0[i] < corner1[i] ? corner0[i] : corner1[i];
        region_in_root_frame.size[i] =
            std::fabs(corner1[i] - corner0[i]);
    }
    box<2,double> new_region;
    if (!compute_intersection(&new_region, old_region_, region_in_root_frame))
        new_region = box<2,double>(make_vector(0., 0.), make_vector(0., 0.));
    set_clip_region(*ctx_, new_region);
}
void scoped_clip_region::restore()
{
    set_clip_region(*ctx_, old_region_);
}
void scoped_clip_region::end()
{
    if (ctx_)
    {
        restore();
        ctx_ = 0;
    }
}

void scoped_clip_region_reset::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_region_ = ctx.clip_region;
    set_clip_region(ctx, ctx.full_region);
}
void scoped_clip_region_reset::end()
{
    if (ctx_)
    {
        set_clip_region(*ctx_, old_region_);
        ctx_ = 0;
    }
}

void scoped_transformation::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_matrix_ = ctx.transformation_matrix;
}
void scoped_transformation::set(matrix<3,3,double> const& transformation)
{
    set_transformation_matrix(*ctx_, old_matrix_ * transformation);
}
void scoped_transformation::restore()
{
    set_transformation_matrix(*ctx_, old_matrix_);
}
void scoped_transformation::end()
{
    if (ctx_)
    {
        restore();
        ctx_ = 0;
    }
}

bool is_visible(geometry_context& ctx, box<2,double> const& region)
{
    // TODO: This assumes that opposite corners of the region will define a
    // bounding box for the region even in the transformed space, but this is
    // only true if the only rotations in the transformation matrix are at
    // right angles.
    // Since a more general test would probably be more expensive, it may be
    // worth keeping track of whether or not the matrix contains such
    // rotations.
    vector<2,double>
        window_low = ctx.clip_region.corner,
        window_high = get_high_corner(ctx.clip_region),
        region_low =
            transform(ctx.transformation_matrix, region.corner),
        region_high =
            transform(ctx.transformation_matrix, get_high_corner(region));
    for (unsigned i = 0; i != 2; ++i)
    {
        if (region_low[i] < window_low[i] &&
            region_high[i] < window_low[i] ||
            region_low[i] > window_high[i] &&
            region_high[i] > window_high[i])
        {
            return false;
        }
    }
    return true;
}

void begin_layout_transform(
    scoped_transformation& transform,
    layout_traversal const& traversal,
    layout_cacher const& cacher)
{
    if (!traversal.is_refresh_pass)
    {
        transform.begin(*traversal.geometry);
        transform.set(translation_matrix(vector<2,double>(
            get_assignment(cacher).region.corner)));
    }
}

layout_scalar
get_max_child_width(layout_calculation_context& ctx, layout_node* children)
{
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x = alia::get_horizontal_requirements(ctx, *i);
        width = (std::max)(x.size, width);
    }
    return width;
}

calculated_layout_requirements
fold_horizontal_child_requirements(
    layout_calculation_context& ctx, layout_node* children)
{
    return calculated_layout_requirements(
        get_max_child_width(ctx, children), 0, 0);
}

calculated_layout_requirements
fold_vertical_child_requirements(
    layout_calculation_context& ctx, layout_node* children,
    layout_scalar assigned_width)
{
    calculated_layout_requirements requirements(0, 0, 0);
    for (layout_node* i = children; i; i = i->next)
    {
        fold_in_requirements(requirements,
            alia::get_vertical_requirements(ctx, *i, assigned_width));
    }
    return requirements;
}

void assign_identical_child_regions(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    for (layout_node* i = children; i; i = i->next)
    {
        alia::set_relative_assignment(ctx, *i,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0), assigned_size),
                assigned_baseline_y));
    }
}

layout_scalar
compute_total_height(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar total_height = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y =
            alia::get_vertical_requirements(ctx, *i, assigned_width);
        total_height += y.size;
    }
    return total_height;
}

}
