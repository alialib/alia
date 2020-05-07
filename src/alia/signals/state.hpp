#ifndef ALIA_SIGNALS_STATE_HPP
#define ALIA_SIGNALS_STATE_HPP

#include <alia/flow/components.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/signals/adaptors.hpp>

namespace alia {

template<class Value>
component_data_signal<Value, duplex_signal>
make_state_signal(component_data<Value>& data)
{
    return component_data_signal<Value, duplex_signal>(&data);
}

// get_state(ctx, initial_value) returns a signal carrying some persistent local
// state whose initial value is determined by the :initial_value signal. The
// returned signal will not have a value until :initial_value has one or one is
// explicitly written to the state signal.
template<class Context, class InitialValue>
auto
get_state(Context ctx, InitialValue const& initial_value)
{
    auto initial_value_signal = signalize(initial_value);

    component_data<typename decltype(initial_value_signal)::value_type>* state;
    get_data(ctx, &state);

    on_refresh(ctx, [&](auto ctx) {
        state->refresh_container(get_active_component_container(ctx));
        if (!state->is_initialized() && signal_has_value(initial_value_signal))
            state->untracked_nonconst_ref() = read_signal(initial_value_signal);
    });

    return make_state_signal(*state);
}

} // namespace alia

#endif
