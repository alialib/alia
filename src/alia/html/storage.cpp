#include <alia/html/storage.hpp>

#include <iostream>

#include <alia/html/dom.hpp>

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

storage_signal::storage_signal(storage_signal_data* data) : data_(data)
{
}

bool
storage_signal::has_value() const
{
    return data_->value.has_value();
}

std::string const&
storage_signal::read() const
{
    return data_->value.get();
}

simple_id<unsigned> const&
storage_signal::value_id() const
{
    id_ = make_id(data_->value.version());
    return id_;
}

bool
storage_signal::ready_to_write() const
{
    return true;
}

void
storage_signal::write(std::string value) const
{
    storage_object(data_->storage).set_item(data_->key, value);
    data_->value.set(std::move(value));
}

std::string
storage_signal::move_out() const
{
    std::string moved = std::move(data_->value.untracked_nonconst_ref());
    return moved;
}

std::string&
storage_signal::destructive_ref() const
{
    return data_->value.untracked_nonconst_ref();
}

void
storage_signal::clear() const
{
    data_->value.clear();
    storage_object(data_->storage).remove_item(data_->key);
}

storage_signal
get_storage_state(html::context ctx, char const* storage_name, char const* key)
{
    storage_signal_data* data;
    if (get_cached_data(ctx, &data))
    {
        // Record the storage name and key.
        data->storage = storage_name;
        data->key = key;

        // Query the initial value.
        storage_object storage(storage_name);
        if (storage.has_item(key))
            data->value.untracked_nonconst_ref() = storage.get_item(key);

        // Install a handler for the HTML window's 'storage' event to monitor
        // changes in the underlying value of the signal.
        if (!strcmp(storage_name, "localStorage"))
        {
            alia::system* sys = &get<alia::system_tag>(ctx);
            detail::install_window_callback(
                data->on_storage_event, "storage", [=](emscripten::val event) {
                    // The event will fire for any storage activity related to
                    // our domain, so we have to check that the key matches.
                    if (event["key"].as<std::string>() == data->key)
                    {
                        std::cout << "storage event!" << std::endl;
                        data->value.set(event["newValue"].as<std::string>());
                        refresh_system(*sys);
                    }
                });
        }
    }

    refresh_handler(ctx, [&](auto ctx) {
        data->value.refresh_container(get_active_component_container(ctx));
    });

    return storage_signal(data);
}

storage_signal
get_session_state(html::context ctx, char const* key)
{
    return get_storage_state(ctx, "sessionStorage", key);
}

storage_signal
get_local_state(html::context ctx, char const* key)
{
    return get_storage_state(ctx, "localStorage", key);
}
}} // namespace alia::html
