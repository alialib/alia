#ifndef ALIA_ENUM_HPP
#define ALIA_ENUM_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/flags.hpp>
#include <vector>
#include <string>

namespace alia {

template<class T>
struct enum_result : control_result<T>
{};

static flag_set const USE_RADIO_GROUP(CUSTOM_FLAG_0_CODE);

enum_result<unsigned>
do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    std::vector<std::string> const& options,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

enum_result<unsigned>
do_enum(
    context& ctx,
    accessor<unsigned> const& value,
    char const* const* options,
    unsigned n_options,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

template<typename T>
enum_result<T>
do_enum(
    context& ctx,
    accessor<T> const& value,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout)
{
    enum_result<unsigned> r = do_enum(ctx,
        accessor_cast<unsigned>(ref(value)),
        get_printable_value_strings(T()), flags, layout_spec);
    enum_result<T> r2;
    r2.changed = r.changed;
    r2.new_value = T(r.new_value);
    return r2;
}

}

#endif
