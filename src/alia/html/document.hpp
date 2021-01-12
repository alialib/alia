#ifndef ALIA_HTML_DOCUMENT_HPP
#define ALIA_HTML_DOCUMENT_HPP

#include <emscripten/val.h>

#include <alia/html/context.hpp>

namespace alia { namespace html {

void
document_title(html::context ctx, readable<std::string> title);

}} // namespace alia::html

#endif
