#include <alia/skia.hpp>
#include "SkPixelRef.h"

namespace alia {

// Check that Skia renders in RGBA order.
#ifdef SK_CPU_BENDIAN
    #if SK_R32_SHIFT != 24 || SK_G32_SHIFT != 16 || \
        SK_B32_SHIFT != 8  || SK_A32_SHIFT != 0
        #error Skia is not configured for RGBA order.
    #endif
#else
    #if SK_R32_SHIFT != 0  || SK_G32_SHIFT != 8 || \
        SK_B32_SHIFT != 16 || SK_A32_SHIFT != 24
        #error Skia is not configured for RGBA order.
    #endif
#endif

SkBitmap& initialize_bitmap(SkBitmap& bitmap, vector<2,int> const& size)
{
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, size[0], size[1],
        size[0] * 4);
    bitmap.allocPixels();
    bitmap.eraseARGB(0, 0, 0, 0);
    return bitmap;
}

void skia_renderer::begin(cached_image_ptr& img, surface& surface,
    vector<2,int> const& size)
{
    img_ = &img;
    surface_ = &surface;
    size_ = size;
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
