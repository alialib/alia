#ifndef ALIA_SIGNALS_TEXT_HPP
#define ALIA_SIGNALS_TEXT_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/basic.hpp>

#include <cstdio>

namespace alia {

// The following implements a very minimal C++-friendly version of printf that
// works with signals.

template<class Value>
Value
make_printf_friendly(Value x)
{
    return x;
}

inline char const*
make_printf_friendly(std::string const& x)
{
    return x.c_str();
}

struct printf_format_error : exception
{
    printf_format_error() : exception("printf format error")
    {
    }
};

template<class... Args>
std::string
invoke_snprintf(std::string const& format, Args const&... args)
{
    int length
        = std::snprintf(0, 0, format.c_str(), make_printf_friendly(args)...);
    if (length < 0)
        throw printf_format_error();
    std::string s;
    if (length > 0)
    {
        s.resize(length);
        std::snprintf(
            &s[0], length + 1, format.c_str(), make_printf_friendly(args)...);
    }
    return s;
}

template<class Format, class... Args>
auto
printf(context ctx, Format format, Args... args)
{
    return apply(
        ctx,
        ALIA_LAMBDIFY(invoke_snprintf),
        signalize(format),
        signalize(args)...);
}

// All conversion of values to and from text goes through the functions
// from_string and to_string. In order to use a particular value type with
// the text-based widgets and utilities provided here, that type must
// implement these functions.

#define ALIA_DECLARE_STRING_CONVERSIONS(T)                                     \
    void from_string(T* value, std::string const& s);                          \
    std::string to_string(T value);

// from_string(value, s) should parse the string s and store it in *value.
// It should throw a validation_error if the string doesn't parse.

// to_string(value) should simply return the string form of value.

// Implementations of from_string and to_string are provided for the following
// types.

ALIA_DECLARE_STRING_CONVERSIONS(short int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned short int)
ALIA_DECLARE_STRING_CONVERSIONS(int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned int)
ALIA_DECLARE_STRING_CONVERSIONS(long int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned long int)
ALIA_DECLARE_STRING_CONVERSIONS(long long int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned long long int)
ALIA_DECLARE_STRING_CONVERSIONS(float)
ALIA_DECLARE_STRING_CONVERSIONS(double)
ALIA_DECLARE_STRING_CONVERSIONS(std::string)

// as_text(ctx, x) creates a text-based interface to the signal x.
template<class Readable>
void
update_text_conversion(keyed_data<std::string>* data, Readable x)
{
    if (signal_has_value(x))
    {
        refresh_keyed_data(*data, x.value_id());
        if (!is_valid(*data))
            set(*data, to_string(read_signal(x)));
    }
    else
    {
        invalidate(*data);
    }
}
template<class Readable>
keyed_data_signal<std::string>
as_text(context ctx, Readable x)
{
    keyed_data<std::string>* data;
    get_cached_data(ctx, &data);
    update_text_conversion(data, x);
    return keyed_data_signal<std::string>(data);
}

// as_duplex_text(ctx, x) is similar to as_text but it's duplex.
template<class Value>
struct duplex_text_data
{
    captured_id input_id;
    Value input_value;
    bool output_valid = false;
    std::string output_text;
    counter_type output_version = 1;
};
template<class Value, class Readable>
void
update_duplex_text(duplex_text_data<Value>* data, Readable x)
{
    if (signal_has_value(x))
    {
        auto const& input_id = x.value_id();
        if (!data->input_id.matches(input_id))
        {
            if (!data->output_valid || read_signal(x) != data->input_value)
            {
                data->input_value = read_signal(x);
                data->output_text = to_string(read_signal(x));
                data->output_valid = true;
                ++data->output_version;
            }
            data->input_id.capture(input_id);
        }
    }
    else
    {
        data->output_valid = false;
    }
}
template<class Wrapped>
struct duplex_text_signal
    : casting_signal_wrapper<duplex_text_signal<Wrapped>, Wrapped, std::string>
{
    duplex_text_signal(
        Wrapped wrapped, duplex_text_data<typename Wrapped::value_type>* data)
        : duplex_text_signal::casting_signal_wrapper(wrapped), data_(data)
    {
    }
    bool
    has_value() const
    {
        return data_->output_valid;
    }
    std::string const&
    read() const
    {
        return data_->output_text;
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->output_version);
        return id_;
    }
    void
    write(std::string s) const
    {
        typename Wrapped::value_type value;
        from_string(&value, s);
        data_->input_value = value;
        this->wrapped_.write(std::move(value));
        data_->output_text = s;
        ++data_->output_version;
    }

 private:
    duplex_text_data<typename Wrapped::value_type>* data_;
    mutable simple_id<counter_type> id_;
};
template<class Signal>
duplex_text_signal<Signal>
as_duplex_text(context ctx, Signal x)
{
    duplex_text_data<typename Signal::value_type>* data;
    get_cached_data(ctx, &data);
    update_duplex_text(data, x);
    return duplex_text_signal<Signal>(x, data);
}

} // namespace alia

#endif
