#ifndef ALIA_TEXT_DISPLAY_HPP
#define ALIA_TEXT_DISPLAY_HPP

#include <alia/layout_interface.hpp>
#include <alia/text_utils.hpp>
#include <alia/flags.hpp>

namespace alia {

void do_text(
    context& ctx,
    char const* text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

void do_text(
    context& ctx,
    std::string const& text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

template<class T>
void do_text(
    context& ctx,
    T const& value,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS)
{
    do_text(ctx, to_string(value), layout_spec, flags);
}

void do_paragraph(
    context& ctx,
    char const* text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

void do_paragraph(
    context& ctx,
    std::string const& text,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS);

}

#endif
