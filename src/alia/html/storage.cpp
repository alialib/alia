#include "storage.hpp"

#include <iostream>

namespace alia { namespace html {

storage_object::storage_object(std::string const& name)
    : object_(emscripten::val::global(name.c_str()))
{
}

size_t
storage_object::length()
{
    return object_["length"].as<size_t>();
}

void
storage_object::set_item(std::string const& key, std::string const& value)
{
    object_.call<void>("setItem", key, value);
}

bool
storage_object::has_item(std::string const& key)
{
    return !object_.call<emscripten::val>("getItem", key).isNull();
}

std::string
storage_object::get_item(std::string const& key)
{
    return object_.call<std::string>("getItem", key);
}

void
storage_object::remove_item(std::string const& key)
{
    object_.call<void>("removeItem", key);
}

void
storage_object::clear()
{
    object_.call<void>("clear");
}

storage_object
local_storage()
{
    return storage_object("localStorage");
}

storage_object
session_storage()
{
    return storage_object("sessionStorage");
}

duplex<std::string>
get_storage_state(
    html::context ctx,
    std::string const& storage_name,
    std::string const& key,
    readable<std::string> default_value)
{
    return add_write_action(
        get_state(
            ctx,
            add_default(
                lambda_reader(
                    [=] { return storage_object(storage_name).has_item(key); },
                    [=] {
                        return storage_object(storage_name).get_item(key);
                    }),
                default_value)),
        callback([=](std::string new_value) {
            storage_object(storage_name).set_item(key, new_value);
        }));
}

auto
get_session_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value)
{
    return get_storage_state(ctx, "sessionStorage", key, default_value);
}

auto
get_local_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value)
{
    return get_storage_state(ctx, "localStorage", key, default_value);
}

}} // namespace alia::html
