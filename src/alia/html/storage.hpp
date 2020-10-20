#ifndef ALIA_HTML_STORAGE_HPP
#define ALIA_HTML_STORAGE_HPP

#include <emscripten/val.h>

namespace alia {
namespace html {

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

} // namespace html
} // namespace alia

#endif
