#include <alia/html/document.hpp>

#include <emscripten/val.h>

namespace alia { namespace html {

document_object::document_object() : val_(emscripten::val::global("document"))
{
}

void
document_object::set_title(std::string title)
{
    val_.set("title", std::move(title));
}
std::string
document_object::get_title()
{
    return val_["title"].as<std::string>();
}

void
document_object::set_body(emscripten::val body)
{
    val_.set("body", std::move(body));
}
emscripten::val
document_object::get_body()
{
    return val_["body"];
}

void
document_title(html::context ctx, readable<std::string> title)
{
    auto& id = get_data<captured_id>(ctx);
    refresh_signal_view(
        id,
        add_default(title, ""),
        [](auto new_title) {
            emscripten::val::global("document").set("title", new_title);
        },
        [] {});
}

}} // namespace alia::html
