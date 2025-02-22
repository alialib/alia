#ifndef ALIA_HTML_FETCH_HPP
#define ALIA_HTML_FETCH_HPP

#include <alia/core.hpp>

#include <map>
#include <string>

namespace alia { namespace html {

enum class http_method
{
    POST,
    GET,
    PUT,
    DELETE
};

std::string
to_string(http_method method);

typedef std::map<std::string, std::string> http_headers;

struct http_request
{
    http_method method;
    std::string url;
    http_headers headers;
    blob body;
};

struct http_response
{
    int status_code;
    blob body;
};

struct http_error
{
    http_request request;
    http_response response;
};

async_signal<http_response>
fetch(alia::core_context ctx, readable<http_request> request);

apply_signal<std::string>
fetch_text(alia::core_context ctx, readable<std::string> path);

inline apply_signal<std::string>
fetch_text(alia::core_context ctx, char const* path)
{
    return fetch_text(ctx, value(path));
}

}} // namespace alia::html

#endif
