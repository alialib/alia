#ifndef ALIA_UI_UTILITIES_RENDERING_HPP
#define ALIA_UI_UTILITIES_RENDERING_HPP

class SkCanvas;

namespace alia {

struct render_event
{
    SkCanvas* canvas = nullptr;
};

} // namespace alia

#endif
