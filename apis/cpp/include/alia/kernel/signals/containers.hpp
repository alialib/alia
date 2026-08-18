#pragma once

#include <alia/kernel/signals/application.hpp>
#include <alia/kernel/signals/core.hpp>

// This file defines utilities for treating a signal's value as a container.

namespace alia {

// `container_size_view(s)` yields a view carrying the size of `s`'s value, as
// determined by calling `.size()` on that value.
template<view_signal Container>
auto
container_size_view(Container cs)
{
    return lazy_apply([](auto const& x) { return x.size(); }, std::move(cs));
}

// `container_empty_view(s)` yields a view to a boolean that is true iff `s`'s
// value is empty. This calls `.empty()` on the value, so it works on most
// containers (including strings).
template<view_signal Container>
auto
container_empty_view(Container cs)
{
    return lazy_apply([](auto const& x) { return x.empty(); }, std::move(cs));
}

} // namespace alia
