#ifndef ALIA_HTML_ASYNC_HPP
#define ALIA_HTML_ASYNC_HPP

#include <functional>

namespace alia {
namespace html {

// Call a function object asynchronously after a delay.
//
// This is a simple wrapper around emscripten_async_call and has identical
// behavior with respect to 'millis'.
//
// See
// https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_async_call
//
void
async_call(std::function<void()> func, int millis);

} // namespace html
} // namespace alia

#endif
