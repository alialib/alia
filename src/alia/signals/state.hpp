#ifndef ALIA_SIGNALS_STATE_HPP
#define ALIA_SIGNALS_STATE_HPP

#include <alia/flow/data_graph.hpp>
#include <alia/signals/core.hpp>

namespace alia {

// state_holder<Value> is designed to be stored persistently as actual
// application state. Signals for it will track changes in it and report its ID
// based on that.
template<class Value>
struct state_holder
{
    state_holder() : version_(0)
    {
    }

    explicit state_holder(Value value) : value_(std::move(value)), version_(1)
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
        ++version_;
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
    nonconst_get()
    {
        ++version_;
        return value_;
    }

 private:
    Value value_;
    // version_ is incremented for each change in the value of the state.
    // If this is 0, the state is considered uninitialized.
    unsigned version_;
};

template<class Value>
struct state_signal : signal<state_signal<Value>, Value, bidirectional_signal>
{
    explicit state_signal(state_holder<Value>* s) : state_(s)
    {
    }

    bool
    is_readable() const
    {
        return state_->is_initialized();
    }

    Value const&
    read() const
    {
        return state_->get();
    }

    simple_id<unsigned> const&
    value_id() const
    {
        id_ = make_id(state_->version());
        return id_;
    }

    bool
    is_writable() const
    {
        return true;
    }

    void
    write(Value const& value) const
    {
        state_->set(value);
    }

 private:
    state_holder<Value>* state_;
    mutable simple_id<unsigned> id_;
};

template<class Value>
state_signal<Value>
make_state_signal(state_holder<Value>& state)
{
    return state_signal<Value>(&state);
}

// get_state(ctx, initial_value) returns a signal carrying some persistent local
// state whose initial value is determined by the :initial_value signal. The
// returned signal will not be readable until :initial_value is readable (or
// a value is explicitly written to the state signal).
template<class Context, class InitialValue>
auto
get_state(Context ctx, InitialValue const& initial_value)
{
    state_holder<typename InitialValue::value_type>* state;
    get_data(ctx, &state);

    if (!state->is_initialized() && signal_is_readable(initial_value))
        state->set(read_signal(initial_value));

    return make_state_signal(*state);
}

} // namespace alia

#endif
