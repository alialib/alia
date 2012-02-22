#ifndef ALIA_TEXT_UTILS_HPP
#define ALIA_TEXT_UTILS_HPP

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <alia/typedefs.hpp>

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
std::string to_string(boost::optional<T> const& value)
{
    return value ? str(boost::format("%s") % value.get()) : "";
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

template<class T>
bool from_string(boost::optional<T>* value, std::string const& str,
    std::string* message)
{
    if (str.empty())
    {
        *value = boost::optional<T>();
        return true;
    }
    else
    {
        T x;
        if (from_string(&x, str, message))
        {
            *value = x;
            return true;
        }
        else
            return false;
    }
}

template<class T>
bool integer_from_string(T* value, std::string const& str,
    std::string* message)
{
    try
    {
        int64 n = boost::lexical_cast<int64>(str);
        *value = boost::numeric_cast<T>(n);
        return true;
    }
    catch (boost::bad_lexical_cast)
    {
        *message = std::string("This input expects ") +
            type_to_string<T>::get() + ".";
        return false;
    }
    catch (boost::bad_numeric_cast&)
    {
        *message = "integer out of range";
        return false;
    }
}

#define ALIA_INTEGER_FROM_STRING(T) \
    static inline bool from_string( \
        T* value, std::string const& str, std::string* message) \
    { return integer_from_string(value, str, message); }

//ALIA_INTEGER_FROM_STRING(int)
//ALIA_INTEGER_FROM_STRING(unsigned)
ALIA_INTEGER_FROM_STRING(int64)
ALIA_INTEGER_FROM_STRING(uint64)
ALIA_INTEGER_FROM_STRING(int32)
ALIA_INTEGER_FROM_STRING(uint32)
ALIA_INTEGER_FROM_STRING(int16)
ALIA_INTEGER_FROM_STRING(uint16)
ALIA_INTEGER_FROM_STRING(int8)
ALIA_INTEGER_FROM_STRING(uint8)

}

#endif
