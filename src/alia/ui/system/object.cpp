#include "alia/ui/utilities/skia.hpp"
#include <alia/ui/system/object.hpp>

#include <alia/core/context/interface.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/utilities/rendering.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkCanvas.h>
#include <include/core/SkMatrix.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace alia {

struct noop_geometry_subscriber : geometry_context_subscriber
{
    void
    push_state() override
    {
    }

    void
    pop_state() override
    {
    }

    void
    set_transformation_matrix(matrix<3, 3, double> const&) override
    {
    }

    void
    set_clip_region(box<2, double> const&) override
    {
    }
};

struct skia_geometry_subscriber : geometry_context_subscriber
{
    void
    push_state() override
    {
        canvas->save();
    }

    void
    pop_state() override
    {
        canvas->restore();
    }

    void
    set_transformation_matrix(matrix<3, 3, double> const& m) override
    {
        // TODO: This could be more efficient.
        SkMatrix matrix;
        SkScalar values[9] = {
            SkScalar(m(0, 0)),
            SkScalar(m(0, 1)),
            SkScalar(m(0, 2)),
            SkScalar(m(1, 0)),
            SkScalar(m(1, 1)),
            SkScalar(m(1, 2)),
            SkScalar(m(2, 0)),
            SkScalar(m(2, 1)),
            SkScalar(m(2, 2))};
        matrix.set9(values);
        canvas->setMatrix(matrix);
    }

    void
    set_clip_region(box<2, double> const& region) override
    {
        // TODO: Change this interface to make the implementation less
        // convoluted.
        // TODO: `region` should just be passed directly as floats?
        canvas->clipRect(as_skrect(layout_box(region)));
    }

    SkCanvas* canvas;
};

void
ui_system::invoke_controller(vanilla_ui_context vanilla_ctx)
{
    // TODO
    layout_style_info style_info{
        make_layout_vector(4, 4), 16, make_layout_vector(12, 16), 12, 1};

    static noop_geometry_subscriber noop_subscriber;

    skia_geometry_subscriber skia_subscriber;

    geometry_context geometry;
    initialize(
        geometry,
        make_box(make_vector<double>(0, 0), vector<2, double>(surface_size)));

    {
        render_event* render;
        if (detect_event(vanilla_ctx, &render))
        {
            skia_subscriber.canvas = render->canvas;
            set_subscriber(geometry, skia_subscriber);
        }
        else
        {
            set_subscriber(geometry, noop_subscriber);
        }
    }

    ui_traversal traversal;

    initialize_layout_traversal(
        this->layout,
        traversal.layout,
        is_refresh_event(vanilla_ctx),
        is_refresh_event(vanilla_ctx) ? nullptr : &geometry,
        &style_info,
        make_vector<float>(200, 200)); // TODO

    auto ctx = add_context_object<ui_traversal_tag>(
        add_context_object<ui_system_tag>(vanilla_ctx, std::ref(*this)),
        std::ref(traversal));

    this->controller(ctx);
}

} // namespace alia
