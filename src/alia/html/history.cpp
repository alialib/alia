#include <alia/html/history.hpp>

#include <iostream>

namespace alia {
namespace html {

history_object::history_object()
    : object_(emscripten::val::global("window")["history"])
{
}

size_t
history_object::length()
{
    return object_["length"].as<size_t>();
}

void
history_object::back()
{
    object_.call<void>("back");
}

void
history_object::forward()
{
    object_.call<void>("forward");
}

void
history_object::go(int relative_position)
{
    object_.call<void>("go", relative_position);
}

void
history_object::push_state(
    emscripten::val const& state,
    std::string const& title,
    std::string const& url)
{
    object_.call<void>("pushState", state, title, url);
}

void
history_object::push_url(std::string const& url)
{
    this->push_state(emscripten::val::null(), "", url);
}

history_object
history()
{
    return history_object();
}

} // namespace html
} // namespace alia
