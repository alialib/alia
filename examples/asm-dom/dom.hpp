#ifndef DOM_HPP
#define DOM_HPP

#include "alia.hpp"
#include "asm-dom.hpp"
#include "color.hpp"

#include <functional>

#include <emscripten/emscripten.h>
#include <emscripten/val.h>

using std::string;

namespace dom {

using namespace alia;

struct element_object;

ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<element_object>&)

typedef alia::extend_context_type_t<alia::context, tree_traversal_tag> context;

struct dom_event : targeted_event
{
    dom_event(emscripten::val event) : event(event)
    {
    }
    emscripten::val event;
};

struct callback_data
{
    component_identity identity;
    std::function<void(emscripten::val)> callback;
};

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type);

struct element_object
{
    void
    create_as_element(char const* type)
    {
        assert(this->js_id == 0);
        this->js_id = EM_ASM_INT(
            { return Module.createElement(Module['UTF8ToString']($0)); },
            type);
    }

    void
    create_as_text_node(char const* value)
    {
        assert(this->js_id == 0);
        this->js_id = EM_ASM_INT(
            { return Module.createTextNode(Module['UTF8ToString']($0)); },
            value);
    }

    void
    relocate(
        element_object& parent, element_object* after, element_object* before)
    {
        assert(this->js_id != 0);
        EM_ASM_(
            { Module.insertBefore($0, $1, $2); },
            parent.js_id,
            this->js_id,
            before ? before->js_id : 0);
    }

    void
    remove()
    {
        assert(this->js_id != 0);
        EM_ASM_({ Module.removeChild($0); }, this->js_id);
        // In asm-dom, removing a child from its parent also destroys it, so we
        // have to mark it as uninitialized here.
        this->js_id = 0;
    }

    ~element_object()
    {
        if (this->js_id != 0)
        {
            this->remove();
        }
    }

    int js_id = 0;
};

void
text_node_(dom::context ctx, readable<string> text);

template<class Text>
void
text_node(dom::context ctx, Text text)
{
    text_node_(ctx, as_text(ctx, signalize(text)));
}

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<string> value);

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<bool> value);

void
set_element_property(
    element_object& object, char const* name, emscripten::val value);

void
clear_element_property(element_object& object, char const* name);

template<class Value>
void
do_element_property(
    context ctx, element_object& object, char const* name, Value const& value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_shadow(
            stored_id,
            value,
            [&](auto const& new_value) {
                set_element_property(object, name, emscripten::val(new_value));
            },
            [&]() { clear_element_property(object, name); });
    });
}

template<class Context>
struct element_handle : noncopyable
{
    element_handle(Context ctx, char const* type) : ctx_(ctx)
    {
        initializing_ = get_cached_data(ctx, &node_);
        if (initializing_)
            node_->object.create_as_element(type);
        if (is_refresh_event(ctx))
            refresh_tree_node(get_object<tree_traversal_tag>(ctx), *node_);
    }

