#ifndef ALIA_BACKENDS_EMSCRIPTEN_HPP
#define ALIA_BACKENDS_EMSCRIPTEN_HPP

#include <memory>

#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

struct emscripten_canvas_impl;

struct emscripten_canvas
{
    emscripten_canvas(std::function<void(ui_context)> controller);
    ~emscripten_canvas();

    void
    do_main_loop();

 private:
    std::unique_ptr<emscripten_canvas_impl> impl_;
};

} // namespace alia

#endif
