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
    this->js_id = EM_ASM_INT(
        { return Module.createElement(Module['UTF8ToString']($0)); }, type);
}

void
element_object::create_as_text_node(char const* value)
{
    assert(this->js_id == 0);
    this->js_id = EM_ASM_INT(
        { return Module.createTextNode(Module['UTF8ToString']($0)); }, value);
}

void
element_object::relocate(
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
element_object::remove()
{
    assert(this->js_id != 0);
    EM_ASM_({ Module.removeChild($0); }, this->js_id);
    // In asm-dom, removing a child from its parent also destroys it, so we
    // have to mark it as uninitialized here.
    this->js_id = 0;
}

element_object::~element_object()
{
    if (this->js_id != 0)
    {
        this->remove();
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
    auto external_id = externalize(&data.identity);
    auto* system = &get<alia::system_tag>(ctx);
    data.callback = [=](emscripten::val v) {
        dom_event event(v);
        dispatch_targeted_event(*system, event, external_id);
    };
    EM_ASM_(
        {
            var element = Module.nodes[$0];
            var type = Module['UTF8ToString']($2);
            var handler = function(e)
            {
                var start = window.performance.now();
                Module.callback_proxy($1, e);
                var end = window.performance.now();
                console.log(
                    "total event time: " + (((end - start) * 1000) | 0)
                    + " µs");
            };
            element.addEventListener(type, handler);
            // Add the handler to asm-dom's event list so that it knows to
            // clear it out before recycling this DOM node.
            element['asmDomEvents'] = element['asmDomEvents'] || {};
            element['asmDomEvents'][type] = handler;
        },
        object.js_id,
        reinterpret_cast<std::uintptr_t>(&data.callback),
        event_type);
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
        refresh_signal_shadow(
            data->value_id,
            text,
            [&](std::string const& new_value) {
                EM_ASM_(
                    { Module.setNodeValue($0, Module['UTF8ToString']($1)); },
                    data->node.object.js_id,
                    new_value.c_str());
            },
            [&]() {
                EM_ASM_(
                    { Module.setNodeValue($0, ""); }, data->node.object.js_id);
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
        refresh_signal_shadow(
            stored_id,
            value,
            [&](std::string const& new_value) {
                EM_ASM_(
                    {
                        Module.setAttribute(
                            $0,
                            Module['UTF8ToString']($1),
                            Module['UTF8ToString']($2));
                    },
                    object.js_id,
                    name,
                    new_value.c_str());
            },
            [&]() {
                EM_ASM_(
                    {
                        Module.removeAttribute($0, Module['UTF8ToString']($1));
                    },
                    object.js_id,
                    name);
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
        refresh_signal_shadow(
            stored_id,
            value,
            [&](bool new_value) {
                if (new_value)
                {
                    EM_ASM_(
                        {
                            Module.setAttribute(
                                $0,
                                Module['UTF8ToString']($1),
                                Module['UTF8ToString']($2));
                        },
                        object.js_id,
                        name,
                        "");
                }
                else
                {
                    EM_ASM_(
                        {
                            Module.removeAttribute(
                                $0, Module['UTF8ToString']($1));
                        },
                        object.js_id,
                        name);
                }
            },
            [&]() {});
    });
}

void
set_element_property(
    element_object& object, char const* name, emscripten::val value)
{
    emscripten::val::global("window")["asmDomHelpers"]["nodes"][object.js_id]
        .set(name, value);

    // Add the property name to the element's 'asmDomRaws' list. asm-dom uses
    // this to track what it needs to clean up when recycling a DOM node.
    EM_ASM_(
        {
            var element = Module.nodes[$0];
            element['asmDomRaws'] = element['asmDomRaws'] || [];
            element['asmDomRaws'].push(Module['UTF8ToString']($1));
        },
        object.js_id,
        name);
}

void
clear_element_property(element_object& object, char const* name)
{
    EM_ASM_(
        {
            var node = Module.nodes[$0];
            delete node[$1];

            // Remove the property name from the element's 'asmDomRaws' list.
            var asmDomRaws = node['asmDomRaws'];
            var index = asmDomRaws.indexOf(Module['UTF8ToString']($1));
            if (index > -1)
            {
                asmDomRaws.splice(index, 1);
            }
        },
        object.js_id,
        name);
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
