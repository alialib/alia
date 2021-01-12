#include "demo.hpp"

#include <nlohmann/json.hpp>

#include <emscripten/fetch.h>

using json = nlohmann::json;

namespace country_fetch {

/// [fetch-country]
// This is what the app sees as the result of the fetch.
struct fetch_result
{
    bool successful;
    std::string country_name;
};

// Here's our handler for the fetch results. (This is called asynchronously.)
void
handle_fetch_result(emscripten_fetch_t* fetch, fetch_result result)
{
    // Recover our callback from the Emscripten fetch object.
    auto* result_callback
        = reinterpret_cast<std::function<void(fetch_result)>*>(
            fetch->userData);

    // Report the result.
    (*result_callback)(result);

    // Clean up.
    delete result_callback;
    emscripten_fetch_close(fetch);
}
// And these are the actual callbacks that we give to Emscripten: one for
// success and one for failure...
void
handle_fetch_success(emscripten_fetch_t* fetch)
{
    // Parse the JSON response and pass it along.
    auto response = json::parse(fetch->data, fetch->data + fetch->numBytes);
    handle_fetch_result(fetch, fetch_result{true, response["name"]});
}
void
handle_fetch_failure(emscripten_fetch_t* fetch)
{
    // Pass along an unsuccessful/empty result.
    handle_fetch_result(fetch, fetch_result{false, std::string()});
}

// Here's our actual component-level function for retrieving country names.
auto
fetch_country_name(html::context ctx, readable<std::string> country_code)
{
    // This will be invoked to launch the fetch operation whenever necessary
    // (i.e., whenever we get a new country code).
    auto launcher = [](auto ctx, auto reporter, auto country_code) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess = handle_fetch_success;
        attr.onerror = handle_fetch_failure;
        attr.userData = new std::function<void(fetch_result)>(
            [reporter](fetch_result result) {
                reporter.report_success(result);
            });
        auto url = "https://restcountries.eu/rest/v2/alpha/" + country_code;
        emscripten_fetch(&attr, url.c_str());
    };
    // async() handles all the underlying logic of determining when to invoke
    // the launcher and how to get results back to this point in the UI.
    return async<fetch_result>(ctx, launcher, country_code);
}

// And here's the UI for interacting with it.
void
fetch_ui(html::context ctx, duplex<std::string> country_code)
{
    html::p(ctx, "Enter a country code:");
    html::input(ctx, country_code);
    auto result = fetch_country_name(ctx, country_code);
    html::p(
        ctx,
        add_default(
            deflicker(
                ctx,
                conditional(
                    alia_field(result, successful),
                    alia_field(result, country_name),
                    "Not found")),
            "Fetching..."));
}
/// [fetch-country]

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) {
        fetch_ui(ctx, get_state(ctx, "us"));
    });
}

static demo the_demo("fetch-country", init_demo);

} // namespace country_fetch
