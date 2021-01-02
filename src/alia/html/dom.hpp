#ifndef ALIA_HTML_DOM_HPP
#define ALIA_HTML_DOM_HPP

#include <asm-dom.hpp>

#include <alia.hpp>

#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <alia/html/context.hpp>

namespace alia { namespace html {

// This implements the interface required of alia object_tree objects.
struct element_object
{
    void
    relocate(
        element_object& parent, element_object* after, element_object* before);

    void
    remove();

    ~element_object();

    enum node_type
    {
        UNINITIALIZED = 0,
        // a normal element; asmdom_id is the ID of the element itself
        NORMAL,
        // a root element that places its children before a placeholder element
        // in the DOM; asmdom_id is the ID of the placeholder
        PLACEHOLDER_ROOT,
        // a root element that places its children at the end of the document
        // body; asmdom_id is irrelevant
        MODAL_ROOT
    };
    node_type type = UNINITIALIZED;

    int asmdom_id = 0;
};

void
create_as_element(element_object& object, char const* type);

void
create_as_text_node(element_object& object, char const* value);

void
create_as_placeholder_root(
    element_object& object, emscripten::val placeholder);

void
create_as_placeholder_root(element_object& object, char const* placeholder_id);

void
create_as_modal_root(element_object& object);

namespace detail {

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
    std::function<bool(emscripten::val)> callback;
};

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type);

void
install_onpopstate_callback(
    std::function<void(emscripten::val)> const* function);

void
install_onhashchange_callback(
    std::function<void(emscripten::val)> const* function);

void
text_node(html::context ctx, readable<std::string> text);

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<std::string> value);

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<bool> value);

void
do_element_class_token(
    context ctx,
    element_object& object,
    bool initializing,
    readable<std::string> value);

void
do_element_class_token(
    context ctx, element_object& object, bool initializing, char const* value);

void
set_element_property(
    element_object& object, char const* name, emscripten::val const& value);

void
clear_element_property(element_object& object, char const* name);

template<class Value>
void
do_element_property(
    context ctx, element_object& object, char const* name, Value const& value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            stored_id,
            value,
            [&](auto const& new_value) {
                set_element_property(object, name, emscripten::val(new_value));
            },
            [&]() { clear_element_property(object, name); });
    });
}

} // namespace detail

template<class Text>
void
text_node(html::context ctx, Text text)
{
    detail::text_node(ctx, as_text(ctx, signalize(text)));
}

template<class Context>
struct element_handle
{
    element_handle(Context ctx, char const* type) : ctx_(ctx)
    {
        initializing_ = get_cached_data(ctx, &node_);
        if (initializing_)
            create_as_element(node_->object, type);
        if (is_refresh_event(ctx))
            refresh_tree_node(get<tree_traversal_tag>(ctx), *node_);
    }

    template<class Value>
    element_handle&
    attr(char const* name, Value value)
    {
        detail::do_element_attribute(
            ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class... Tokens>
    element_handle&
    class_(Tokens... tokens)
    {
        (detail::do_element_class_token(
             ctx_, node_->object, initializing_, tokens),
         ...);
        return *this;
    }

    template<class Value>
    element_handle&
    prop(char const* name, Value value)
    {
        detail::do_element_property(
            ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Function>
    element_handle&
    callback(char const* event_type, Function&& fn)
    {
        auto& data = get_cached_data<detail::callback_data>(ctx_);
        if (initializing_)
        {
            detail::install_element_callback(
                ctx_, node_->object, data, event_type);
        }
        on_targeted_event<detail::dom_event>(
            ctx_, &data.identity, [&](auto ctx, auto& e) {
                std::forward<Function>(fn)(e.event);
            });
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
        invoke_component_function(ctx_, std::forward<Function>(fn));
        return *this;
    }

    template<class Text>
    element_handle&
    text(Text text)
    {
        return children([&](auto ctx) { text_node(ctx, text); });
    }

    template<class Callback>
    element_handle&
    on_init(Callback&& callback)
    {
        if (initializing_)
            std::forward<Callback>(callback)(*this);
        return *this;
    }

    int
    asmdom_id()
    {
        return node_->object.asmdom_id;
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
    begin(context ctx, char const* type);

    void
    end()
    {
        tree_scoping_.end();
    }

    template<class Value>
    scoped_element&
    attr(char const* name, Value value)
    {
        detail::do_element_attribute(
            *ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Value>
    scoped_element&
    prop(char const* name, Value value)
    {
        detail::do_element_property(
            *ctx_, node_->object, name, signalize(value));
        return *this;
    }

    template<class Function>
    scoped_element&
    callback(char const* event_type, Function&& fn)
    {
        auto ctx = *ctx_;
        auto& data = get_cached_data<detail::callback_data>(ctx);
        if (initializing_)
        {
            detail::install_element_callback(
                ctx, node_->object, data, event_type);
        }
        on_targeted_event<detail::dom_event>(
            ctx, &data.identity, [&](auto ctx, auto& e) {
                std::forward<Function>(fn)(e.event);
            });
        return *this;
    }

    template<class Callback>
    scoped_element&
    on_init(Callback callback)
    {
        if (initializing_)
            callback(*this);
        return *this;
    }

    int
    asmdom_id()
    {
        return node_->object.asmdom_id;
    }

 private:
    optional_context<context> ctx_;
    tree_node<element_object>* node_;
    scoped_tree_node<element_object> tree_scoping_;
    bool initializing_;
};

template<class Content>
void
invoke_tree(context ctx, tree_node<element_object>& root, Content&& content)
{
    scoped_tree_root<element_object> scoped_root;
    if (is_refresh_event(ctx))
        scoped_root.begin(get<html::tree_traversal_tag>(ctx), root);
    std::forward<Content>(content)();
}

}} // namespace alia::html

#endif
