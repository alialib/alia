#include <alia/html/document.hpp>

#include <emscripten/val.h>

namespace alia { namespace html {

void
document_title(html::context ctx, readable<std::string> title)
{
    auto& id = get_data<captured_id>(ctx);
    refresh_signal_view(
        id,
        add_default(title, ""),
        [](auto new_title) {
            emscripten::val::global("document")
                .set("title", emscripten::val(new_title));
        },
        [] {});
}

}} // namespace alia::html
