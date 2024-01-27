#include <alia/core/signals/text.hpp>

#include <sstream>

namespace alia {

template<class T>
bool
string_to_value(std::string const& str, T* value)
{
    std::istringstream s(str);
    T x;
    if (!(s >> x))
        return false;
    s >> std::ws;
    if (s.eof())
    {
        *value = x;
        return true;
    }
    return false;
}

template<class T>
std::string
value_to_string(T const& value)
{
    std::ostringstream s;
    s << value;
    return s.str();
}

template<class T>
void
float_from_string(T* value, std::string const& str)
{
    if (!string_to_value(str, value))
        throw validation_error("This input expects a number.");
}

#define ALIA_FLOAT_CONVERSIONS(T)                                             \
    void from_string(T* value, std::string const& str)                        \
    {                                                                         \
        float_from_string(value, str);                                        \
    }                                                                         \
    std::string to_string(T value)                                            \
    {                                                                         \
        return value_to_string(value);                                        \
    }

ALIA_FLOAT_CONVERSIONS(float)
ALIA_FLOAT_CONVERSIONS(double)

template<class T>
void
signed_integer_from_string(T* value, std::string const& str)
{
    long long n;
    if (!string_to_value(str, &n))
        throw validation_error("This input expects an integer.");
    T x = T(n);
    if (x != n)
        throw validation_error("This integer is outside the supported range.");
    *value = x;
}

template<class T>
void
unsigned_integer_from_string(T* value, std::string const& str)
{
    unsigned long long n;
    if (!string_to_value(str, &n))
        throw validation_error("This input expects an integer.");
    T x = T(n);
    if (x != n)
        throw validation_error("This integer is outside the supported range.");
    *value = x;
}

#define ALIA_SIGNED_INTEGER_CONVERSIONS(T)                                    \
    void from_string(T* value, std::string const& str)                        \
    {                                                                         \
        signed_integer_from_string(value, str);                               \
    }                                                                         \
    std::string to_string(T value)                                            \
    {                                                                         \
        return value_to_string(value);                                        \
    }

#define ALIA_UNSIGNED_INTEGER_CONVERSIONS(T)                                  \
    void from_string(T* value, std::string const& str)                        \
    {                                                                         \
        unsigned_integer_from_string(value, str);                             \
    }                                                                         \
    std::string to_string(T value)                                            \
    {                                                                         \
        return value_to_string(value);                                        \
    }

ALIA_SIGNED_INTEGER_CONVERSIONS(short int)
ALIA_UNSIGNED_INTEGER_CONVERSIONS(unsigned short int)
ALIA_SIGNED_INTEGER_CONVERSIONS(int)
ALIA_UNSIGNED_INTEGER_CONVERSIONS(unsigned int)
ALIA_SIGNED_INTEGER_CONVERSIONS(long int)
ALIA_UNSIGNED_INTEGER_CONVERSIONS(unsigned long int)
ALIA_SIGNED_INTEGER_CONVERSIONS(long long int)
ALIA_UNSIGNED_INTEGER_CONVERSIONS(unsigned long long int)

void
from_string(std::string* value, std::string const& str)
{
    *value = str;
}
std::string
to_string(std::string value)
{
    return value;
}

} // namespace alia
