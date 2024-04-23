#include <alia/ui/layout/geometry.hpp>

namespace alia {

void
set_subscriber(geometry_context& ctx, geometry_context_subscriber& subscriber)
{
    ctx.subscriber = &subscriber;
    if (ctx.subscriber)
    {
        ctx.subscriber->set_transformation_matrix(ctx.transformation_matrix);
        ctx.subscriber->set_clip_region(ctx.clip_region);
    }
}

void
initialize(geometry_context& ctx, box<2, double> const& full_region)
{
    ctx.full_region = ctx.clip_region = full_region;
    ctx.transformation_matrix = identity_matrix<3, double>();
    ctx.subscriber = 0;
}

void
set_clip_region(geometry_context& ctx, box<2, double> const& clip_region)
{
    ctx.clip_region = clip_region;
    if (ctx.subscriber)
        ctx.subscriber->set_clip_region(clip_region);
}

void
set_transformation_matrix(
    geometry_context& ctx, matrix<3, 3, double> const& matrix)
{
    ctx.transformation_matrix = matrix;
    if (ctx.subscriber)
        ctx.subscriber->set_transformation_matrix(matrix);
}

void
scoped_clip_region::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_region_ = ctx.clip_region;
}
void
scoped_clip_region::set(box<2, double> const& region)
{
    vector<2, double> corner0
        = transform(ctx_->transformation_matrix, region.corner);
    vector<2, double> corner1
        = transform(ctx_->transformation_matrix, get_high_corner(region));
    box<2, double> region_in_root_frame;
    for (unsigned i = 0; i != 2; ++i)
    {
        region_in_root_frame.corner[i]
            = corner0[i] < corner1[i] ? corner0[i] : corner1[i];
        region_in_root_frame.size[i] = std::fabs(corner1[i] - corner0[i]);
    }
    box<2, double> new_region;
    if (!compute_intersection(&new_region, old_region_, region_in_root_frame))
        new_region = box<2, double>(make_vector(0., 0.), make_vector(0., 0.));
    set_clip_region(*ctx_, new_region);
}
void
scoped_clip_region::restore()
{
    set_clip_region(*ctx_, old_region_);
}
void
scoped_clip_region::end()
{
    if (ctx_)
    {
        restore();
        ctx_ = 0;
    }
}

void
scoped_clip_region_reset::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_region_ = ctx.clip_region;
    set_clip_region(ctx, ctx.full_region);
}
void
scoped_clip_region_reset::end()
{
    if (ctx_)
    {
        set_clip_region(*ctx_, old_region_);
        ctx_ = 0;
    }
}

void
scoped_transformation::begin(geometry_context& ctx)
{
    ctx_ = &ctx;
    old_matrix_ = ctx.transformation_matrix;
}
void
scoped_transformation::set(matrix<3, 3, double> const& transformation)
{
    set_transformation_matrix(*ctx_, old_matrix_ * transformation);
}
void
scoped_transformation::restore()
{
    set_transformation_matrix(*ctx_, old_matrix_);
}
void
scoped_transformation::end()
{
    if (ctx_)
    {
        restore();
        ctx_ = 0;
    }
}

bool
is_visible(geometry_context& ctx, box<2, double> const& region)
{
    // TODO: This assumes that opposite corners of the region will define a
    // bounding box for the region even in the transformed space, but this is
    // only true if the only rotations in the transformation matrix are at
    // right angles.
    // Since a more general test would probably be more expensive, it may be
    // worth keeping track of whether or not the matrix contains such
    // rotations.
    vector<2, double> window_low
        = ctx.clip_region.corner,
        window_high = get_high_corner(ctx.clip_region),
        region_low = transform(ctx.transformation_matrix, region.corner),
        region_high
        = transform(ctx.transformation_matrix, get_high_corner(region));
    for (unsigned i = 0; i != 2; ++i)
    {
        if ((region_low[i] < window_low[i] && region_high[i] < window_low[i])
            || (region_low[i] > window_high[i]
                && region_high[i] > window_high[i]))
        {
            return false;
        }
    }
    return true;
}

}
