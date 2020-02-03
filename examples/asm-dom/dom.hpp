#ifndef DOM_HPP
#define DOM_HPP

#include "alia.hpp"
#include "asm-dom.hpp"
#include "color.hpp"

using std::string;

namespace dom {

using namespace alia;

struct context_info
{
    asmdom::Children* current_children;
};
ALIA_DEFINE_COMPONENT_TYPE(context_info_tag, context_info*)

typedef alia::add_component_type_t<alia::context, context_info_tag> context;

struct click_event : targeted_event
{
};

struct value_update_event : targeted_event
{
    string value;
};

void
do_text_(dom::context ctx, readable<std::string> text);

template<class Signal>
void
do_text(dom::context ctx, Signal text)
{
    do_text_(ctx, as_text(ctx, text));
}

struct input_data
{
    captured_id external_id;
    string value;
};

void
do_input_(dom::context ctx, bidirectional<string> value);

template<class Signal>
void
do_input(dom::context ctx, Signal signal)
{
    do_input_(ctx, as_bidirectional_text(ctx, signal));
}

void
do_number_input_(dom::context ctx, bidirectional<string> value);

template<class Signal>
void
do_number_input(dom::context ctx, Signal value)
{
    do_number_input_(ctx, as_bidirectional_text(ctx, value));
}

void
do_button(dom::context ctx, readable<std::string> text, action<> on_click);

void
do_colored_box(dom::context ctx, readable<rgb8> color);

struct dom_external_interface : alia::external_interface
{
    alia::system* system;

    void
    request_animation_refresh();
};

struct system
{
    std::function<void(dom::context)> controller;

    asmdom::VNode* current_view = nullptr;

    dom_external_interface external;

    alia::system alia_system;

    void
    operator()(alia::context ctx);
};

void
initialize(
    dom::system& dom_system,
    alia::system& alia_system,
    std::string const& dom_node_id,
    std::function<void(dom::context)> controller);

} // namespace dom

#endif
