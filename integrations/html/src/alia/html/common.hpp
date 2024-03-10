#ifndef ALIA_HTML_COMMON_HPP
#define ALIA_HTML_COMMON_HPP

#include <any>

namespace alia { namespace html {

struct blob
{
    char const* data = nullptr;
    uint64_t size = 0;
    // :ownership stores an (optional) object that provides ownership of :data.
    std::any ownership;
};

template<typename T>
struct array_deleter
{
    void
    operator()(T const* p)
    {
        delete[] p;
    }
};

blob
to_blob(std::string s);

}} // namespace alia::html

#endif
