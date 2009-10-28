#ifndef ALIA_RADIO_BUTTON_HPP
#define ALIA_RADIO_BUTTON_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/flags.hpp>
#include <string>

namespace alia {

// accepted flags: all control flags

bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    flag_set flags = NO_FLAGS,
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
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

// std::string version
bool do_radio_button(
    context& ctx,
    accessor<unsigned> const& value,
    unsigned index,
    std::string const& text,
    flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout,
    region_id id = auto_id);

//class radio_button_group
//{
// public:
//    radio_button_group() {}
//    radio_button_group(context& ctx, accessor<int> const& value);
//
//    void begin(context& ctx, accessor<bool> const& value);
//
//    bool do_button(layout const& layout_spec = default_layout,
//        region_id id = auto_id);
//
//    bool changed() const { return changed_; }
//
// private:
//    context* ctx_;
//    bool changed_;
//    int index_;
//};

}

#endif
