#ifndef ALIA_SKIA_IMAGE_CANVAS_HPP
#define ALIA_SKIA_IMAGE_CANVAS_HPP

#include <alia/image.hpp>
#include "SkCanvas.h"
#include "SkBitmap.h"

namespace alia { namespace skia {

class image_canvas
{
 public:
    image_canvas(image_view<rgba8>& image);
    SkBitmap bitmap;
    SkCanvas canvas;
};

}}

#endif
