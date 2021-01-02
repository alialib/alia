#include <alia/html/canvas.hpp>

#include <emscripten/emscripten.h>

namespace alia { namespace html {

void
clear_canvas(int asmdom_id)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.clearRect(0, 0, canvas.width, canvas.height);
        },
        asmdom_id);
}

void
set_fill_style(int asmdom_id, char const* style)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.fillStyle = Module['UTF8ToString']($1);
        },
        asmdom_id,
        style);
}

void
fill_rect(int asmdom_id, double x, double y, double width, double height)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.fillRect($1, $2, $3, $4);
        },
        asmdom_id,
        x,
        y,
        width,
        height);
}

}} // namespace alia::html
