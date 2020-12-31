#ifndef ALIA_HTML_STORAGE_HPP
#define ALIA_HTML_STORAGE_HPP

#include <alia/html/context.hpp>

#include <emscripten/val.h>

namespace alia {
namespace html {

// RAW ACCESS

struct storage_object
{
    explicit storage_object(std::string const& name);

    size_t
    length();

    void
    set_item(std::string const& key, std::string const& value);

    bool
    has_item(std::string const& key);

    std::string
    get_item(std::string const& key);

    void
    remove_item(std::string const& key);

    void
    clear();

 private:
    emscripten::val object_;
};

storage_object
local_storage();

storage_object
session_storage();

// SIGNALIZED ACCESS

duplex<std::string>
get_storage_state(
    html::context ctx,
    std::string const& storage_name,
    std::string const& key,
    readable<std::string> default_value);

auto
get_session_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value);

auto
get_local_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value);

} // namespace html
} // namespace alia

#endif