    template<class Value>
    element_handle&
    attr(char const* name, Value value)
    {
        do_element_attribute(ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Value>
    element_handle&
    prop(char const* name, Value value)
    {
        do_element_property(ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Function>
    element_handle&
    callback(char const* event_type, Function&& fn)
    {
        auto& data = get_cached_data<callback_data>(ctx_);
        if (initializing_)
            install_element_callback(ctx_, node_->object, data, event_type);
        on_targeted_event<dom_event>(
            ctx_, &data.identity, [&](auto ctx, auto& e) { fn(e.event); });
        return *this;
    }

    template<class Function>
    element_handle&
    children(Function&& fn)
    {
        auto& traversal = get<tree_traversal_tag>(ctx_);
        scoped_tree_children<element_object> tree_scope;
        if (is_refresh_event(ctx_))
            tree_scope.begin(traversal, *node_);
        fn(ctx_);
        return *this;
    }

    template<class Text>
    element_handle&
    text(Text text)
    {
        return children([&](auto ctx) { text_node(ctx, text); });
    }

 private:
    Context ctx_;
    tree_node<element_object>* node_;
    bool initializing_;
};

template<class Context>
element_handle<Context>
element(Context ctx, char const* type)
{
    return element_handle<Context>(ctx, type);
}

struct scoped_element : noncopyable
{
    scoped_element()
    {
    }
    scoped_element(context ctx, char const* type)
    {
        begin(ctx, type);
    }
    ~scoped_element()
    {
        end();
    }

    scoped_element&
    begin(context ctx, char const* type)
    {
        ctx_.reset(ctx);
        initializing_ = get_cached_data(ctx, &node_);
        if (initializing_)
            node_->object.create_as_element(type);
        if (is_refresh_event(ctx))
            tree_scoping_.begin(get_object<tree_traversal_tag>(ctx), *node_);
        return *this;
    }

    void
    end()
    {
        tree_scoping_.end();
    }

    template<class Value>
    scoped_element&
    attr(char const* name, Value value)
    {
        do_element_attribute(*ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Value>
    scoped_element&
    prop(char const* name, Value value)
    {
        do_element_property(*ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Function>
    scoped_element&
    callback(char const* event_type, Function&& fn)
    {
        auto ctx = *ctx_;
        auto& data = get_cached_data<callback_data>(ctx);
        if (initializing_)
            install_element_callback(ctx, node_->object, data, event_type);
        on_targeted_event<dom_event>(
            ctx, &data.identity, [&](auto ctx, auto& e) { fn(e.event); });
        return *this;
    }

 private:
    optional_context<context> ctx_;
    tree_node<element_object>* node_;
    scoped_tree_node<element_object> tree_scoping_;
    bool initializing_;
};

template<class Signal>
void
text(dom::context ctx, Signal signal)
{
    dom::element(ctx, "p").text(signal);
}

void
input_(dom::context ctx, duplex<string> value);

template<class Signal>
void
input(dom::context ctx, Signal signal)
{
    input_(ctx, as_duplex_text(ctx, signal));
}

void
button_(dom::context ctx, readable<std::string> text, action<> on_click);

template<class Text>
void
button(dom::context ctx, Text text, action<> on_click)
{
    button_(ctx, signalize(text), on_click);
}

void
checkbox_(dom::context ctx, duplex<bool> value, readable<std::string> label);

template<class Label>
void
checkbox(dom::context ctx, duplex<bool> value, Label label)
{
    checkbox_(ctx, value, signalize(label));
}

void
link_(dom::context ctx, readable<std::string> text, action<> on_click);

template<class Text>
void
link(dom::context ctx, Text text, action<> on_click)
{
    link_(ctx, signalize(text), on_click);
}

void
colored_box(dom::context ctx, readable<rgb8> color);

inline void
colored_box(dom::context ctx, rgb8 const& color)
{
    colored_box(ctx, value(color));
}

struct div_data;

struct scoped_div : scoped_element
{
    scoped_div()
    {
    }
    scoped_div(dom::context ctx, readable<std::string> class_name)
    {
        begin(ctx, class_name);
    }
    ~scoped_div()
    {
        end();
    }

    scoped_div&
    begin(dom::context ctx, readable<std::string> class_name)
    {
        this->scoped_element::begin(ctx, "div").attr("class", class_name);
        return *this;
    }
};

struct cached_content_data
{
    component_container_ptr container;
    tree_caching_data<element_object> caching;
};

template<class Context, class Function>
void
cached_content(Context ctx, id_interface const& id, Function&& fn)
{
    cached_content_data* data;
    if (get_data(ctx, &data))
        data->container.reset(new component_container);

    scoped_component_container container(ctx, &data->container);

    scoped_tree_cacher<element_object> cacher;

    bool content_traversal_required;
    if (is_refresh_event(ctx))
    {
        cacher.begin(
            get_object<tree_traversal_tag>(ctx),
            data->caching,
            id,
            container.is_dirty());
        content_traversal_required = cacher.content_traversal_required();
    }
    else
    {
        content_traversal_required = container.is_on_route();
    }

    ALIA_EVENT_DEPENDENT_IF(content_traversal_required)
    {
        fn(ctx);
    }
    ALIA_END
}

struct system
{
    std::function<void(dom::context)> controller;

    tree_node<element_object> root_node;

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
