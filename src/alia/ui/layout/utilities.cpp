#include <alia/ui/layout/utilities.hpp>

#include <alia/core/flow/macros.hpp>

#include <algorithm>

namespace alia {

void
fold_in_requirements(
    layout_requirements& current, layout_requirements const& additional)
{
    current.ascent = (std::max)(current.ascent, additional.ascent);
    current.descent = (std::max)(current.descent, additional.descent);
    current.size = (std::max)(
        (std::max)(current.size, additional.size),
        current.ascent + current.descent);
}

void
fold_in_requirements(
    calculated_layout_requirements& current,
    layout_requirements const& additional)
{
    current.ascent = (std::max)(current.ascent, additional.ascent);
    current.descent = (std::max)(current.descent, additional.descent);
    current.size = (std::max)(
        (std::max)(current.size, additional.size),
        current.ascent + current.descent);
}

void
set_next_node(layout_traversal& traversal, layout_node* node)
{
    if (*traversal.next_ptr != node)
    {
        record_layout_change(traversal);
        *traversal.next_ptr = node;
    }
}

void
add_layout_node(layout_traversal& traversal, layout_node* node)
{
    set_next_node(traversal, node);
    traversal.next_ptr = &node->next;
}

void
layout_container::record_content_change()
{
    if (!cache_is_fully_invalid(this->cacher))
    {
        invalidate_cached_layout(this->cacher);
        if (this->parent)
            this->parent->record_content_change();
    }
}

void
record_layout_change(layout_traversal& traversal)
{
    if (traversal.active_container)
        traversal.active_container->record_content_change();
}

layout
add_default_size(layout const& layout_spec, absolute_size const& size)
{
    layout adjusted_spec = layout_spec;
    if (adjusted_spec.size[0].length <= 0)
        adjusted_spec.size[0] = size[0];
    if (adjusted_spec.size[1].length <= 0)
        adjusted_spec.size[1] = size[1];
    return adjusted_spec;
}
layout
add_default_padding(layout const& layout_spec, layout_flag_set flag)
{
    assert((flag.code & ~PADDING_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & PADDING_MASK_CODE) == 0)
        adjusted_spec.flags |= flag;
    return adjusted_spec;
}
layout
add_default_x_alignment(layout const& layout_spec, layout_flag_set alignment)
{
    assert((alignment.code & ~X_ALIGNMENT_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & X_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= alignment;
    return adjusted_spec;
}
layout
add_default_y_alignment(layout const& layout_spec, layout_flag_set alignment)
{
    assert((alignment.code & ~Y_ALIGNMENT_MASK_CODE) == 0);
    layout adjusted_spec = layout_spec;
    if ((adjusted_spec.flags.code & Y_ALIGNMENT_MASK_CODE) == 0)
        adjusted_spec.flags |= alignment;
    return adjusted_spec;
}
layout
add_default_alignment(
    layout const& layout_spec,
    layout_flag_set x_alignment,
    layout_flag_set y_alignment)
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

float
resolve_absolute_length(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    unsigned axis,
    absolute_length const& length)
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

vector<2, float>
resolve_absolute_size(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    absolute_size const& size)
{
    return make_vector(
        resolve_absolute_length(ppi, style_info, 0, size[0]),
        resolve_absolute_length(ppi, style_info, 1, size[1]));
}

float
resolve_absolute_length(
    layout_traversal& traversal, unsigned axis, absolute_length const& length)
{
    return resolve_absolute_length(
        traversal.ppi, *traversal.style_info, axis, length);
}

vector<2, float>
resolve_absolute_size(layout_traversal& traversal, absolute_size const& size)
{
    return resolve_absolute_size(traversal.ppi, *traversal.style_info, size);
}

float
resolve_relative_length(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    unsigned axis,
    relative_length const& length,
    float full_length)
{
    return length.is_relative
             ? length.length * full_length
             : resolve_absolute_length(
                   ppi,
                   style_info,
                   axis,
                   absolute_length(length.length, length.units));
}

vector<2, float>
resolve_relative_size(
    vector<2, float> const& ppi,
    layout_style_info const& style_info,
    relative_size const& size,
    vector<2, float> const& full_size)
{
    return make_vector(
        resolve_relative_length(ppi, style_info, 0, size[0], full_size[0]),
        resolve_relative_length(ppi, style_info, 1, size[1], full_size[1]));
}

float
resolve_relative_length(
    layout_traversal& traversal,
    unsigned axis,
    relative_length const& length,
    float full_length)
{
    return resolve_relative_length(
        traversal.ppi, *traversal.style_info, axis, length, full_length);
}

vector<2, float>
resolve_relative_size(
    layout_traversal& traversal,
    relative_size const& size,
    vector<2, float> const& full_size)
{
    return resolve_relative_size(
        traversal.ppi, *traversal.style_info, size, full_size);
}

box_border_width<float>
resolve_box_border_width(
    layout_traversal& traversal,
    box_border_width<absolute_length> const& border)
{
    return box_border_width<float>{
        resolve_absolute_length(traversal, 1, border.top),
        resolve_absolute_length(traversal, 0, border.right),
        resolve_absolute_length(traversal, 1, border.bottom),
        resolve_absolute_length(traversal, 0, border.left)};
}

box_border_width<layout_scalar>
as_layout_size(box_border_width<float> const& border)
{
    return box_border_width<layout_scalar>{
        as_layout_size(border.top),
        as_layout_size(border.right),
        as_layout_size(border.bottom),
        as_layout_size(border.left)};
}

bool
operator==(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return a.size == b.size && a.flags == b.flags
        && a.growth_factor == b.growth_factor
        && a.padding_size == b.padding_size;
}
bool
operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return !(a == b);
}

void
resolve_layout_spec(
    layout_traversal& traversal,
    resolved_layout_spec& resolved,
    layout const& spec,
    layout_flag_set default_flags)
{
    resolved.size
        = as_layout_size(resolve_absolute_size(traversal, spec.size));
    resolved.flags.code
        = ((spec.flags.code & X_ALIGNMENT_MASK_CODE) != 0
               ? (spec.flags.code & X_ALIGNMENT_MASK_CODE)
               : (default_flags.code & X_ALIGNMENT_MASK_CODE))
        | ((spec.flags.code & Y_ALIGNMENT_MASK_CODE) != 0
               ? (spec.flags.code & Y_ALIGNMENT_MASK_CODE)
               : (default_flags.code & Y_ALIGNMENT_MASK_CODE))
        | ((spec.flags.code & PADDING_MASK_CODE) != 0
               ? (spec.flags.code & PADDING_MASK_CODE)
               : (default_flags.code & PADDING_MASK_CODE));
    if ((resolved.flags & PADDED
         || (!(resolved.flags & UNPADDED) && (default_flags & PADDED))))
    {
        resolved.padding_size = traversal.style_info->padding_size;
    }
    else
        resolved.padding_size = make_layout_vector(0, 0);
    resolved.growth_factor
        = spec.growth_factor == 0
               && (resolved.flags.code
                   & (GROW_X_CODE | GROW_Y_CODE | PROPORTIONAL_GROW_CODE))
                      != 0
            ? 1
            : spec.growth_factor;
}

void
resolve_requirements(
    layout_requirements& requirements,
    resolved_layout_spec const& spec,
    unsigned axis,
    calculated_layout_requirements const& calculated)
{
    layout_scalar padding = spec.padding_size[axis];
    requirements.size
        = (std::max)(
              (std::max)(
                  calculated.size, calculated.ascent + calculated.descent),
              spec.size[axis])
        + padding * 2;
    requirements.ascent = calculated.ascent + padding;
    requirements.descent = calculated.descent + padding;
    requirements.growth_factor = spec.growth_factor;
}

static void
resolve_axis_assignment(
    layout_scalar& offset,
    layout_scalar& size,
    unsigned alignment_code,
    layout_scalar assigned_size,
    layout_scalar baseline,
    layout_scalar required_size,
    layout_scalar ascent)
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
    // assert(offset >= 0 && offset + size <= assigned_size);
}

inline unsigned
get_axis_alignment_code(resolved_layout_spec const& spec, unsigned axis)
{
    return (spec.flags.code >> (axis * X_TO_Y_SHIFT)) & X_ALIGNMENT_MASK_CODE;
}

layout_scalar
resolve_assigned_width(
    resolved_layout_spec const& spec,
    layout_scalar assigned_width,
    layout_requirements const& horizontal_requirements)
{
    layout_scalar offset, size;
    resolve_axis_assignment(
        offset,
        size,
        get_axis_alignment_code(spec, 0),
        assigned_width,
        0,
        horizontal_requirements.size,
        0);
    return size - spec.padding_size[0] * 2;
}

relative_layout_assignment
resolve_relative_assignment(
    resolved_layout_spec const& spec,
    relative_layout_assignment const& assignment,
    layout_requirements const& horizontal_requirements,
    layout_requirements const& vertical_requirements)
{
    layout_scalar x_offset, x_size;
    resolve_axis_assignment(
        x_offset,
        x_size,
        get_axis_alignment_code(spec, 0),
        assignment.region.size[0],
        0,
        horizontal_requirements.size,
        0);
    layout_scalar y_offset, y_size;
    resolve_axis_assignment(
        y_offset,
        y_size,
        get_axis_alignment_code(spec, 1),
        assignment.region.size[1],
        assignment.baseline_y,
        vertical_requirements.size,
        vertical_requirements.ascent);
    return relative_layout_assignment{
        layout_box(
            assignment.region.corner + make_layout_vector(x_offset, y_offset)
                + spec.padding_size,
            make_layout_vector(x_size, y_size) - spec.padding_size * 2),
        vertical_requirements.ascent - spec.padding_size[1]};
}

bool
update_layout_cacher(
    layout_traversal& traversal,
    layout_cacher& cacher,
    layout const& layout_spec,
    layout_flag_set default_flags)
{
    resolved_layout_spec resolved_spec;
    resolve_layout_spec(traversal, resolved_spec, layout_spec, default_flags);
    return detect_layout_change(
        traversal, &cacher.resolved_spec, resolved_spec);
}

bool
operator==(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return a.size == b.size && a.ascent == b.ascent && a.descent == b.descent;
}

bool
operator!=(
    leaf_layout_requirements const& a, leaf_layout_requirements const& b)
{
    return !(a == b);
}

layout_requirements
layout_leaf::get_horizontal_requirements()
{
    layout_requirements requirements;
    resolve_requirements(
        requirements,
        resolved_spec_,
        0,
        calculated_layout_requirements(requirements_.size[0], 0, 0));
    return requirements;
}
layout_requirements
layout_leaf::get_vertical_requirements(layout_scalar /*assigned_width*/)
{
    layout_requirements requirements;
    resolve_requirements(
        requirements,
        resolved_spec_,
        1,
        calculated_layout_requirements(
            requirements_.size[1],
            requirements_.ascent,
            requirements_.descent));
    return requirements;
}
void
layout_leaf::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    layout_requirements horizontal_requirements, vertical_requirements;
    resolve_requirements(
        horizontal_requirements,
        resolved_spec_,
        0,
        calculated_layout_requirements(requirements_.size[0], 0, 0));
    resolve_requirements(
        vertical_requirements,
        resolved_spec_,
        1,
        calculated_layout_requirements(
            requirements_.size[1],
            requirements_.ascent,
            requirements_.descent));
    relative_assignment_ = resolve_relative_assignment(
        resolved_spec_,
        assignment,
        horizontal_requirements,
        vertical_requirements);
}

