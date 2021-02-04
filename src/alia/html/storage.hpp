#ifndef ALIA_HTML_STORAGE_HPP
#define ALIA_HTML_STORAGE_HPP

#include <emscripten/val.h>

#include <alia/html/context.hpp>
#include <alia/html/dom.hpp>

namespace alia { namespace html {

// RAW ACCESS

struct storage_object
{
    explicit storage_object(std::string const& name);

    size_t
    length();

    void
    set_item(std::string const& key, std::string const& value);

    bool
    has_item(std::string const& key);

    std::string
    get_item(std::string const& key);

    void
    remove_item(std::string const& key);

    void
    clear();

 private:
    emscripten::val object_;
};

storage_object
local_storage();

storage_object
session_storage();

// SIGNALIZED ACCESS

struct storage_signal_data
{
    std::string storage;
    std::string key;
    state_storage<std::string> value;
    detail::window_callback on_storage_event;
};

struct storage_signal
    : signal<
          storage_signal,
          std::string,
          signal_capabilities<signal_copyable, signal_clearable>>
{
    explicit storage_signal(storage_signal_data* data);

    bool
    has_value() const override;

    std::string const&
    read() const override;

    simple_id<unsigned> const&
    value_id() const override;

    bool
    ready_to_write() const override;

    void
    write(std::string value) const override;

    std::string
    movable_value() const override;

    void
    clear() const override;

 private:
    storage_signal_data* data_;
    mutable simple_id<unsigned> id_;
};

storage_signal
get_storage_state(
    html::context ctx, char const* storage_name, char const* key);

storage_signal
get_session_state(html::context ctx, char const* key);

storage_signal
get_local_state(html::context ctx, char const* key);

}} // namespace alia::html

#endif
