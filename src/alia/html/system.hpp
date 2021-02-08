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

    alia::system alia_system;

    void
    operator()(alia::context ctx);

    std::string hash;
    detail::window_callback hashchange, popstate;
};

// Initialize the HTML system with no root DOM element.
// You're required to root your elements yourself.
void
initialize(
    html::system& system, std::function<void(html::context)> controller);

// Get the core alia system object associated with the HTML system.
inline alia::system&
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
inline std::string const&
get_location_hash(html::system const& sys)
{
    return sys.hash;
}

// Set the current location hash associated with the HTML system.
void
set_location_hash(html::system& sys, std::string new_hash);

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
