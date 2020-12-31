#ifndef ALIA_HTML_CANVAS_HPP
#define ALIA_HTML_CANVAS_HPP

namespace alia {
namespace html {

void
clear_canvas(int asmdom_id);

void
set_fill_style(int asmdom_id, char const* style);

void
fill_rect(int asmdom_id, double x, double y, double width, double height);

} // namespace html
} // namespace alia

#endif
