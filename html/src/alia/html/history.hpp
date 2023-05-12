#ifndef ALIA_HTML_HISTORY_HPP
#define ALIA_HTML_HISTORY_HPP

#include <emscripten/val.h>

namespace alia { namespace html {

struct history_object
{
    history_object();

    size_t
    length();

    void
    back();

    void
    forward();

    void
    go(int relative_position = 0);

    void
    push_state(
        emscripten::val const& state,
        std::string const& title,
        std::string const& url);

    void
    push_url(std::string const& url);

 private:
    emscripten::val object_;
};

history_object
history();

}} // namespace alia::html

#endif
