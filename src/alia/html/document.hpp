#ifndef ALIA_HTML_DOCUMENT_HPP
#define ALIA_HTML_DOCUMENT_HPP

#include <emscripten/val.h>

#include <alia/html/context.hpp>

namespace alia { namespace html {

struct document_object
{
    document_object();

    void
    set_title(std::string title);
    std::string
    get_title();
    __declspec(property(get = get_title, put = set_title)) std::string title;

    void
    set_body(emscripten::val body);
    emscripten::val
    get_body();
    __declspec(property(get = get_body, put = set_body)) emscripten::val body;

 private:
    emscripten::val val_;
};

void
document_title(html::context ctx, readable<std::string> title);

}} // namespace alia::html

#endif
