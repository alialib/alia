#include <alia/html/bootstrap.hpp>
#include <alia/html/document.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/storage.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demolib/utilities.hpp"

using namespace alia;
namespace bs = alia::html::bootstrap;

// clang-format off
/// [fetch-demo]
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Construct the request object for retrieving a country's info.
html::http_request
make_country_request(std::string const& country_code)
{
    return html::http_request{
        html::http_method::GET,
        "https://restcountries.com/v3.1/alpha/" + country_code,
        html::http_headers(), // no headers
        html::blob()}; // no body
}

// Use nlohmann::json to parse the response and extract the name.
std::optional<std::string>
parse_country_response(html::http_response const& response)
{
    // The REST Countries API returns an error status code if the country isn't
    // found.
    if (response.status_code != 200)
        return std::nullopt;

    auto const& body = response.body;
    return json::parse(body.data, body.data + body.size)["name"];
}

// Here's our signal-based interface for fetching country names.
// This essentially defines a small dataflow pipeline that maps country codes
// to country names via a trip to the REST Countries server.
auto
fetch_country_name(html::context ctx, readable<std::string> country_code)
{
    auto request = apply(ctx, make_country_request, country_code);

    // html::fetch() does the brunt of the work. It takes care of detecting
    // when the input request changes, issuing the new request, and presenting
    // the response (when it arrives) as its output.
    auto response = html::fetch(ctx, request);

    return apply(ctx, parse_country_response, response);
}

// And here's the UI for interacting with it.
void
country_name_lookup_ui(html::context ctx)
{
    auto country_code = get_state(ctx, "us");
    p(ctx, "Enter a country code:");
    input(ctx, country_code);

    auto name = fetch_country_name(ctx, country_code);
    p(ctx,
        add_default(
            deflicker(ctx, conditional(name, *name, "Not found")),
            "Fetching..."));
}
/// [fetch-demo]
// clang-format on

void
fetch_demo(demo_context ctx)
{
    section_heading(ctx, "fetch", "HTTP Fetch");
    p(ctx, [&] {
        text(ctx, "This demo uses the ");
        link(ctx, "REST Countries API", "https://restcountries.com/");
        text(
            ctx,
            " (directly from your browser) to translate two- and three-letter "
            "country codes into country names.");
    });
    div(ctx, "demo-panel", [&] { country_name_lookup_ui(ctx); });
    code_snippet(ctx, "fetch-demo");
}

void
storage_demo(demo_context ctx)
{
    section_heading(ctx, "storage", "Storage");

    subsection_heading(ctx, "Local");

    p(ctx,
      "The input in this demo is connected to the HTML5 local storage API "
      "of your browser. If you edit it, close this page and reopen it, "
      "you'll see the same value.");

    p(ctx,
      "You can even open this page in a second window, edit the value "
      "there, and watch it update here.");

    div(ctx, "demo-panel", [&] {
        /// [local-storage]
        input(ctx, get_local_state(ctx, "demo-local-storage-key"));
        /// [local-storage]
    });

    code_snippet(ctx, "local-storage");

    subsection_heading(ctx, "Session");

    p(ctx,
      "The input in this demo is connected to the HTML5 session storage "
      "API of your browser. This value will survive refreshes, but it will "
      "reset if you close the page, and it's not shared across multiple "
      "instances of the page.");

    div(ctx, "demo-panel", [&] {
        /// [session-storage]
        input(ctx, get_session_state(ctx, "demo-session-storage-key"));
        /// [session-storage]
    });

    code_snippet(ctx, "session-storage");
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        document_title(ctx, "alia/HTML I/O Demos");

        placeholder_root(ctx, "demos", [&] {
            h1(ctx).classes("mt-5 mb-3").text("alia/HTML I/O Demos");

            p(ctx,
              "The following demonstrate some of the I/O capabilities of "
              "alia/HTML.");

            storage_demo(ctx);
            fetch_demo(ctx);

            div(ctx, "my-5");
        });
    });
}

int
main()
{
    static html::system sys;
    initialize(sys, root_ui);
};
