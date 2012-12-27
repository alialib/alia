#include <alia/skia.hpp>
#include <SkPixelRef.h>
#include <SkTypeface.h>
#include <SkPaint.h>

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

SkRect layout_box_as_skia_rect(layout_box const& box)
{
    SkRect rect;
    rect.fLeft = layout_scalar_as_skia_scalar(box.corner[0]);
    rect.fRight = layout_scalar_as_skia_scalar(box.corner[0] + box.size[0]);
    rect.fTop = layout_scalar_as_skia_scalar(box.corner[1]);
    rect.fBottom = layout_scalar_as_skia_scalar(box.corner[1] + box.size[1]);
    return rect;
}

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_box const& region)
{
    SkScalar radius =
        SkScalarDiv(
            layout_scalar_as_skia_scalar(
                (std::min)(region.size[0], region.size[1])),
            SkIntToScalar(4));
    canvas.drawRoundRect(
        layout_box_as_skia_rect(region), radius, radius, paint);
}
void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_vector const& size)
{
    SkRect rect;
    rect.fLeft = 0;
    rect.fRight = layout_scalar_as_skia_scalar(size[0]);
    rect.fTop = 0;
    rect.fBottom = layout_scalar_as_skia_scalar(size[1]);
    SkScalar radius =
        SkScalarDiv(
            layout_scalar_as_skia_scalar((std::min)(size[0], size[1])),
            SkIntToScalar(4));
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
void draw_rect(SkCanvas& canvas, SkPaint& paint, layout_box const& region)
{
    canvas.drawRect(layout_box_as_skia_rect(region), paint);
}
void draw_rect(SkCanvas& canvas, SkPaint& paint, layout_vector const& size)
{
    SkRect rect;
    rect.fLeft = 0;
    rect.fRight = layout_scalar_as_skia_scalar(size[0]);
    rect.fTop = 0;
    rect.fBottom = layout_scalar_as_skia_scalar(size[1]);
    canvas.drawRect(rect, paint);
}

void set_skia_font_info(SkPaint& paint, font const& font)
{
    paint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
    paint.setTypeface(
        SkTypeface::CreateFromName(
            font.name.c_str(),
            SkTypeface::Style(
                ((font.style & BOLD) ?
                    SkTypeface::kBold : SkTypeface::kNormal) |
                ((font.style & ITALIC) ?
                    SkTypeface::kItalic : SkTypeface::kNormal))))->unref();
    paint.setTextAlign(SkPaint::kLeft_Align);
    paint.setAntiAlias(true);
    paint.setLCDRenderText(true);
    paint.setTextSize(SkFloatToScalar(font.size));
    paint.setSubpixelText(true);
    paint.setAutohinted(true);
}

}
