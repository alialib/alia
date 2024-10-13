#ifndef ALIA_UI_LAYOUT_GEOMETRY_HPP
#define ALIA_UI_LAYOUT_GEOMETRY_HPP

#include <alia/ui/geometry.hpp>

namespace alia {

struct layout_traversal;
struct layout_cacher;

// geometry_context represents a context for defining 2D geometry.
// It provides a transformation matrix, which maps the current frame of
// reference for the context to its root frame of reference.
// It also provides a clip region, which is a rectangle defined in the root
// frame of reference.
// An object may 'subscribe' to a geometry_context so that it is informed of
// changes in the state of the context.
// TODO: Redesign this....
struct geometry_context_subscriber
{
    virtual void
    push_state()
        = 0;
    virtual void
    pop_state()
        = 0;
    virtual void
    set_transformation_matrix(
        matrix<3, 3, double> const& transformation_matrix)
        = 0;
    virtual void
    set_clip_region(box<2, double> const& clip_region)
        = 0;
};
struct geometry_context
{
    box<2, double> full_region;
    matrix<3, 3, double> transformation_matrix;
    box<2, double> clip_region;
    geometry_context_subscriber* subscriber;
};

void
initialize(geometry_context& ctx, box<2, double> const& full_region);

void
set_subscriber(geometry_context& ctx, geometry_context_subscriber& subscriber);

void
set_clip_region(geometry_context& ctx, box<2, double> const& clip_region);

void
set_transformation_matrix(
    geometry_context& ctx, matrix<3, 3, double> const& matrix);

// Is any part of the given region visible through the clipping rectangle?
bool
is_visible(geometry_context& ctx, box<2, double> const& region);

// geometry_contexts are specified by the same generic Context concept as is
// used in data_graph.hpp.
inline geometry_context&
get_geometry_context(geometry_context& ctx)
{
    return ctx;
}

// scoped_clip_region is a scoped object that further restricts the clip region
// of a geometry_context while it's active. While it's active, the context's
// clip region will be set to the intersection of the old region and what's
// passed to set().
// The region is specified in the current frame of reference for the context.
// However, since the region for the context must be an axis-aligned rectangle
// in the root frame of reference, this will only work properly if the current
// transformation matrix is composed only of translations, scaling, and
// 90-degree rotations.
class scoped_clip_region : noncopyable
{
 public:
    scoped_clip_region() : ctx_(0)
    {
    }
    template<class Context>
    scoped_clip_region(Context& ctx)
    {
        begin(get_geometry_context(ctx));
    }
    template<class Context>
    scoped_clip_region(Context& ctx, box<2, double> const& region)
    {
        begin(ctx);
        set(region);
    }
    ~scoped_clip_region()
    {
        end();
    }
    void
    begin(geometry_context& ctx);
    void
    set(box<2, double> const& region);
    void
    end();

 private:
    geometry_context* ctx_;
    box<2, double> old_region_;
};

// scoped_clip_region_reset resets the clip region to the full geometry region
// for its scope. This can be useful for drawing overlays that are meant to
// extend beyond the clip region normally associated with their scope.
// class scoped_clip_region_reset : noncopyable
// {
//  public:
//     scoped_clip_region_reset() : ctx_(0)
//     {
//     }
//     template<class Context>
//     scoped_clip_region_reset(Context& ctx)
//     {
//         begin(get_geometry_context(ctx));
//     }
//     ~scoped_clip_region_reset()
//     {
//         end();
//     }
//     void
//     begin(geometry_context& ctx);
//     void
//     end();

//  private:
//     geometry_context* ctx_;
//     box<2, double> old_region_;
// };

// scoped_transformation is a scoped object that applies a transformation to a
// geometry_context. While it's active, the context's transformation matrix
// will be set to the product of the old matrix and what's passed to set().
class scoped_transformation : noncopyable
{
 public:
    scoped_transformation() : ctx_(0)
    {
    }
    template<class Context>
    scoped_transformation(Context& ctx)
    {
        begin(get_geometry_context(ctx));
    }
    template<class Context>
    scoped_transformation(
        Context& ctx, matrix<3, 3, double> const& transformation)
    {
        begin(get_geometry_context(ctx));
        set(transformation);
    }
    ~scoped_transformation()
    {
        end();
    }
    void
    begin(geometry_context& ctx);
    void
    set(matrix<3, 3, double> const& transformation);
    void
    end();

 private:
    geometry_context* ctx_;
    matrix<3, 3, double> old_matrix_;
};

void
begin_layout_transform(
    scoped_transformation& transform,
    layout_traversal const& traversal,
    layout_cacher const& cacher);

} // namespace alia

#endif
