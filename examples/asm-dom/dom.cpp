#include "dom.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/val.h>

namespace dom {

void
do_text_(dom::context ctx, readable<std::string> text)
{
    handle_event<refresh_event>(ctx, [text](auto ctx, auto& e) {
        get_component<context_info_tag>(ctx)->current_children->push_back(
            asmdom::h(
                "p", signal_is_readable(text) ? read_signal(text) : string()));
    });
}

void
do_heading_(
    dom::context ctx, readable<std::string> level, readable<std::string> text)
{
    handle_event<refresh_event>(ctx, [=](auto ctx, auto& e) {
        get_component<context_info_tag>(ctx)->current_children->push_back(
            asmdom::h(
                signal_is_readable(level) ? read_signal(level) : "p",
                signal_is_readable(text) ? read_signal(text) : string()));
    });
}

void
do_input_(dom::context ctx, bidirectional<string> value)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto id = get_node_id(ctx);
    auto routable_id = make_routable_node_id(ctx, id);

    if (signal_is_readable(value))
    {
        if (!data->external_id.matches(value.value_id()))
        {
            data->value = read_signal(value);
            data->external_id.capture(value.value_id());
        }
    }
    else
    {
        if (!data->external_id.matches(no_id))
        {
            data->value = string();
            data->external_id.capture(no_id);
        }
    }

    auto* system = get_component<system_tag>(ctx);

    handle_event<refresh_event>(ctx, [=](auto ctx, auto& e) {
        get_component<context_info_tag>(ctx)->current_children->push_back(
            asmdom::h(
                "input",
                asmdom::Data(
                    asmdom::Props{{"value", emscripten::val(data->value)}},
                    asmdom::Callbacks{
                        {"oninput", [=](emscripten::val e) {
                             value_update_event update;
                             update.value
                                 = e["target"]["value"].as<std::string>();
                             dispatch_targeted_event(
                                 *system, update, routable_id);
                             refresh_system(*system);
                             return true;
                         }}})));
    });
    handle_targeted_event<value_update_event>(ctx, id, [=](auto ctx, auto& e) {
        if (signal_is_writable(value))
        {
            write_signal(value, e.value);
        }
        data->value = e.value;
    });
}

void
do_number_input_(dom::context ctx, bidirectional<string> value)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto id = get_node_id(ctx);
    auto routable_id = make_routable_node_id(ctx, id);

    if (signal_is_readable(value))
    {
        if (!data->external_id.matches(value.value_id()))
        {
            data->value = read_signal(value);
            data->external_id.capture(value.value_id());
        }
    }
    else
    {
        if (!data->external_id.matches(no_id))
        {
            data->value = string();
            data->external_id.capture(no_id);
        }
    }

    auto* system = get_component<system_tag>(ctx);

    handle_event<refresh_event>(ctx, [=](auto ctx, auto& e) {
        get_component<context_info_tag>(ctx)->current_children->push_back(
            asmdom::h(
                "input",
                asmdom::Data(
                    asmdom::Attrs{{"type", "number"}},
                    asmdom::Props{{"value", emscripten::val(data->value)}},
                    asmdom::Callbacks{
                        {"oninput", [=](emscripten::val e) {
                             value_update_event update;
                             update.value
                                 = e["target"]["value"].as<std::string>();
                             dispatch_targeted_event(
                                 *system, update, routable_id);
                             refresh_system(*system);
                             return true;
                         }}})));
    });
    handle_targeted_event<value_update_event>(ctx, id, [=](auto ctx, auto& e) {
        if (signal_is_writable(value))
        {
            write_signal(value, e.value);
        }
        data->value = e.value;
    });
}

void
do_button_(dom::context ctx, readable<std::string> text, action<> on_click)
{
    auto id = get_node_id(ctx);
    auto routable_id = make_routable_node_id(ctx, id);
    auto* system = get_component<system_tag>(ctx);
    handle_event<refresh_event>(ctx, [=](auto ctx, auto& e) {
        if (signal_is_readable(text))
        {
            get_component<context_info_tag>(ctx)->current_children->push_back(
                asmdom::h(
                    "button",
                    asmdom::Data(
                        asmdom::Attrs{{"class", "btn"},
                                      {"disabled",
                                       on_click.is_ready() ? "false" : "true"}},
                        asmdom::Callbacks{{"onclick",
                                           [=](emscripten::val) {
                                               click_event click;
                                               dispatch_targeted_event(
                                                   *system, click, routable_id);
                                               refresh_system(*system);
                                               return true;
                                           }}}),
                    read_signal(text)));
        }
    });
    handle_targeted_event<click_event>(ctx, id, [=](auto ctx, auto& e) {
        if (action_is_ready(on_click))
        {
            perform_action(on_click);
        }
    });
}

void
do_colored_box(dom::context ctx, readable<rgb8> color)
{
    handle_event<refresh_event>(ctx, [color](auto ctx, auto& e) {
        char style[64] = {'\0'};
        if (signal_is_readable(color))
        {
            rgb8 const& c = read_signal(color);
            sprintf(style, "background-color: #%02x%02x%02x", c.r, c.g, c.b);
        }
        get_component<context_info_tag>(ctx)->current_children->push_back(
            asmdom::h(
                "div",
                asmdom::Data(asmdom::Attrs{{"class", "colored-box"},
                                           {"style", style}})));
    });
}

static void
handle_refresh_event(dom::context ctx, system& system)
{
    asmdom::Children children;
    get_component<context_info_tag>(ctx)->current_children = &children;

    system.controller(ctx);

    asmdom::VNode* root = asmdom::h(
        "div", asmdom::Data(asmdom::Attrs{{"class", "container"}}), children);

    asmdom::patch(system.current_view, root);
    system.current_view = root;
}

void
refresh_for_emscripten(void* system)
{
    refresh_system(*reinterpret_cast<alia::system*>(system));
}

void
dom_external_interface::request_animation_refresh()
{
    emscripten_async_call(refresh_for_emscripten, this->system, -1);
}

void
system::operator()(alia::context vanilla_ctx)
{
    context_info* context_info;
    get_data(vanilla_ctx, &context_info);
    dom::context ctx
        = add_component<context_info_tag>(vanilla_ctx, context_info);

    if (is_refresh_event(ctx))
    {
        handle_refresh_event(ctx, *this);
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
        asmdom::init(config);
        asmdom_initialized = true;
    }

    // Hook up the dom::system to the alia::system.
    alia_system.external = &dom_system.external;
    dom_system.external.system = &alia_system;
    alia_system.controller = std::ref(dom_system);
    dom_system.controller = std::move(controller);

    // Replace the requested node in the DOM with our virtual DOM.
    emscripten::val document = emscripten::val::global("document");
    emscripten::val root
        = document.call<emscripten::val>("getElementById", dom_node_id);
    dom_system.current_view = asmdom::h("div", std::string(""));
    asmdom::patch(root, dom_system.current_view);

    // Update the virtual DOM.
    refresh_system(alia_system);
}

} // namespace dom
