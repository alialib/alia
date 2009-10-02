#ifndef ALIA_TEXT_UTILS_HPP
#define ALIA_TEXT_UTILS_HPP

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace alia {

template<class T>
struct type_to_string
{
    static char const* get() { return typeid(T).name(); }
};
template<>
struct type_to_string<float>
{
    static char const* get() { return "a number"; }
};
template<>
struct type_to_string<double>
{
    static char const* get() { return "a number"; }
};
template<>
struct type_to_string<int>
{
    static char const* get() { return "an integer"; }
};
template<>
struct type_to_string<unsigned int>
{
    static char const* get() { return "a non-negative integer"; }
};

template<class T>
std::string to_string(T value)
{
    return str(boost::format("%s") % value);
}

template<class T>
bool from_string(T* value, std::string const& str, std::string* message)
{
    try
    {
        *value = boost::lexical_cast<T>(str);
        return true;
    }
    catch (boost::bad_lexical_cast)
    {
        *message = std::string("This input expects ") +
            type_to_string<T>::get() + ".";
        return false;
    }
}

}

#endif
