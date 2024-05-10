#pragma once

#include <alia/core/flow/data_graph.hpp>
#include <alia/core/id.hpp>

// TODO: Sort this stuff into proper locations

namespace alia {

// KEYED_DATA

template<class Data>
struct keyed_data
{
    keyed_data()
    {
    }

    explicit operator bool() const
    {
        return key_.is_initialized();
    }

    void
    clear()
    {
        key_.clear();
    }

    Data const&
    operator*() const
    {
        return value_;
    }

    Data const*
    operator->() const
    {
        return &value_;
    }

    template<class GetValue>
    void
    refresh(id_interface const& key, GetValue&& get_value)
    {
        if (!key_.matches(key))
        {
            value_ = std::forward<GetValue>(get_value)();
            key_.capture(key);
        }
    }

 private:
    captured_id key_;
    Data value_;
};

} // namespace alia
