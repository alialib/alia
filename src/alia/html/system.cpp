#include <alia/html/system.hpp>

#include <alia/html/dom.hpp>
#include <alia/html/history.hpp>

namespace alia { namespace html {

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
    auto ctx = extend_context<system_tag>(
        extend_context<tree_traversal_tag>(vanilla_ctx, traversal), *this);

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
    html::system& dom_system,
    alia::system& alia_system,
    std::string const& dom_node_id,
    std::function<void(html::context)> controller)
{
    emscripten::val document = emscripten::val::global("document");
    emscripten::val placeholder
        = document.call<emscripten::val>("getElementById", dom_node_id);
    if (placeholder.isNull())
    {
        auto msg = dom_node_id + " not found in document";
        EM_ASM_({ console.error(Module['UTF8ToString']($0)); }, msg.c_str());
        throw exception(msg);
    }
    initialize(dom_system, alia_system, placeholder, controller);
}

direct_const_signal<std::string>
get_location_hash(html::context ctx)
{
    return direct(const_cast<std::string const&>(get<system_tag>(ctx).hash));
}

void
update_location_hash(html::system& dom_system)
{
    auto hash = emscripten::val::global("window")["location"]["hash"]
                    .as<std::string>();
    if (hash.empty() || hash[0] != '#')
    {
        hash = "#/";
        history().push_url(hash);
    }
    dom_system.hash = hash;
}

void
initialize(
    html::system& dom_system,
    alia::system& alia_system,
    emscripten::val placeholder_dom_node,
    std::function<void(html::context)> controller)
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

    // Initialize the alia::system and hook it up to the html::system.
    initialize_system(
        alia_system,
        std::ref(dom_system),
        new dom_external_interface(alia_system));
    dom_system.controller = std::move(controller);

    // Replace the requested node in the DOM with our root DOM node.
    emscripten::val document = emscripten::val::global("document");
    // HACK to get the root element to fill the whole window.
    // root.call<void>(
    //     "setAttribute", emscripten::val("class"), emscripten::val("h-100"));
    create_as_placeholder_root(
        dom_system.root_node.object, placeholder_dom_node);

    // Query the hash and install an event handler to monitor it.
    update_location_hash(dom_system);
    dom_system.onhashchange = [&](emscripten::val) {
        update_location_hash(dom_system);
        refresh_system(alia_system);
    };
    detail::install_onhashchange_callback(&dom_system.onhashchange);
    detail::install_onpopstate_callback(&dom_system.onhashchange);

    // Update our DOM.
    refresh_system(alia_system);
}

}} // namespace alia::html