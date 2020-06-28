#include "dom.hpp"

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <chrono>

namespace dom {

void
callback_proxy(std::uintptr_t callback, emscripten::val event)
{
    (*reinterpret_cast<std::function<void(emscripten::val)>*>(callback))(event);
};

EMSCRIPTEN_BINDINGS(callback_proxy)
{
    emscripten::function(
        "callback_proxy", &callback_proxy, emscripten::allow_raw_pointers());
};

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type)
{
    auto external_id = externalize(&data.identity);
    auto* system = &get<system_tag>(ctx);
    data.callback = [=](emscripten::val v) {
        dom_event event(v);
        dispatch_targeted_event(*system, event, external_id);
    };
    EM_ASM_(
        {
            const element = Module.nodes[$0];
            const type = Module['UTF8ToString']($2);
            const handler = function(e)
            {
                const start = window.performance.now();
                Module.callback_proxy($1, e);
                const end = window.performance.now();
                console.log(
                    "total event time: " + (((end - start) * 1000) | 0)
                    + " µs");
            };
            element.addEventListener(type, handler);
            // Add the handler to asm-dom's event list so that it knows to clear
            // it out before recycling this DOM node.
            if (!element.hasOwnProperty('asmDomEvents'))
                element['asmDomEvents'] = {};
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
text_node_(dom::context ctx, readable<string> text)
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
    readable<string> value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_shadow(
            stored_id,
            value,
            [&](string const& new_value) {
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
                    { Module.removeAttribute($0, Module['UTF8ToString']($1)); },
                    object.js_id,
                    name);
            });
    });
}

void
do_element_attribute(
    context ctx, element_object& object, char const* name, readable<bool> value)
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
            const element = Module.nodes[$0];
            if (!element.hasOwnProperty('asmDomRaws'))
                element['asmDomRaws'] = [];
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
            const asmDomRaws = node['asmDomRaws'];
            const index = asmDomRaws.indexOf(Module['UTF8ToString']($1));
            if (index > -1)
            {
                asmDomRaws.splice(index, 1);
            }
        },
        object.js_id,
        name);
}

struct input_data
{
    captured_id value_id;
    string value;
    signal_validation_data validation;
    unsigned version = 0;
};

void
input_(dom::context ctx, duplex<string> value_)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto value = enforce_validity(ctx, value_, data->validation);

    on_refresh(ctx, [&](auto ctx) {
        if (!value.is_invalidated())
        {
            refresh_signal_shadow(
                data->value_id,
                value,
                [&](string new_value) {
                    data->value = std::move(new_value);
                    ++data->version;
                },
                [&]() {
                    data->value.clear();
                    ++data->version;
                });
        }
    });

    element(ctx, "input")
        .attr(
            "class",
            conditional(
                value.is_invalidated(), "invalid-input", "form-control"))
        .prop("value", data->value)
        .callback("input", [=](emscripten::val& e) {
            auto new_value = e["target"]["value"].as<std::string>();
            write_signal(value, new_value);
            data->value = new_value;
            ++data->version;
        });
}

void
button_(dom::context ctx, readable<std::string> text, action<> on_click)
{
    element(ctx, "button")
        .attr("class", "btn")
        .attr("disabled", !on_click.is_ready())
        .callback("click", [&](auto& e) { perform_action(on_click); })
        .text(text);
}

void
checkbox_(dom::context ctx, duplex<bool> value, readable<std::string> label)
{
    bool determinate = value.has_value();
    bool checked = determinate && value.read();
    bool disabled = !value.ready_to_write();

    element(ctx, "div")
        .attr("class", "custom-control custom-checkbox")
        .children([&](auto ctx) {
            element(ctx, "input")
                .attr("type", "checkbox")
                .attr("class", "custom-control-input")
                .attr("disabled", disabled)
                .attr("id", "custom-check-1")
                .prop("indeterminate", !determinate)
                .prop("checked", checked)
                .callback("change", [&](emscripten::val e) {
                    write_signal(value, e["target"]["checked"].as<bool>());
                });
            element(ctx, "label")
                .attr("class", "custom-control-label")
                .attr("for", "custom-check-1")
                .text(label);
        });
}

