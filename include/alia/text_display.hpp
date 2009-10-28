#ifndef ALIA_TEXT_DISPLAY_HPP
#define ALIA_TEXT_DISPLAY_HPP

#include <alia/layout_interface.hpp>
#include <alia/text_utils.hpp>
#include <alia/flags.hpp>

namespace alia {

void do_text(
    context& ctx,
    char const* text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

void do_text(
    context& ctx,
    std::string const& text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

template<class T>
void do_text(
    context& ctx,
    T const& value,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout)
{
    do_text(ctx, to_string(value), flags, layout_spec);
}

void do_paragraph(
    context& ctx,
    char const* text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

void do_paragraph(
    context& ctx,
    std::string const& text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout);

}

#endif
