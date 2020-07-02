#ifndef ALIA_SIGNALS_STATE_HPP
#define ALIA_SIGNALS_STATE_HPP

#include <alia/flow/components.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/signals/adaptors.hpp>

namespace alia {

// state_storage<Value> is designed to be stored persistently within the
// component tree to represent application state or other data that needs to be
// tracked similarly. It contains a 'version' number that counts changes and
// serves as a signal value ID, and it also takes care of mark the component
// tree as 'dirty' when it's updated.
template<class Value>
struct state_storage
{
    state_storage() : version_(0)
    {
    }

    explicit state_storage(Value value) : value_(std::move(value)), version_(1)
    {
    }

    bool
    is_initialized() const
    {
        return version_ != 0;
    }

    Value const&
    get() const
    {
        return value_;
    }

    unsigned
    version() const
    {
        return version_;
    }

    void
    set(Value value)
    {
        value_ = std::move(value);
        handle_change();
    }

    // If you REALLY need direct, non-const access to the underlying state,
    // you can use this. It returns a non-const reference to the value and
    // increments the version number (assuming you'll make some changes).
    //
    // Note that you should be careful to use this atomically. In other words,
    // call this to get a reference, do your update, and then discard the
    // reference before anyone else observes the state. If you hold onto the
    // reference and continue making changes while other alia code is accessing
    // it, they'll end up with outdated views of the state.
    //
    // Also note that if you call this on an uninitialized state, you're
    // expected to initialize it.
    //
    Value&
    nonconst_ref()
    {
        handle_change();
        return value_;
    }

    // This is even less safe. It's like above, but any changes you make will
    // NOT be marked in the component tree, so you should only use this if you
    // know it's safe to do so.
    Value&
    untracked_nonconst_ref()
    {
        ++version_;
        return value_;
    }

    // Update the container that the state is part of.
    void
    refresh_container(component_container_ptr const& container)
    {
        container_ = container;
    }

 private:
    void
    handle_change()
    {
        ++version_;
        mark_dirty_component(container_);
    }

    Value value_;
    // version_ is incremented for each change in the value of the state.
    // If this is 0, the state is considered uninitialized.
    unsigned version_;
    component_container_ptr container_;
};

template<class Value, class Direction>
struct state_signal : signal<state_signal<Value, Direction>, Value, Direction>
{
    explicit state_signal(state_storage<Value>* data) : data_(data)
    {
    }

    bool
    has_value() const
    {
        return data_->is_initialized();
    }

    Value const&
    read() const
    {
        return data_->get();
    }

    simple_id<unsigned> const&
    value_id() const
    {
        id_ = make_id(data_->version());
        return id_;
    }

    bool
    ready_to_write() const
    {
        return true;
    }

    void
    write(Value value) const
    {
        data_->set(std::move(value));
    }

 private:
    state_storage<Value>* data_;
    mutable simple_id<unsigned> id_;
};

template<class Value>
state_signal<Value, duplex_signal>
make_state_signal(state_storage<Value>& data)
{
    return state_signal<Value, duplex_signal>(&data);
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

    state_storage<typename decltype(initial_value_signal)::value_type>* state;
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
