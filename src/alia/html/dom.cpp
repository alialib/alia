#include <alia/html/dom.hpp>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <chrono>

namespace alia { namespace html {

void
create_as_element(element_object& object, char const* type)
{
    assert(object.asmdom_id == 0);
#ifdef ALIA_HTML_LOGGING
    std::cout << "asmdom::direct::createElement: " << type << std::endl;
#endif
    object.asmdom_id = asmdom::direct::createElement(type);
#ifdef ALIA_HTML_LOGGING
    std::cout << "-> " << object.asmdom_id << std::endl;
#endif
    object.type = element_object::NORMAL;
}

void
create_as_text(element_object& object, char const* value)
{
    assert(object.asmdom_id == 0);
#ifdef ALIA_HTML_LOGGING
    std::cout << "asmdom::direct::createTextNode: " << value << std::endl;
#endif
    object.asmdom_id = asmdom::direct::createTextNode(value);
#ifdef ALIA_HTML_LOGGING
    std::cout << "-> " << object.asmdom_id << std::endl;
#endif
    object.type = element_object::NORMAL;
}

void
create_as_existing(element_object& object, emscripten::val node)
{
    assert(object.asmdom_id == 0);
#ifdef ALIA_HTML_LOGGING
    std::cout << "create_as_existing" << std::endl;
#endif
    object.asmdom_id = asmdom::direct::toElement(node);
#ifdef ALIA_HTML_LOGGING
    std::cout << "-> " << object.asmdom_id << std::endl;
#endif
    object.type = element_object::NORMAL;
}

void
create_as_body(element_object& object)
{
    assert(object.asmdom_id == 0);
#ifdef ALIA_HTML_LOGGING
    std::cout << "create_as_body" << std::endl;
#endif
    object.asmdom_id = asmdom::direct::toElement(
        emscripten::val::global("document")["body"]);
#ifdef ALIA_HTML_LOGGING
    std::cout << "-> " << object.asmdom_id << std::endl;
#endif
    object.type = element_object::BODY;
}

void
create_as_placeholder_root(element_object& object, emscripten::val placeholder)
{
    assert(object.asmdom_id == 0);
    object.type = element_object::PLACEHOLDER_ROOT;
    object.asmdom_id = asmdom::direct::toElement(placeholder);
    asmdom::direct::toElement(placeholder["parentNode"]);
}

void
create_as_placeholder_root(element_object& object, char const* placeholder_id)
{
    emscripten::val document = emscripten::val::global("document");
    emscripten::val placeholder = document.call<emscripten::val>(
        "getElementById", emscripten::val(placeholder_id));
    // Strip out the ID to creating duplicate IDs through template reuse.
    // It's not longer needed once we've find it.
    placeholder.call<void>("removeAttribute", emscripten::val("id"));
    create_as_placeholder_root(object, placeholder);
}

void
create_as_modal_root(element_object& object)
{
    object.type = element_object::MODAL_ROOT;
}

void
element_object::relocate(
    element_object& new_parent, element_object* after, element_object* before)
{
    assert(this->type == element_object::NORMAL);
    assert(this->asmdom_id != 0);
    assert(new_parent.type != element_object::UNINITIALIZED);
    switch (new_parent.type)
    {
        case element_object::NORMAL:
        case element_object::BODY:
            assert(new_parent.asmdom_id != 0);
#ifdef ALIA_HTML_LOGGING
            std::cout << "asmdom::direct::insertBefore: "
                      << new_parent.asmdom_id << ", " << this->asmdom_id
                      << ", " << (before ? before->asmdom_id : 0) << std::endl;
#endif
            asmdom::direct::insertBefore(
                new_parent.asmdom_id,
                this->asmdom_id,
                before ? before->asmdom_id : 0);
            break;
        case element_object::PLACEHOLDER_ROOT:
            assert(new_parent.asmdom_id != 0);
            asmdom::direct::insertBefore(
                asmdom::direct::parentNode(new_parent.asmdom_id),
                this->asmdom_id,
                before ? before->asmdom_id : new_parent.asmdom_id);
            break;
        case element_object::MODAL_ROOT: {
            emscripten::val document = emscripten::val::global("document");
            document["body"].call<emscripten::val>(
                "appendChild", asmdom::direct::getElement(this->asmdom_id));
            break;
        }
        case element_object::UNINITIALIZED:
            // Suppress warnings.
            break;
    }
}

void
element_object::remove()
{
    assert(this->asmdom_id != 0);
#ifdef ALIA_HTML_LOGGING
    std::cout << "asmdom::direct::remove: " << this->asmdom_id << std::endl;
#endif
    asmdom::direct::remove(this->asmdom_id);
}

element_object::~element_object()
{
    this->destroy();
}

void
element_object::destroy()
{
    if (this->asmdom_id != 0)
    {
        // Using asmdom::direct::deleteElement invokes the node recycler, which
        // seems to cause problems when external JS code messes around with our
        // elements. Instead, we just delete it from the node table.
        EM_ASM({ delete Module['nodes'][$0]; }, this->asmdom_id);
        this->asmdom_id = 0;
    }
    this->type = element_object::UNINITIALIZED;
}

namespace detail {

void
callback_proxy(std::uintptr_t callback, emscripten::val event)
{
    (*reinterpret_cast<std::function<void(emscripten::val)>*>(callback))(
        event);
};

EMSCRIPTEN_BINDINGS(callback_proxy)
{
    emscripten::function(
        "callback_proxy", &callback_proxy, emscripten::allow_raw_pointers());
};

void
install_onpopstate_callback(
    std::function<void(emscripten::val)> const* function)
{
    EM_ASM(
        {
            window.onpopstate = function(event)
            {
                Module.callback_proxy($0, event);
            };
        },
        reinterpret_cast<std::uintptr_t>(function));
}

void
install_onhashchange_callback(
    std::function<void(emscripten::val)> const* function)
{
    EM_ASM(
        {
            window.onhashchange = function()
            {
                Module.callback_proxy($0, null);
            };
        },
        reinterpret_cast<std::uintptr_t>(function));
}

window_callback::~window_callback()
{
    if (this->installed)
    {
        EM_ASM(
            {
                if ('aliaEventHandlers' in Module)
                {
                    var aliaEventHandlers = Module['aliaEventHandlers'];
                    window.removeEventListener(
                        Module['UTF8ToString']($0), aliaEventHandlers[$1]);
                    delete aliaEventHandlers[$1];
                }
            },
            this->event.c_str(),
            reinterpret_cast<std::uintptr_t>(&this->function));
    }
}

void
install_window_callback(
    window_callback& callback,
    char const* event,
    std::function<void(emscripten::val)> function)
{
    callback.event = event;
    callback.function = std::move(function);
    EM_ASM(
        {
            var event = Module['UTF8ToString']($0);
            var handler = function(e)
            {
                Module.callback_proxy($1, e);
            };

            if (!('aliaEventHandlers' in Module))
                Module['aliaEventHandlers'] = {};
            Module['aliaEventHandlers'][$1] = handler;

            window.addEventListener(event, handler);
        },
        event,
        reinterpret_cast<std::uintptr_t>(&callback.function));
    callback.installed = true;
}

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type)
{
#ifdef ALIA_HTML_LOGGING
    std::cout << "install callback: " << object.asmdom_id << ": " << event_type
              << std::endl;
#endif
    auto external_id = externalize(&data.identity);
    auto* system = &get<alia::system_tag>(ctx);
    auto callback = [=](emscripten::val v) {
        dom_event event(v);
#ifdef ALIA_HTML_LOGGING
        auto start = std::chrono::high_resolution_clock::now();
#endif
        dispatch_targeted_event(*system, event, external_id);
#ifdef ALIA_HTML_LOGGING
        auto elapsed_ms
            = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now() - start)
                  .count();
        std::cout << "event time: " << elapsed_ms << " µs" << std::endl;
#endif
        return true;
    };
    asmdom::direct::setCallback(object.asmdom_id, event_type, callback);
}

struct text_data
{
    tree_node<element_object> node;
    captured_id value_id;
};

void
text(html::context ctx, readable<std::string> text)
{
    text_data* data;
    if (get_cached_data(ctx, &data))
        create_as_text(data->node.object, "");
    if (is_refresh_event(ctx))
    {
        refresh_tree_node(get<tree_traversal_tag>(ctx), data->node);
        refresh_signal_view(
            data->value_id,
            text,
            [&](std::string const& new_value) {
#ifdef ALIA_HTML_LOGGING
                std::cout << "asmdom::direct::setNodeValue: "
                          << data->node.object.asmdom_id << ": " << new_value
                          << std::endl;
#endif
                asmdom::direct::setNodeValue(
                    data->node.object.asmdom_id, new_value.c_str());
            },
            [&]() {
#ifdef ALIA_HTML_LOGGING
                std::cout << "asmdom::direct::setNodeValue: "
                          << data->node.object.asmdom_id << ": (null)"
                          << std::endl;
#endif
                asmdom::direct::setNodeValue(data->node.object.asmdom_id, "");
            });
    }
}

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<std::string> value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            stored_id,
            value,
            [&](std::string const& new_value) {
                asmdom::direct::setAttribute(
                    object.asmdom_id, name, new_value.c_str());
            },
            [&]() {
                asmdom::direct::removeAttribute(object.asmdom_id, name);
            });
    });
}

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<bool> value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            stored_id,
            value,
            [&](bool new_value) {
                if (new_value)
                    asmdom::direct::setAttribute(object.asmdom_id, name, "");
                else
                    asmdom::direct::removeAttribute(object.asmdom_id, name);
            },
            [&]() {
                asmdom::direct::removeAttribute(object.asmdom_id, name);
            });
    });
}

