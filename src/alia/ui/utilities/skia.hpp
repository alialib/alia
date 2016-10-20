#ifndef ALIA_UI_UTILITIES_SKIA_HPP
#define ALIA_UI_UTILITIES_SKIA_HPP

#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/styling.hpp>
#include <SkCanvas.h>
#include <SkBitmap.h>

// This file provides various utilities for using Skia as a widget renderer.

namespace alia {

// Cast a Skia scalar to layout scalar representing a size.
static inline layout_scalar skia_scalar_as_layout_size(SkScalar x)
{ return SkScalarCeilToInt(x); }
// Cast a Skia scalar to a layout scalar.
static inline layout_scalar skia_scalar_as_layout_scalar(SkScalar x)
{ return SkScalarRoundToInt(x); }
// Cast a layout scalar to a Skia scalar.
static inline SkScalar layout_scalar_as_skia_scalar(layout_scalar x)
{ return SkIntToScalar(x); }

// skia_renderer is used to render to a cached image using Skia.
struct skia_renderer
{
 public:
    skia_renderer() : surface_(0) {}

    template<class Context>
    skia_renderer(Context& ctx, cached_image_ptr& img,
        vector<2,int> const& size)
      : bitmap_()
      , canvas_(initialize_bitmap(bitmap_, size))
    { begin(img, get_surface(ctx), size); }

    template<class Context>
    skia_renderer(surface& surface, cached_image_ptr& img,
        vector<2,int> const& size)
      : bitmap_()
      , canvas_(initialize_bitmap(bitmap_, size))
    { begin(img, surface, size); }

    void begin(cached_image_ptr& img, surface& surface,
        vector<2,int> const& size);

    // Get access to the Skia canvas that represents the cached image.
    SkCanvas& canvas() { return canvas_; }

    // Call this when you're done rendering to write the rendered image to the
    // cached_image_ptr.
    void cache();

 private:
    static SkBitmap& initialize_bitmap(
        SkBitmap& bitmap, vector<2,int> const& size);

    surface* surface_;

    cached_image_ptr* img_;

    vector<2,int> size_;

    SkBitmap bitmap_;
    SkCanvas canvas_;
};

// Various utilities for drawing with Skia...

static inline SkColor as_skia_color(rgba8 color)
{ return SkColorSetARGB(color.a, color.r, color.g, color.b); }

static inline void set_color(SkPaint& paint, rgba8 color)
{
    paint.setARGB(color.a, color.r, color.g, color.b);
}

typedef vector<2,SkScalar> skia_vector;
typedef box<2,SkScalar> skia_box;

SkRect skia_box_as_skia_rect(skia_box const& box);
skia_box layout_box_as_skia_box(layout_box const& box);
skia_box float_box_as_skia_box(box<2,float> const& box);

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_box const& region);
void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_vector const& size);

void draw_rect(SkCanvas& canvas, SkPaint& paint,
    skia_box const& region);

void draw_rect(SkCanvas& canvas, SkPaint& paint,
    skia_box const& region, resolved_box_corner_sizes const& radii);

resolved_box_corner_sizes
adjust_border_radii_for_border_width(
    resolved_box_corner_sizes const& radii,
    box_border_width<float> const& border_width);

void set_skia_font_info(SkPaint& paint, font const& font);

void setup_focus_drawing(dataless_ui_context& ctx, SkPaint& paint);

typedef caching_renderer_data focus_rect_data;

void draw_focus_rect(dataless_ui_context& ctx, focus_rect_data& data,
    layout_box const& content_region);

}

#endif
