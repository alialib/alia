#include <alia/html/routing.hpp>

namespace alia { namespace html {

direct_const_signal<std::string>
get_location_hash(html::context ctx)
{
    return direct(const_cast<std::string const&>(get<system_tag>(ctx).hash));
}

namespace detail {

element_handle
internal_link(context ctx, readable<std::string> path)
{
    return element(ctx, "a")
        .attr("href", "#" + path)
        .attr("disabled", path.has_value() ? "false" : "true")
        .callback("click", [&](emscripten::val& e) {
            e.call<void>("preventDefault");
            if (path.has_value())
            {
                set_location_hash(
                    get<html::system_tag>(ctx), "#" + read_signal(path));
            }
        });
}

element_handle
internal_link(
    context ctx, readable<std::string> text, readable<std::string> path)
{
    return detail::internal_link(ctx, path).text(text);
}

} // namespace detail

}} // namespace alia::html
