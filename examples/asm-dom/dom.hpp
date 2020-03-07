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
ALIA_DEFINE_COMPONENT_TYPE(context_info_tag, context_info&)

typedef alia::add_component_type_t<alia::context, context_info_tag> context;

void
do_text_(dom::context ctx, readable<std::string> text);

template<class Signal>
void
do_text(dom::context ctx, Signal text)
{
    do_text_(ctx, as_text(ctx, signalize(text)));
}

void
do_heading_(
    dom::context ctx, readable<std::string> level, readable<std::string> text);

template<class Level, class Text>
void
do_heading(dom::context ctx, Level level, Text text)
{
    do_heading_(ctx, signalize(level), as_text(ctx, signalize(text)));
}

void
do_input_(dom::context ctx, duplex<string> value);

template<class Signal>
void
do_input(dom::context ctx, Signal signal)
{
    do_input_(ctx, as_duplex_text(ctx, signal));
}

void
do_button_(dom::context ctx, readable<std::string> text, action<> on_click);

template<class Text>
void
do_button(dom::context ctx, Text text, action<> on_click)
{
    do_button_(ctx, signalize(text), on_click);
}

void
do_colored_box(dom::context ctx, readable<rgb8> color);

void
do_hr(dom::context ctx);

struct div_data;

struct scoped_div : noncopyable
{
    scoped_div(dom::context ctx, readable<std::string> class_name)
    {
        begin(ctx, class_name);
    }

    ~scoped_div()
    {
        end();
    }

    void
    begin(dom::context ctx, readable<std::string> class_name);

    void
    end();

    bool
    is_relevant()
    {
        return routing_.is_relevant();
    }

 private:
    std::optional<dom::context> ctx_;
    div_data* data_ = nullptr;
    asmdom::Children* parent_children_list_ = nullptr;
    scoped_routing_region routing_;
};

template<class DoContents>
void
do_div(
    dom::context ctx, readable<std::string> class_name, DoContents do_contents)
{
    scoped_div div(ctx, class_name);
    do_contents(ctx);
}

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