struct element_class_token_data
{
    std::string existing_value;
    captured_id value_id;
};

void
clear_class_token(int asmdom_id, std::string const& token)
{
    EM_ASM(
        {
            var node = Module['nodes'][$0];
            node.classList.remove(Module['UTF8ToString']($1));
        },
        asmdom_id,
        token.c_str());
}

void
add_class_token(int asmdom_id, std::string const& token)
{
    EM_ASM(
        {
            var node = Module['nodes'][$0];
            node.classList.add(Module['UTF8ToString']($1));
        },
        asmdom_id,
        token.c_str());
}

void
do_element_class_token(
    context ctx, element_object& object, bool, readable<std::string> value)
{
    auto& data = get_cached_data<element_class_token_data>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            data.value_id,
            value,
            [&](std::string const& new_value) {
                if (!data.existing_value.empty())
                {
                    clear_class_token(object.asmdom_id, data.existing_value);
                }
                add_class_token(object.asmdom_id, new_value);
                data.existing_value = new_value;
            },
            [&]() {
                if (!data.existing_value.empty())
                {
                    clear_class_token(object.asmdom_id, data.existing_value);
                    data.existing_value.clear();
                }
            });
    });
}

void
do_element_class_token(
    context ctx, element_object& object, bool initializing, char const* value)
{
    on_refresh(ctx, [&](auto ctx) {
        if (initializing)
            add_class_token(object.asmdom_id, value);
    });
}

