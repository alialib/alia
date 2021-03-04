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

    void
    destroy();

    bool
    is_initialized() const
    {
        return type != UNINITIALIZED;
    }

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
        MODAL_ROOT,
        // the document body itself; asmdom_id is the ID of the body
        BODY
    };
    node_type type = UNINITIALIZED;

    int asmdom_id = 0;
};

void
create_as_element(element_object& object, char const* type);

void
create_as_text(element_object& object, char const* value);

void
create_as_existing(element_object& object, emscripten::val node);

void
create_as_body(element_object& object);

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

struct element_callback : noncopyable
{
    ~element_callback();

    component_identity identity;
    int asmdom_id = 0;
    std::string event;
    std::function<void(emscripten::val)> function;
};

void
install_element_callback(
    context ctx,
    element_object& object,
    element_callback& callback,
    char const* event_type);

struct window_callback : noncopyable
{
    ~window_callback();

    bool installed = false;
    std::string event;
    std::function<void(emscripten::val)> function;
};

void
install_window_callback(
    window_callback& callback,
    char const* event,
    std::function<void(emscripten::val)> function);

void
text(html::context ctx, readable<std::string> text);

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<std::string> value);

void
do_element_attribute(
    context ctx, element_object& object, char const* name, char const* value);

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
    refresh_handler(ctx, [&](auto ctx) {
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
text(html::context ctx, Text text)
{
    detail::text(ctx, as_text(ctx, signalize(text)));
}

struct element_handle_storage
{
    html::context ctx;

    html::context
    context()
    {
        return ctx;
    }

    tree_node<element_object>* node;
    bool initializing;
};

template<class Derived, class Storage>
struct element_handle_base
{
    // Specify an attribute value...
    // as a constant string value
    Derived&
    attr(char const* name, char const* value)
    {
        if (this->initializing())
            asmdom::direct::setAttribute(this->asmdom_id(), name, value);
        return static_cast<Derived&>(*this);
    }
    // dynamically, via a string signal
    Derived&
    attr(char const* name, readable<std::string> value)
    {
        detail::do_element_attribute(
            this->context(), this->node().object, name, value);
        return static_cast<Derived&>(*this);
    }
    // via a boolean signal
    Derived&
    attr(char const* name, readable<bool> value)
    {
        detail::do_element_attribute(
            this->context(), this->node().object, name, value);
        return static_cast<Derived&>(*this);
    }
    // via a raw boolean
    Derived&
    attr(char const* name, bool value)
    {
        detail::do_element_attribute(
            this->context(), this->node().object, name, alia::value(value));
        return static_cast<Derived&>(*this);
    }
    // as a constant with no value - This is equivalent to setting a boolean
    // attribute to a constant value of 'true'.
    Derived&
    attr(char const* name)
    {
        if (this->initializing())
            asmdom::direct::setAttribute(this->asmdom_id(), name, "");
        return static_cast<Derived&>(*this);
    }

    // Specify a CONSTANT value for the 'class' attribute.
    Derived&
    classes(char const* value)
    {
        return this->attr("class", value);
    }
    // Dynamically specify an individual token on the class attribute.
    template<class Token>
    Derived&
    class_(Token token)
    {
        detail::do_element_class_token(
            this->context(), this->node().object, this->initializing(), token);
        return static_cast<Derived&>(*this);
    }
    // Dynamically specify a conditional token on the class attribute.
    template<class Token, class Condition>
    Derived&
    class_(Token token, Condition condition)
    {
        detail::do_element_class_token(
            this->context(),
            this->node().object,
            this->initializing(),
            mask(std::move(token), std::move(condition)));
        return static_cast<Derived&>(*this);
    }

    // Specify the value of a property.
    template<class Value>
    Derived&
    prop(char const* name, Value value)
    {
        detail::do_element_property(
            this->context(), this->node().object, name, signalize(value));
        return static_cast<Derived&>(*this);
    }

    // Specify a handler for a DOM event.
    // The JS event object will be passed to the handler function as an
    // emscripten::val.
    template<class Function>
    Derived&
    handler(char const* event_type, Function&& fn)
    {
        auto& data
            = get_cached_data<detail::element_callback>(this->context());
        refresh_component_identity(this->context(), data.identity);
        if (this->initializing())
        {
            detail::install_element_callback(
                this->context(), this->node().object, data, event_type);
        }
        targeted_event_handler<detail::dom_event>(
            this->context(), &data.identity, [&](auto ctx, auto& e) {
                std::forward<Function>(fn)(e.event);
            });
        return static_cast<Derived&>(*this);
    }

    // Specify an action to perform in response to a DOM event.
    Derived&
    on(char const* event_type, action<> const& action)
    {
        return handler(
            event_type, [&](emscripten::val) { perform_action(action); });
    }

    // Specify a callback to call on element initialization.
    template<class Callback>
    Derived&
    init(Callback&& callback)
    {
        if (this->initializing())
            std::forward<Callback>(callback)(static_cast<Derived&>(*this));
        return static_cast<Derived&>(*this);
    }

    int
    asmdom_id()
    {
        return this->node().object.asmdom_id;
    }

    html::context
    context()
    {
        return storage_.context();
    }
    tree_node<element_object>&
    node()
    {
        return *storage_.node;
    }
    bool
    initializing()
    {
        return storage_.initializing;
    }

    Storage storage_;
};

template<class Derived>
struct regular_element_handle
    : element_handle_base<Derived, element_handle_storage>
{
    regular_element_handle(
        context ctx, tree_node<element_object>* node, bool initializing)
        : regular_element_handle::element_handle_base{
            element_handle_storage{ctx, node, initializing}}
    {
    }

    template<class Other>
    regular_element_handle(regular_element_handle<Other> const& other)
        : regular_element_handle::element_handle_base{other.storage_}
    {
    }

    template<class Function>
    Derived&
    content(Function&& fn)
    {
        auto& traversal = get<tree_traversal_tag>(this->context());
        scoped_tree_children<element_object> tree_scope;
        if (is_refresh_event(this->context()))
            tree_scope.begin(traversal, this->node());
        std::forward<Function>(fn)();
        return static_cast<Derived&>(*this);
    }

    template<class Text>
    Derived&
    text(Text text)
    {
        return content([&] { html::text(this->context(), text); });
    }
};

struct element_handle : regular_element_handle<element_handle>
{
    using regular_element_handle::regular_element_handle;
};

element_handle
element(context ctx, char const* type);

struct body_handle : element_handle_base<body_handle, element_handle_storage>
{
    body_handle(
        html::context ctx, tree_node<element_object>* node, bool initializing)
        : body_handle::element_handle_base{
            element_handle_storage{ctx, node, initializing}}
    {
    }

    template<class Function>
    body_handle&
    content(Function&& fn)
    {
        scoped_tree_root<element_object> tree_scope;
        if (is_refresh_event(this->context()))
        {
            tree_scope.begin(
                get<tree_traversal_tag>(this->context()), this->node());
        }
        std::forward<Function>(fn)();
        return *this;
    }
};

body_handle
body(context ctx);

struct scoped_element_handle_storage
{
    optional_context<html::context> ctx;

    html::context
    context()
    {
        return *ctx;
    }

    tree_node<element_object>* node;
    bool initializing;
};

template<class Derived>
struct scoped_element_base
    : element_handle_base<Derived, scoped_element_handle_storage>
{
    scoped_element_base()
    {
    }
    scoped_element_base(context ctx, char const* type)
    {
        begin(ctx, type);
    }
    ~scoped_element_base()
    {
        end();
    }

    Derived&
    begin(context ctx, char const* type)
    {
        this->storage_.ctx.reset(ctx);
        bool initializing = get_cached_data(ctx, &this->storage_.node);
        this->storage_.initializing = initializing;
        if (initializing)
            create_as_element(this->node().object, type);
        if (is_refresh_event(ctx))
            tree_scoping_.begin(get<tree_traversal_tag>(ctx), this->node());
        return static_cast<Derived&>(*this);
    }

    void
    end()
    {
        tree_scoping_.end();
    }

 private:
    scoped_tree_node<element_object> tree_scoping_;
};

struct scoped_element : scoped_element_base<scoped_element>
{
    using scoped_element_base::scoped_element_base;
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

void
placeholder_root(
    html::context ctx,
    char const* placeholder_id,
    alia::function_view<void()> content);

void
modal_root(html::context ctx, alia::function_view<void()> content);

struct override_data
{
    tree_node<element_object> node;

    ~override_data()
    {
        node.object.remove();
    }
};

struct html_fragment_handle
{
    context ctx;
    bool just_loaded;

    template<class Content>
    html_fragment_handle&
    override(char const* placeholder_id, Content&& content)
    {
        override_data* data;
        get_cached_data(ctx, &data);
        if (just_loaded)
        {
            // It's possible the node was already in use, so destroy it first.
            data->node.object.destroy();
            create_as_placeholder_root(data->node.object, placeholder_id);
        }

        ALIA_IF(data->node.object.is_initialized())
        {
            invoke_tree(ctx, data->node, content);
        }
        ALIA_END

        return *this;
    }
};

html_fragment_handle
html_fragment(context ctx, readable<std::string> html);

inline html_fragment_handle
html_fragment(context ctx, char const* html)
{
    return html_fragment(ctx, value(html));
}

void
focus(element_handle element);

bool
mouse_inside(context ctx, html::element_handle element);

}} // namespace alia::html

#endif
