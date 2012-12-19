#ifndef ALIA_SKIA_HPP
#define ALIA_SKIA_HPP

#include <alia/rendering.hpp>

#include <SkCanvas.h>
#include <SkBitmap.h>

namespace alia {

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

//struct caching_skia_renderer
//{
// public:
//    caching_skia_renderer() {}
//
//    template<class Context>
//    caching_skia_renderer(Context& ctx, caching_renderer_data& data,
//        id_interface const& content_id, box<2,int> const& region)
//    { begin(ctx, data, content_id, region); }
//
//    ~caching_skia_renderer() { end(); }
//
//    template<class Context>
//    void begin(Context& ctx, caching_renderer_data& data,
//        id_interface const& content_id, box<2,int> const& region)
//    {
//        begin(data, get_surface(ctx), content_id, region);
//    }
//
//    void begin(caching_renderer_data& data, surface& surface,
//        id_interface const& content_id, box<2,int> const& region);
//
//    void end() {}
//
//    bool needs_rendering() const { return caching_.needs_rendering(); }
//    SkCanvas& canvas() { return skia_.canvas(); }
//
//    void draw();
//
// private:
//    caching_renderer caching_;
//    skia_renderer skia_;
//};


static inline void set_color(SkPaint& paint, rgba8 color)
{
    paint.setARGB(color.a, color.r, color.g, color.b);
}

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    box<2,int> const& region);
void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    vector<2,int> const& size);

void draw_rect(SkCanvas& canvas, SkPaint& paint,
    box<2,SkScalar> const& region);
void draw_rect(SkCanvas& canvas, SkPaint& paint, box<2,int> const& region);
void draw_rect(SkCanvas& canvas, SkPaint& paint, vector<2,int> const& size);

}

#endif
