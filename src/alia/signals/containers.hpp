#ifndef ALIA_SIGNALS_CONTAINERS_HPP
#define ALIA_SIGNALS_CONTAINERS_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/operators.hpp>

namespace alia {

// is_empty(s), where :s is a signal, yields a signal indicating whether or not
// :s carries an empty value. (This is determined by calling the `empty()`
// member function of the value, so it works on most containers (including
// strings).)
template<class ContainerSignal>
auto
is_empty(ContainerSignal cs)
{
    return lazy_apply([](auto const& x) { return x.empty(); }, cs);
}

// hide_if_empty(s), where :s is a signal, yields a wrapper for :s that will
// claim to have no value if that value would be empty.
template<class ContainerSignal>
auto
hide_if_empty(ContainerSignal cs)
{
    return mask_reads(cs, !is_empty(cs));
}

} // namespace alia

#endif
