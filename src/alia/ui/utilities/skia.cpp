#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/utilities.hpp>

#include <SkPixelRef.h>
#include <SkTypeface.h>
#include <SkPaint.h>
#include <SkImageInfo.h>
#include <SkPath.h>

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

SkBitmap& skia_renderer::initialize_bitmap(
    SkBitmap& bitmap, vector<2,int> const& size)
{
    bitmap.allocPixels(
        SkImageInfo::Make(
            size[0], size[1],
            kRGBA_8888_SkColorType,
            kPremul_SkAlphaType));
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

SkRect skia_box_as_skia_rect(skia_box const& box)
{
    SkRect rect;
    rect.fLeft = box.corner[0];
    rect.fRight = box.corner[0] + box.size[0];
    rect.fTop = box.corner[1];
    rect.fBottom = box.corner[1] + box.size[1];
    return rect;
}

skia_box layout_box_as_skia_box(layout_box const& box)
{
    return skia_box(
        make_vector(
            layout_scalar_as_skia_scalar(box.corner[0]),
            layout_scalar_as_skia_scalar(box.corner[1])),
        make_vector(
            layout_scalar_as_skia_scalar(box.size[0]),
            layout_scalar_as_skia_scalar(box.size[1])));
}

skia_box float_box_as_skia_box(box<2,float> const& box)
{
    return skia_box(
        make_vector(
            SkFloatToScalar(box.corner[0]),
            SkFloatToScalar(box.corner[1])),
        make_vector(
            SkFloatToScalar(box.size[0]),
            SkFloatToScalar(box.size[1])));
}

void draw_round_rect(SkCanvas& canvas, SkPaint& paint,
    layout_box const& region)
{
    SkScalar radius =
        layout_scalar_as_skia_scalar(
            (std::min)(region.size[0], region.size[1])) /
        SkIntToScalar(4);
    canvas.drawRoundRect(
        skia_box_as_skia_rect(layout_box_as_skia_box(region)), radius, radius, paint);
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
        layout_scalar_as_skia_scalar((std::min)(size[0], size[1])) /
        SkIntToScalar(4);
    canvas.drawRoundRect(rect, radius, radius, paint);
}

void draw_rect(SkCanvas& canvas, SkPaint& paint, skia_box const& region)
{
    SkRect rect;
    rect.fLeft = region.corner[0];
    rect.fRight = region.corner[0] + region.size[0];
    rect.fTop = region.corner[1];
    rect.fBottom = region.corner[1] + region.size[1];
    canvas.drawRect(rect, paint);
}

void set_skia_font_info(SkPaint& paint, font const& font)
{
    paint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
    paint.setTypeface(
        SkTypeface::MakeFromName(
            font.name.c_str(),
            SkFontStyle(
                (font.style & BOLD) ?
                    SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight,
                SkFontStyle::kNormal_Width,
                (font.style & ITALIC) ?
                    SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant)));
    paint.setUnderlineText((font.style & UNDERLINE) ? true : false);
    paint.setStrikeThruText((font.style & STRIKETHROUGH) ? true : false);
    paint.setTextAlign(SkPaint::kLeft_Align);
    paint.setAntiAlias(true);
    paint.setLCDRenderText(true);
    paint.setTextSize(SkFloatToScalar(font.size));
    paint.setSubpixelText(true);
    paint.setAutohinted(true);
}

float get_focus_border_width(dataless_ui_context& ctx)
{
    return resolve_absolute_length(get_layout_traversal(ctx), 0,
        get_property(ctx, "focus-border-width", INHERITED_PROPERTY,
            absolute_length(1.5, PIXELS)));
}

void setup_focus_drawing(dataless_ui_context& ctx, SkPaint& paint)
{
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(SkFloatToScalar(get_focus_border_width(ctx)));
    paint.setStrokeCap(SkPaint::kSquare_Cap);
    set_color(paint, get_color_property(ctx, "focus-color"));
}

void draw_focus_rect(dataless_ui_context& ctx, focus_rect_data& data,
    layout_box const& content_region)
{
    float width = get_focus_border_width(ctx);
    layout_box padded_region =
        add_border(content_region, as_layout_size(width));
    caching_renderer cache(ctx, data, *ctx.style.id, padded_region);
    if (cache.needs_rendering())
    {
        skia_renderer renderer(ctx, cache.image(), padded_region.size);
        SkRect rect = skia_box_as_skia_rect(layout_box_as_skia_box(
            layout_box(make_layout_vector(0, 0), content_region.size)));
        renderer.canvas().translate(
            layout_scalar_as_skia_scalar(as_layout_size(width)),
            layout_scalar_as_skia_scalar(as_layout_size(width)));
        SkPaint paint;
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        setup_focus_drawing(ctx, paint);
        paint.setStrokeJoin(SkPaint::kRound_Join);
        renderer.canvas().drawRect(rect, paint);
        renderer.cache();
        cache.mark_valid();
    }
    cache.draw();
}

static void clamp_radius_pair(SkScalar& x0, SkScalar& x1, SkScalar total)
{
    if (x0 + x1 > total)
    {
        SkScalar half = SkScalarHalf(total);
        if (x0 < half)
            x1 = total - x0;
        else if (x1 < half)
            x0 = total - x1;
        else
            x0 = x1 = half;
    }
}

void draw_rect(SkCanvas& canvas, SkPaint& paint,
    skia_box const& region, resolved_box_corner_sizes const& radii)
{
    SkRect rect = skia_box_as_skia_rect(region);

    SkScalar w = rect.width();
    SkScalar half_w = SkScalarHalf(w);
    SkScalar h = rect.height();
    SkScalar half_h = SkScalarHalf(h);

    if (half_w <= 0 || half_h <= 0)
        return;

    SkScalar r[4][2];
    for (int i = 0; i != 4; ++i)
    {
        vector<2,float> const& c = radii.corners[i];
        r[i][0] = SkFloatToScalar(c[0]);
        r[i][1] = SkFloatToScalar(c[1]);
    }
    clamp_radius_pair(r[0][0], r[1][0], w);
    clamp_radius_pair(r[1][1], r[2][1], h);
    clamp_radius_pair(r[2][0], r[3][0], w);
    clamp_radius_pair(r[3][1], r[0][1], h);

    SkScalar s[4][2];
    #define CUBIC_ARC_FACTOR ((SK_ScalarSqrt2 - SK_Scalar1) * 4 / 3)
    for (int i = 0; i != 4; ++i)
    {
        s[i][0] = SkScalarMul(r[i][0], CUBIC_ARC_FACTOR);
        s[i][1] = SkScalarMul(r[i][1], CUBIC_ARC_FACTOR);
    }

    SkPath path;
    path.moveTo(rect.fLeft, rect.fTop + r[0][1]);
    path.cubicTo(
        rect.fLeft, rect.fTop + r[0][1] - s[0][1],
        rect.fLeft + r[0][0] - s[0][0], rect.fTop,
        rect.fLeft + r[0][0], rect.fTop);
    path.lineTo(rect.fRight - r[1][0], rect.fTop);
    path.cubicTo(
        rect.fRight - r[1][0] + s[1][0], rect.fTop,
        rect.fRight, rect.fTop + r[1][1] - s[1][1],
        rect.fRight, rect.fTop + r[1][1]);
    path.lineTo(rect.fRight, rect.fBottom - r[2][1]);
    path.cubicTo(
        rect.fRight, rect.fBottom - r[2][1] + s[2][1],
        rect.fRight - r[2][0] + s[2][0], rect.fBottom,
        rect.fRight - r[2][0], rect.fBottom);
    path.lineTo(rect.fLeft + r[3][0], rect.fBottom);
    path.cubicTo(
        rect.fLeft + r[3][0] - s[3][0], rect.fBottom,
        rect.fLeft, rect.fBottom - r[3][1] + s[3][1],
        rect.fLeft, rect.fBottom - r[3][1]);
    path.lineTo(rect.fLeft, rect.fTop + r[0][1]);
    canvas.drawPath(path, paint);
}

resolved_box_corner_sizes
adjust_border_radii_for_border_width(
    resolved_box_corner_sizes const& radii,
    box_border_width<float> const& border_width)
{
    // Not sure if this is actually correct.
    resolved_box_corner_sizes adjusted;
    adjusted.corners[0][0] = radii.corners[0][0] - border_width.left;
    adjusted.corners[0][1] = radii.corners[0][1] - border_width.top;
    adjusted.corners[1][0] = radii.corners[1][0] - border_width.right;
    adjusted.corners[1][1] = radii.corners[1][1] - border_width.top;
    adjusted.corners[2][0] = radii.corners[2][0] - border_width.right;
    adjusted.corners[2][1] = radii.corners[2][1] - border_width.bottom;
    adjusted.corners[3][0] = radii.corners[3][0] - border_width.left;
    adjusted.corners[3][1] = radii.corners[3][1] - border_width.bottom;
    return adjusted;
}

}
