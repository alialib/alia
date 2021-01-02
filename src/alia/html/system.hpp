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

    tree_node<element_object> root_node;

    alia::system alia_system;

    void
    operator()(alia::context ctx);

    std::string hash;
    std::function<void(emscripten::val)> onhashchange;
};

direct_const_signal<std::string>
get_location_hash(html::context ctx);

void
update_location_hash(html::system& dom_system);

void
initialize(
    html::system& dom_system,
    alia::system& alia_system,
    std::string const& dom_node_id,
    std::function<void(html::context)> controller);

void
initialize(
    html::system& dom_system,
    alia::system& alia_system,
    emscripten::val placeholder_dom_node,
    std::function<void(html::context)> controller);

}} // namespace alia::html

#endif
