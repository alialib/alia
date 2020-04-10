#include "dom.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/val.h>

namespace dom {

struct element_data
{
    asmdom::VNode* vnode = nullptr;
    captured_id key;
};

template<class CreateElement>
void
add_element(
    dom::context ctx,
    element_data& data,
    id_interface const& key,
    CreateElement create_element)
{
    on_refresh(ctx, [&](auto ctx) {
        // Theoretically, we should be able to reuse the vnode if the key hasn't
        // changed. However, asm-dom's patching semantics mean that sometimes
        // the vnodes get overwritten or destroyed without our knowledge, so
        // this isn't as simple as it seems.
        // if (!data.key.matches(key))
        {
            data.vnode = create_element();
            // data.key.capture(key);
        }
        get<context_info_tag>(ctx).current_children->push_back(data.vnode);
    });
}

void
do_text_(dom::context ctx, readable<std::string> text)
{
    add_element(
        ctx, get_cached_data<element_data>(ctx), text.value_id(), [=]() {
            return asmdom::h(
                "p", signal_has_value(text) ? read_signal(text) : string());
        });
}

void
do_heading_(
    dom::context ctx, readable<std::string> level, readable<std::string> text)
{
    add_element(
        ctx,
        get_cached_data<element_data>(ctx),
        combine_ids(ref(level.value_id()), ref(text.value_id())),
        [=]() {
            return asmdom::h(
                signal_has_value(level) ? read_signal(level) : "p",
                signal_has_value(text) ? read_signal(text) : string());
        });
}

struct input_data : node_identity
{
    captured_id external_id;
    string value;
    bool invalid = false;
    element_data element;
    unsigned version = 0;
};

struct value_update_event : targeted_event
{
    string value;
};

void
do_input_(dom::context ctx, duplex<string> value)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto id = data;
    auto routable_id = make_routable_node_id(ctx, id);

    refresh_signal_shadow(
        data->external_id,
        value,
        [&](string new_value) {
            data->value = std::move(new_value);
            data->invalid = false;
            ++data->version;
        },
        [&]() {
            data->value.clear();
            data->invalid = false;
            ++data->version;
        });

    auto* system = &ctx.get<system_tag>();

    add_element(ctx, data->element, make_id(data->version), [&]() {
        asmdom::Attrs attrs;
        if (data->invalid)
            attrs["class"] = "invalid-input";
        return asmdom::h(
            "input",
            asmdom::Data(
                attrs,
                asmdom::Props{{"value", emscripten::val(data->value)}},
                asmdom::Callbacks{
                    {"oninput", [=](emscripten::val e) {
                         value_update_event update;
                         update.value = e["target"]["value"].as<std::string>();
                         dispatch_targeted_event(*system, update, routable_id);
                         return true;
                     }}}));
    });
    on_targeted_event<value_update_event>(ctx, id, [=](auto ctx, auto& e) {
        if (signal_ready_to_write(value))
        {
            try
            {
                write_signal(value, e.value);
            }
            catch (validation_error&)
            {
                data->invalid = true;
            }
        }
        data->value = e.value;
        ++data->version;
    });
}

struct click_event : targeted_event
{
};

void
do_button_(dom::context ctx, readable<std::string> text, action<> on_click)
{
    auto id = get_node_id(ctx);
    auto routable_id = make_routable_node_id(ctx, id);
    auto* system = &ctx.get<system_tag>();

    element_data* element;
    get_cached_data(ctx, &element);

    if (signal_has_value(text))
    {
        add_element(
            ctx,
            get_cached_data<element_data>(ctx),
            combine_ids(ref(text.value_id()), make_id(on_click.is_ready())),
            [=]() {
                return asmdom::h(
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
                                               return true;
                                           }}}),
                    read_signal(text));
            });
    }

    on_targeted_event<click_event>(
        ctx, id, [=](auto ctx, auto& e) { perform_action(on_click); });
}

void
do_colored_box(dom::context ctx, readable<rgb8> color)
{
    add_element(
        ctx, get_cached_data<element_data>(ctx), color.value_id(), [=]() {
            char style[64] = {'\0'};
            if (signal_has_value(color))
            {
                rgb8 const& c = read_signal(color);
                sprintf(
                    style, "background-color: #%02x%02x%02x", c.r, c.g, c.b);
            }
            return asmdom::h(
                "div",
                asmdom::Data(
                    asmdom::Attrs{{"class", "colored-box"}, {"style", style}}));
        });
}

void
do_hr(dom::context ctx)
{
    add_element(ctx, get_cached_data<element_data>(ctx), unit_id, [=]() {
        return asmdom::h("hr");
    });
}

struct div_data
{
    element_data element;
    keyed_data<std::string> class_name;
    asmdom::Children children_;
};

void
scoped_div::begin(dom::context ctx, readable<std::string> class_name)
{
    ctx_ = ctx;

    get_cached_data(ctx, &data_);

    on_refresh(ctx, [&](auto ctx) {
        auto& dom_context = get<context_info_tag>(ctx);
        parent_children_list_ = dom_context.current_children;
        dom_context.current_children = &data_->children_;
        data_->children_.clear();

        refresh_keyed_data(data_->class_name, class_name.value_id());
        if (!is_valid(data_->class_name) && class_name.has_value())
            set(data_->class_name, class_name.read());
    });

    routing_.begin(ctx);
}

void
scoped_div::end()
{
    if (ctx_)
    {
        dom::context& ctx = *ctx_;

        routing_.end();

        auto& dom_context = ctx.get<context_info_tag>();
        dom_context.current_children = parent_children_list_;

        if (is_refresh_event(ctx))
        {
            asmdom::Attrs attrs;
            if (is_valid(data_->class_name))
                attrs["class"] = get(data_->class_name);
            parent_children_list_->push_back(
                asmdom::h("div", asmdom::Data(attrs), data_->children_));
        }

        ctx_.reset();
    }
}

static void
handle_refresh_event(dom::context ctx, system& system)
{
    asmdom::Children children;
    ctx.get<context_info_tag>().current_children = &children;

    system.controller(ctx);

    asmdom::VNode* root = asmdom::h(
        "div", asmdom::Data(asmdom::Attrs{{"class", "container"}}), children);

    asmdom::patch(system.current_view, root);
    system.current_view = root;
}

static void
refresh_for_emscripten(void* system)
{
    refresh_system(*reinterpret_cast<alia::system*>(system));
}

struct timer_callback_data
{
    alia::system* system;
    routable_node_id component;
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
    schedule_timer_event(routable_node_id component, millisecond_count time)
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
    context_info context_info;
    dom::context ctx = vanilla_ctx.add<context_info_tag>(context_info);

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
        config.clearMemory = true;
        asmdom::init(config);
        asmdom_initialized = true;
    }

    // Hook up the dom::system to the alia::system.
    initialize_system(
        alia_system,
        std::ref(dom_system),
        new dom_external_interface(alia_system));
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
