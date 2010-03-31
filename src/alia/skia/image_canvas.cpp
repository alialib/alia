#include <alia/skia/image_canvas.hpp>

namespace alia { namespace skia {

image_canvas::image_canvas(image_view<rgba8>& image)
{
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, image.size[0],
        image.size[1], image.stride * 4);
    bitmap.setPixels(image.pixels);
    canvas.setBitmapDevice(bitmap);
}

}}
