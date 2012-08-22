#include <alia/skia.hpp>
#include "SkPixelRef.h"

namespace alia {

void skia_renderer::begin(cached_image_ptr& img, surface& surface,
    vector<2,int> const& size)
{
    img_ = &img;
    surface_ = &surface;
    size_ = size;

    // Note that although the config specifies ARGB, the default Skia
    // configuration actually arranges ARGB values in RGBA format.
    bitmap_.setConfig(SkBitmap::kARGB_8888_Config, size[0], size[1],
        size[0] * 4);
    bitmap_.allocPixels();
    bitmap_.eraseARGB(0, 0, 0, 0);

    canvas_.setBitmapDevice(bitmap_);
}

void skia_renderer::cache()
{
    if (surface_)
    {
        bitmap_.lockPixels();
        surface_->cache_image(*img_,
            image_interface(bitmap_.pixelRef()->pixels(), RGBA,
                vector<2,unsigned>(size_), size_[0]));
        bitmap_.unlockPixels();

        surface_ = 0;
    }
}

//void caching_skia_renderer::begin(
//    caching_renderer_data& data, surface& surface,
//    id_interface const& content_id, box<2,int> const& region)
//{
//    caching_.begin(data, surface, content_id, region);
//    if (caching_.needs_rendering())
//        skia_.begin(caching_.image(), surface, region.size);
//}
//
//void caching_skia_renderer::draw()
//{
//    if (skia_.is_active())
//    {
//        skia_.cache();
//        caching_.mark_valid();
//    }
//    caching_.draw();
//}

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    box<2,int> const& region)
{
    SkRect rect;
    rect.fLeft = SkScalar(region.corner[0]);
    rect.fRight = SkScalar(region.corner[0] + region.size[0]);
    rect.fTop = SkScalar(region.corner[1]);
    rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
    SkScalar radius = SkScalar((std::min)(region.size[0], region.size[1])) / 4;
    canvas.drawRoundRect(rect, radius, radius, paint);
}
void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    vector<2,int> const& size)
{
    SkRect rect;
    rect.fLeft = 0;
    rect.fRight = SkScalar(size[0]);
    rect.fTop = 0;
    rect.fBottom = SkScalar(size[1]);
    SkScalar radius = SkScalar((std::min)(size[0], size[1])) / 4;
    canvas.drawRoundRect(rect, radius, radius, paint);
}

void draw_rect(SkCanvas& canvas, SkPaint& paint, box<2,SkScalar> const& region)
{
    SkRect rect;
    rect.fLeft = region.corner[0];
    rect.fRight = region.corner[0] + region.size[0];
    rect.fTop = region.corner[1];
    rect.fBottom = region.corner[1] + region.size[1];
    canvas.drawRect(rect, paint);
}
void draw_rect(SkCanvas& canvas, SkPaint& paint, box<2,int> const& region)
{
    SkRect rect;
    rect.fLeft = SkScalar(region.corner[0]);
    rect.fRight = SkScalar(region.corner[0] + region.size[0]);
    rect.fTop = SkScalar(region.corner[1]);
    rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
    canvas.drawRect(rect, paint);
}
void draw_rect(SkCanvas& canvas, SkPaint& paint, vector<2,int> const& size)
{
    SkRect rect;
    rect.fLeft = 0;
    rect.fRight = SkScalar(size[0]);
    rect.fTop = 0;
    rect.fBottom = SkScalar(size[1]);
    canvas.drawRect(rect, paint);
}

}
