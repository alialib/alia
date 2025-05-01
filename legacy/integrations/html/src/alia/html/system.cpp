#include "alia/core/context/interface.hpp"
#include <alia/html/system.hpp>

#include <alia/html/context.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/history.hpp>
#include <functional>

namespace alia { namespace html {

static void
refresh_for_emscripten(void* system)
{
    refresh_system(*reinterpret_cast<html::system*>(system));
}

struct timer_callback_data
{
    std::function<void()> callback;
};

static void
timer_callback(void* user_data)
{
    std::unique_ptr<timer_callback_data> data(
        reinterpret_cast<timer_callback_data*>(user_data));
    data->callback();
}

struct dom_external_interface : default_external_interface
{
    dom_external_interface(html::system& owner)
        : default_external_interface(owner)
    {
    }

    void
    schedule_animation_refresh()
    {
        emscripten_async_call(refresh_for_emscripten, &this->owner, -1);
    }

    void
    schedule_callback(std::function<void()> callback, millisecond_count time)
    {
        auto timeout_data = new timer_callback_data{std::move(callback)};
        emscripten_async_call(
            timer_callback, timeout_data, time - this->get_tick_count());
    }
};

void
system::invoke_controller(vanilla_context vanilla_ctx)
{
    tree_traversal<element_object> traversal;
    auto ctx = add_context_object<system_tag>(
        add_context_object<tree_traversal_tag>(
            vanilla_ctx, std::ref(traversal)),
        std::ref(*this));

    this->controller(ctx);
}

void
initialize(html::system& system, std::function<void(html::context)> controller)
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
    initialize_core_system<html::vanilla_context>(
        system, new dom_external_interface(system));
    system.controller = std::move(controller);

    // Update our DOM.
    refresh_system(system);
}

void
set_location_hash(html::system& sys, std::string new_hash)
{
    history().push_url(std::move(new_hash));
    update_location_hash(sys);
}

void
update_location_hash(html::system& sys)
{
    auto hash = emscripten::val::global("window")["location"]["hash"]
                    .as<std::string>();
    if (hash.empty() || hash[0] != '#')
    {
        hash = "#/";
        emscripten::val::global("window")["location"].call<void>(
            "replace", emscripten::val(hash));
    }
    sys.hash = hash;
}

void
enable_hash_monitoring(html::system& sys)
{
    // Do an initial query.
    update_location_hash(sys);
    refresh_system(sys);
    // Install monitors.
    auto onhashchange = [&sys](emscripten::val) {
        update_location_hash(sys);
        refresh_system(sys);
    };
    detail::install_window_callback(
        sys.hashchange, "hashchange", onhashchange);
}

}} // namespace alia::html