void
set_element_property(
    element_object& object, char const* name, emscripten::val const& value)
{
#ifdef ALIA_HTML_LOGGING
    std::cout << "asmdom::direct::setProperty: " << object.asmdom_id << "."
              << name << ": " << value.as<std::string>() << std::endl;
#endif
    asmdom::direct::setProperty(object.asmdom_id, name, value);
}

void
clear_element_property(element_object& object, char const* name)
{
#ifdef ALIA_HTML_LOGGING
    std::cout << "asmdom::direct::removeProperty: " << object.asmdom_id << "."
              << name << std::endl;
#endif
    asmdom::direct::removeProperty(object.asmdom_id, name);
}

} // namespace detail

element_handle
element(context ctx, char const* type)
{
    tree_node<element_object>* node;
    bool initializing = get_cached_data(ctx, &node);
    if (initializing)
        create_as_element(node->object, type);
    if (is_refresh_event(ctx))
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
    return element_handle(ctx, node, initializing);
}

body_handle
body(context ctx)
{
    tree_node<element_object>* node;
    bool initializing = get_cached_data(ctx, &node);
    if (initializing)
    {
        create_as_body(node->object);
        // Clear out existing children/attributes.
        EM_ASM(
            {
                var node = Module['nodes'][$0];
                node.innerHTML = "";
                while (node.attributes.length > 0)
                    node.removeAttribute(node.attributes[0].name);
            },
            node->object.asmdom_id);
    }
    return body_handle(ctx, node, initializing);
}

void
placeholder_root(
    html::context ctx,
    char const* placeholder_id,
    alia::function_view<void()> content)
{
    tree_node<element_object>* node;
    if (get_cached_data(ctx, &node))
        create_as_placeholder_root(node->object, placeholder_id);

    invoke_tree(ctx, *node, content);
}

void
modal_root(html::context ctx, alia::function_view<void()> content)
{
    tree_node<element_object>* node;
    if (get_cached_data(ctx, &node))
        create_as_modal_root(node->object);

    invoke_tree(ctx, *node, content);
}

html_fragment_handle
html_fragment(context ctx, readable<std::string> html)
{
    auto elm = element(ctx, "div");
    auto& captured_html_id = get_cached_data<captured_id>(ctx);
    bool just_loaded = false;
    refresh_signal_view(
        captured_html_id,
        html,
        [&](std::string const& new_html) {
            EM_ASM(
                {
                    var node = Module['nodes'][$0];
                    node.innerHTML = Module['UTF8ToString']($1);
                },
                elm.asmdom_id(),
                new_html.c_str());
            just_loaded = true;
        },
        [&] {});
    return html_fragment_handle{ctx, just_loaded};
}

}} // namespace alia::html
