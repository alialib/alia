#ifndef ALIA_RADIO_BUTTON_HPP
#define ALIA_RADIO_BUTTON_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <string>

namespace alia {

// accepted flags: all control flags

bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

// radio button with text - The text is placed to the right of the radio button
// inside a row object, and the radio button can be interacted with by clicking
// on either the button itself or the text.
// accepted flags: all control flags, STATIC (for the text)

// char const* version
bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    char const* text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

// std::string version
bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    std::string const& text,
    unsigned flags = 0,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

//class radio_button_group
//{
// public:
//    radio_button_group(context& context, value_parameter<int> const& value);
//    radio_button_group(value_parameter<int> const& value);
//    radio_button_group(manual_init) {}
//
//    init(context& context, value_parameter<bool> const& value);
//
//    bool do_button(radio_button_data& data, std::string const& text = "",
//        layout const& layout_spec = default_layout, bool enabled = false);
//
//    bool do_button(std::string const& text = "",
//        layout const& layout_spec = default_layout, bool enabled = false);
//
//    bool changed() const { return changed_; }
//
// private:
//    context* ctx_;
//    value_parameter<int> value_;
//    bool changed_;
//    int index_;
//};

}

#endif
