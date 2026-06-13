#include <alia/abi/ui/layout/utilities/placement.h>

extern "C" {

alia_layout_axis_placement
alia_resolve_container_x(
    alia_layout_flags_t flags, float assigned_size, float required_size)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 1, 1, 1};
    static_assert(ALIA_X_ALIGNMENT_MASK >> ALIA_X_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_X_ALIGNMENT_MASK) >> ALIA_X_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return alia_layout_axis_placement{
        .offset = offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

alia_layout_axis_placement
alia_resolve_container_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {1, 0, 0, 0, 1, 0, 1, 1};
    static_assert(ALIA_Y_ALIGNMENT_MASK >> ALIA_Y_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_Y_ALIGNMENT_MASK) >> ALIA_Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - required_size;
    return alia_layout_axis_placement{
        .offset = offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - ascent),
        .size = required_size + sizes[index] * extra_space};
}

alia_box
alia_resolve_container_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent)
{
    alia_layout_axis_placement x_placement
        = alia_resolve_container_x(flags, assigned_size.x, required_size.x);
    alia_layout_axis_placement y_placement = alia_resolve_container_y(
        flags, assigned_size.y, baseline, required_size.y, ascent);
    return {
        {x_placement.offset, y_placement.offset},
        {x_placement.size, y_placement.size}};
}

alia_layout_axis_placement
alia_resolve_leaf_x(
    alia_layout_flags_t flags,
    float assigned_size,
    float required_size,
    float spacing)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    static_assert(ALIA_X_ALIGNMENT_MASK >> ALIA_X_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_X_ALIGNMENT_MASK) >> ALIA_X_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + spacing * 2);
    return alia_layout_axis_placement{
        .offset = spacing + offsets[index] * extra_space,
        .size = required_size + sizes[index] * extra_space};
}

alia_layout_axis_placement
alia_resolve_leaf_y(
    alia_layout_flags_t flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float spacing)
{
    static float const offsets[8] = {0, 0.5, 0, 1, 0, 0, 0, 0};
    static float const baseline_offsets[8] = {0, 0, 0, 0, 0, 1, 0, 0};
    static float const sizes[8] = {0, 0, 0, 0, 1, 0, 1, 1};
    static_assert(ALIA_Y_ALIGNMENT_MASK >> ALIA_Y_ALIGNMENT_BIT_OFFSET == 7);
    auto const index
        = (flags & ALIA_Y_ALIGNMENT_MASK) >> ALIA_Y_ALIGNMENT_BIT_OFFSET;
    float const extra_space = assigned_size - (required_size + spacing * 2);
    return alia_layout_axis_placement{
        .offset = spacing + offsets[index] * extra_space
                + baseline_offsets[index] * (baseline - (ascent + spacing)),
        .size = required_size + sizes[index] * extra_space};
}

alia_box
alia_resolve_leaf_box(
    alia_layout_flags_t flags,
    alia_vec2f assigned_size,
    float baseline,
    alia_vec2f required_size,
    float ascent,
    alia_vec2f spacing)
{
    alia_layout_axis_placement x_placement = alia_resolve_leaf_x(
        flags, assigned_size.x, required_size.x, spacing.x);
    alia_layout_axis_placement y_placement = alia_resolve_leaf_y(
        flags, assigned_size.y, baseline, required_size.y, ascent, spacing.y);
    return {
        {x_placement.offset, y_placement.offset},
        {x_placement.size, y_placement.size}};
}

float
alia_resolve_baseline(
    alia_layout_flags_t flags,
    float assigned_height,
    float ascent,
    float descent)
{
    static float const offsets[4] = {0.5, 0, 1, 0.5};
    static_assert(
        ALIA_BASELINE_GROUP_ALIGNMENT_MASK
            >> ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET
        == 3);
    auto const index = (flags & ALIA_BASELINE_GROUP_ALIGNMENT_MASK)
                    >> ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET;
    return ascent + offsets[index] * (assigned_height - ascent - descent);
}

} // extern "C"