void
link_(dom::context ctx, readable<std::string> text, action<> on_click)
{
    element(ctx, "li").children([&](auto ctx) {
        element(ctx, "a")
            .attr("href", "javascript: void(0);")
            .attr("disabled", on_click.is_ready() ? "false" : "true")
            .children([&](auto ctx) { text_node(ctx, text); })
            .callback(
                "click", [&](emscripten::val) { perform_action(on_click); });
    });
}

void
colored_box(dom::context ctx, readable<rgb8> color)
{
    element(ctx, "div")
        .attr("class", "colored-box")
        .attr(
            "style",
            printf(
                ctx,
                "background-color: #%02x%02x%02x",
                alia_field(color, r),
                alia_field(color, g),
                alia_field(color, b)));
}

static void
refresh_for_emscripten(void* system)
{
    refresh_system(*reinterpret_cast<alia::system*>(system));
}

struct timer_callback_data
{
    alia::system* system;
    external_component_id component;
    millisecond_count trigger_time;
};

static void
timer_callback(void* user_data)
{
    std::unique_ptr<timer_callback_data> data(
        reinterpret_cast<timer_callback_data*>(user_data));
    timer_event event;
    event.trigger_time = data->trigger_time;
    dispatch_targeted_event(*data->system, event, data->component);
}

struct dom_external_interface : default_external_interface
{
    dom_external_interface(alia::system& owner)
        : default_external_interface(owner)
    {
    }

    void
    schedule_animation_refresh()
    {
        emscripten_async_call(refresh_for_emscripten, &this->owner, -1);
    }

    void
    schedule_timer_event(
        external_component_id component, millisecond_count time)
    {
        auto timeout_data
            = new timer_callback_data{&this->owner, component, time};
        emscripten_async_call(
            timer_callback, timeout_data, time - this->get_tick_count());
    }
};

void
system::operator()(alia::context vanilla_ctx)
{
    tree_traversal<element_object> traversal;
    auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);

    if (is_refresh_event(ctx))
    {
        traverse_object_tree(
            traversal, this->root_node, [&]() { this->controller(ctx); });
    }
    else
    {
        this->controller(ctx);
    }
}

void
initialize(
    dom::system& dom_system,
    alia::system& alia_system,
    std::string const& dom_node_id,
    std::function<void(dom::context)> controller)
{
    // Initialize asm-dom (once).
    static bool asmdom_initialized = false;
    if (!asmdom_initialized)
    {
        asmdom::Config config = asmdom::Config();
        config.unsafePatch = true;
        config.clearMemory = true;
        asmdom::init(config);
        asmdom_initialized = true;
    }

    // Initialize the alia::system and hook it up to the dom::system.
    initialize_system(
        alia_system,
        std::ref(dom_system),
        new dom_external_interface(alia_system));
    dom_system.controller = std::move(controller);

    // Replace the requested node in the DOM with our virtual DOM.
    emscripten::val document = emscripten::val::global("document");
    emscripten::val placeholder
        = document.call<emscripten::val>("getElementById", dom_node_id);
    if (placeholder.isNull())
    {
        auto msg = dom_node_id + " not found in document";
        EM_ASM_({ console.error(Module['UTF8ToString']($0)); }, msg.c_str());
        throw exception(msg);
    }
    // For now, just create a div to hold all our content.
    emscripten::val root = document.call<emscripten::val>(
        "createElement", emscripten::val("div"));
    placeholder["parentNode"].call<emscripten::val>(
        "replaceChild", root, placeholder);
    dom_system.root_node.object.js_id
        = emscripten::val::global("window")["asmDomHelpers"]["domApi"]
              .call<int>("addNode", root);

    // Update the virtual DOM.
    refresh_system(alia_system);
}

} // namespace dom
