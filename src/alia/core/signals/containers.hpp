#ifndef ALIA_CORE_SIGNALS_CONTAINERS_HPP
#define ALIA_CORE_SIGNALS_CONTAINERS_HPP

#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/application.hpp>
#include <alia/core/signals/operators.hpp>

namespace alia {

// size(s), where :s is a signal, yields a signal carrying the size of :s
// (as determined by calling s.size()).
template<class ContainerSignal>
auto
size(ContainerSignal cs)
{
    return lazy_apply(ALIA_MEM_FN(size), std::move(cs));
}

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
