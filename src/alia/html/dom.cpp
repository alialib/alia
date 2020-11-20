#include <alia/html/dom.hpp>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <chrono>

namespace alia {
namespace html {

void
element_object::create_as_element(char const* type)
{
    assert(this->js_id == 0);
    // std::cout << "asmdom::direct::createElement: " << type << std::endl;
    this->js_id = asmdom::direct::createElement(type);
    // std::cout << "-> " << this->js_id << std::endl;
}

void
element_object::create_as_text_node(char const* value)
{
    assert(this->js_id == 0);
    // std::cout << "asmdom::direct::createTextNode: " << value << std::endl;
    this->js_id = asmdom::direct::createTextNode(value);
    // std::cout << "-> " << this->js_id << std::endl;
}

void
element_object::relocate(
    element_object& parent, element_object* after, element_object* before)
{
    assert(this->js_id != 0);
    // std::cout << "asmdom::direct::insertBefore: " << parent.js_id << ", "
    //           << this->js_id << ", " << (before ? before->js_id : 0)
    //           << std::endl;
    asmdom::direct::insertBefore(
        parent.js_id, this->js_id, before ? before->js_id : 0);
}

void
element_object::remove()
{
    assert(this->js_id != 0);
    // std::cout << "asmdom::direct::remove: " << this->js_id << std::endl;
    asmdom::direct::remove(this->js_id);
}

element_object::~element_object()
{
    if (this->js_id != 0)
    {
        this->remove();
        // std::cout << "asmdom::direct::deleteElement: " << this->js_id
        //           << std::endl;
        asmdom::direct::deleteElement(this->js_id);
    }
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
    EM_ASM_(
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
    EM_ASM_(
        {
            window.onhashchange = function()
            {
                Module.callback_proxy($0, null);
            };
        },
        reinterpret_cast<std::uintptr_t>(function));
}

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type)
{
    // std::cout << "install callback" << std::endl;
    auto external_id = externalize(&data.identity);
    auto* system = &get<alia::system_tag>(ctx);
    // TODO: Probably don't need to store the callback in data anymore.
    data.callback = [=](emscripten::val v) {
        dom_event event(v);
        auto start = std::chrono::high_resolution_clock::now();
        dispatch_targeted_event(*system, event, external_id);
        auto elapsed_ms
            = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now() - start)
                  .count();
        std::cout << "event time: " << elapsed_ms << " µs" << std::endl;
        // TODO: Report whether or not it was actually handled?
        return true;
    };
    asmdom::direct::setCallback(object.js_id, event_type, data.callback);
    // std::cout << "worked!" << std::endl;
}

struct text_node_data
{
    tree_node<element_object> node;
    captured_id value_id;
};

void
text_node(html::context ctx, readable<std::string> text)
{
    text_node_data* data;
    if (get_cached_data(ctx, &data))
        data->node.object.create_as_text_node("");
    if (is_refresh_event(ctx))
    {
        refresh_tree_node(get<tree_traversal_tag>(ctx), data->node);
        refresh_signal_view(
            data->value_id,
            text,
            [&](std::string const& new_value) {
                // std::cout << "asmdom::direct::setNodeValue: "
                //           << data->node.object.js_id << ": " << new_value
                //           << std::endl;
                asmdom::direct::setNodeValue(
                    data->node.object.js_id, new_value.c_str());
            },
            [&]() {
                // std::cout << "asmdom::direct::setNodeValue: "
                //           << data->node.object.js_id << ": (null)"
                //           << std::endl;
                asmdom::direct::setNodeValue(data->node.object.js_id, "");
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
                    object.js_id, name, new_value.c_str());
            },
            [&]() { asmdom::direct::removeAttribute(object.js_id, name); });
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
                    asmdom::direct::setAttribute(object.js_id, name, "");
                else
                    asmdom::direct::removeAttribute(object.js_id, name);
            },
            [&]() { asmdom::direct::removeAttribute(object.js_id, name); });
    });
}

void
set_element_property(
    element_object& object, char const* name, emscripten::val const& value)
{
    // std::cout << "asmdom::direct::setProperty: " << object.js_id << "." <<
    // name
    //           << ": " << value.as<std::string>() << std::endl;
    asmdom::direct::setProperty(object.js_id, name, value);
}

void
clear_element_property(element_object& object, char const* name)
{
    // std::cout << "asmdom::direct::removeProperty: " << object.js_id << "."
    //           << name << std::endl;
    asmdom::direct::removeProperty(object.js_id, name);
}

} // namespace detail

scoped_element&
scoped_element::begin(context ctx, char const* type)
{
    ctx_.reset(ctx);
    initializing_ = get_cached_data(ctx, &node_);
    if (initializing_)
        node_->object.create_as_element(type);
    if (is_refresh_event(ctx))
        tree_scoping_.begin(get<tree_traversal_tag>(ctx), *node_);
    return *this;
}

} // namespace html
} // namespace alia
