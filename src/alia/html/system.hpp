#ifndef ALIA_HTML_SYSTEM_HPP
#define ALIA_HTML_SYSTEM_HPP

#include <functional>

#include <emscripten/val.h>

#include <alia/html/context.hpp>
#include <alia/html/dom.hpp>

namespace alia { namespace html {

struct system
{
    std::function<void(html::context)> controller;

    tree_node<element_object> placeholder_node;

    alia::system alia_system;

    void
    operator()(alia::context ctx);

    std::string hash;
    std::function<void(emscripten::val)> onhashchange;
};

// Initialize the HTML system so that it places its top-level DOM elements
// before the element whose ID is placeholder_node_id.
void
initialize(
    html::system& system,
    std::string const& placeholder_node_id,
    std::function<void(html::context)> controller);

// Initialize the HTML system so that it places its top-level DOM elements
// before placeholder_node.
void
initialize(
    html::system& system,
    emscripten::val placeholder_node,
    std::function<void(html::context)> controller);

// Get the core alia system object associated with the HTML system.
static inline alia::system&
get_alia_system(html::system& sys)
{
    return sys.alia_system;
}

// Get the current location hash associated with the HTML system.
//
// Note that this will only be up-to-date if enable_hash_monitoring() has been
// called on this system or if you're manually calling update_location_hash()
// when necessary.
//
std::string
get_location_hash(html::system& sys);

// Install an event handler to monitor the hash.
// This needs to be done to enable SPA-style routing.
void
enable_hash_monitoring(html::system& sys);

// Manually update the location hash.
// If no hash is currently present, navigate to '#/'.
void
update_location_hash(html::system& sys);

}} // namespace alia::html

#endif
