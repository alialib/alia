#include "fetch.hpp"

#include <emscripten/fetch.h>

namespace alia { namespace html {

std::string
to_string(http_method method)
{
    switch (method)
    {
        case http_method::POST:
            return "POST";
        case http_method::GET:
        default:
            return "GET";
        case http_method::PUT:
            return "PUT";
        case http_method::DELETE:
            return "DELETE";
    }
}

blob
to_blob(std::string s)
{
    std::shared_ptr<char> storage(new char[s.size()], array_deleter<char>());
    memcpy(storage.get(), s.data(), s.size());
    return blob{storage.get(), s.size(), storage};
}

namespace {

struct fetch_user_data
{
    http_request request;
    async_reporter<http_response> reporter;
};

struct scoped_emscripten_fetch : noncopyable
{
    explicit scoped_emscripten_fetch(emscripten_fetch_t* fetch) : fetch_(fetch)
    {
    }

    ~scoped_emscripten_fetch()
    {
        emscripten_fetch_close(fetch_);
    }

 private:
    emscripten_fetch_t* fetch_;
};

void
handle_fetch_response(emscripten_fetch_t* fetch)
{
    std::shared_ptr<scoped_emscripten_fetch> fetch_ownership(
        new scoped_emscripten_fetch(fetch));

    // Grab our data from the Emscripten fetch object and assume ownership of
    // it.
    std::unique_ptr<fetch_user_data> data(
        reinterpret_cast<fetch_user_data*>(fetch->userData));

    // Construct the response.
    http_response response;
    response.status_code = fetch->status;
    response.body = blob{fetch->data, fetch->numBytes, fetch_ownership};

    // Invoke the callback.
    data->reporter.report_success(response);
}

void
launch_fetch_operation(
    alia::dataless_context ctx,
    async_reporter<http_response> reporter,
    http_request const& request)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    attr.onsuccess = handle_fetch_response;
    // Emscripten doesn't seem to have a way to distinguish between actual
    // network errors and HTTP status codes outside the 2xx range, so I think
    // the best approach for now is to just send back all responses as part of
    // the normal data flow and let the application handle non-2xx codes how it
    // sees fit.
    attr.onerror = handle_fetch_response;

    std::unique_ptr<fetch_user_data> user_data(new fetch_user_data);
    user_data->request = request;
    user_data->reporter = reporter;
    auto const& persistent_request = user_data->request;

    std::vector<char const*> headers;
    headers.reserve(request.headers.size() * 2 + 1);
    for (auto const& h : persistent_request.headers)
    {
        headers.push_back(h.first.c_str());
        headers.push_back(h.second.c_str());
    }
    headers.push_back(0);
    attr.requestHeaders = &headers[0];

    attr.requestData = persistent_request.body.data;
    attr.requestDataSize = persistent_request.body.size;

    auto method_string = to_string(request.method);

    attr.userData = user_data.get();

    strcpy(attr.requestMethod, method_string.c_str());
    emscripten_fetch(&attr, request.url.c_str());

    user_data.release();
}

} // namespace

async_signal<http_response>
fetch(alia::context ctx, readable<http_request> request)
{
    return async<http_response>(ctx, launch_fetch_operation, request);
}

namespace {

http_request
make_text_request(std::string const& path)
{
    return http_request{http_method::GET, path, http_headers{}, blob()};
}

} // namespace

apply_signal<std::string>
fetch_text(alia::context ctx, readable<std::string> path)
{
    return apply(
        ctx,
        [](auto result) {
            return std::string(result.body.data, result.body.size);
        },
        fetch(ctx, apply(ctx, make_text_request, path)));
}

}} // namespace alia::html
