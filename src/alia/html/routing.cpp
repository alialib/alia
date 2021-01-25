#include <alia/html/routing.hpp>

namespace alia { namespace html {

direct_const_signal<std::string>
get_location_hash(html::context ctx)
{
    return direct(const_cast<std::string const&>(get<system_tag>(ctx).hash));
}

element_handle
internal_link(
    context ctx, readable<std::string> text, readable<std::string> path)
{
    return element(ctx, "a")
        .attr("href", path)
        .attr("disabled", path.has_value() ? "false" : "true")
        .text(text)
        .callback("click", [&](emscripten::val& e) {
            e.call<void>("preventDefault");
            if (path.has_value())
            {
                set_location_hash(
                    get<html::system_tag>(ctx), "#" + read_signal(path));
            }
        });
}

}} // namespace alia::html