void
layout_leaf::refresh_layout(
    layout_traversal& traversal,
    layout const& layout_spec,
    leaf_layout_requirements const& requirements,
    layout_flag_set default_flags)
{
    if (traversal.is_refresh_pass)
    {
        if (detect_layout_change(traversal, &layout_spec_, layout_spec))
        {
            resolve_layout_spec(
                traversal, resolved_spec_, layout_spec, default_flags);
        }
        detect_layout_change(traversal, &requirements_, requirements);
    }
}

layout_scalar
get_max_child_width(layout_node* children)
{
    layout_scalar width = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements x = node.get_horizontal_requirements();
        width = (std::max)(x.size, width);
    });
    return width;
}

calculated_layout_requirements
fold_horizontal_child_requirements(layout_node* children)
{
    return calculated_layout_requirements(get_max_child_width(children), 0, 0);
}

calculated_layout_requirements
fold_vertical_child_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    calculated_layout_requirements requirements(0, 0, 0);
    walk_layout_children(children, [&](layout_node& node) {
        fold_in_requirements(
            requirements, node.get_vertical_requirements(assigned_width));
    });
    return requirements;
}

void
assign_identical_child_regions(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    walk_layout_children(children, [&](layout_node& node) {
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(make_layout_vector(0, 0), assigned_size),
            assigned_baseline_y});
    });
}

layout_scalar
compute_total_height(layout_node* children, layout_scalar assigned_width)
{
    layout_scalar total_height = 0;
    walk_layout_children(children, [&](layout_node& node) {
        layout_requirements y = node.get_vertical_requirements(assigned_width);
        total_height += y.size;
    });
    return total_height;
}

void
scoped_layout_container::begin(
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
void
scoped_layout_container::end()
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

} // namespace alia
