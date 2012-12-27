#ifndef ALIA_SKIA_HPP
#define ALIA_SKIA_HPP

#include <alia/rendering.hpp>
#include <alia/layout_interface.hpp>
#include <SkCanvas.h>
#include <SkBitmap.h>

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

SkBitmap& initialize_bitmap(SkBitmap& bitmap, vector<2,int> const& size);

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

    void cache();

    SkCanvas& canvas() { return canvas_; }

    bool is_active() const { return surface_ != 0; }

 private:
    void begin(cached_image_ptr& img, surface& surface,
        vector<2,int> const& size);

    surface* surface_;

    cached_image_ptr* img_;

    vector<2,int> size_;

    SkBitmap bitmap_;
    SkCanvas canvas_;
};

static inline void set_color(SkPaint& paint, rgba8 color)
{
    paint.setARGB(color.a, color.r, color.g, color.b);
}

SkRect layout_box_as_skia_rect(layout_box const& box);

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_box const& region);
void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_vector const& size);

void draw_rect(SkCanvas& canvas, SkPaint& paint,
    box<2,SkScalar> const& region);
void draw_rect(SkCanvas& canvas, SkPaint& paint, layout_box const& region);
void draw_rect(SkCanvas& canvas, SkPaint& paint, layout_vector const& size);

void set_skia_font_info(SkPaint& paint, font const& font);

}

#endif
