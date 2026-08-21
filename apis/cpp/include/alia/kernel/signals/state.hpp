#pragma once

#include <alia/abi/ui/context.h>
#include <alia/abi/ui/system/work.h>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/substrate.hpp>

#include <utility>

// This file implements component-local persistent state.

namespace alia {

// `state_storage<Value>` is designed to be stored persistently in the
// component tree to represent application state or other data that needs to be
// tracked similarly. It contains a 'version' number that counts changes and
// serves as a signal value ID, and it also takes care of marking the component
// tree as 'dirty' when it's updated.
template<class Value>
struct state_storage
{
    // TODO: Make these private?
    Value value{};
    uint32_t version = 0;

    bool
    is_initialized() const
    {
        return version != 0;
    }

    bool
    has_value() const
    {
        return (version & 1u) != 0;
    }

    void
    set(Value v, alia_context* ctx)
    {
        value = std::move(v);
        set_valid_flag();
        handle_tracked_change(ctx);
    }

    void
    clear(alia_context* ctx)
    {
        clear_valid_flag();
        handle_tracked_change(ctx);
    }

    Value&
    untracked_nonconst_ref()
    {
        set_valid_flag();
        inc_version();
        return value;
    }

    void
    untracked_clear()
    {
        clear_valid_flag();
        inc_version();
    }

 private:
    void
    set_valid_flag()
    {
        version |= 1u;
    }

    void
    clear_valid_flag()
    {
        version &= ~1u;
    }

    void
    inc_version()
    {
        version += 2u;
    }

    // TODO: Revisit the use of the context here.
    void
    handle_tracked_change(alia_context* ctx)
    {
        inc_version();
        if (ctx && ctx->system)
            alia_ui_mark_dirty(alia_ctx_system(ctx));
    }
};

template<class Value>
struct state_binding
    : signal<
          state_binding<Value>,
          Value,
          binding_caps<signal_movable, signal_clearable>,
          uint32_t>
{
    state_binding(state_storage<Value>* data, alia_context* ctx)
        : data_(data), ctx_(ctx)
    {
    }

    bool
    has_value() const override
    {
        return data_->has_value();
    }

    uint32_t
    value_id() const
    {
        return data_->version;
    }

    Value const&
    read() const override
    {
        return data_->value;
    }

    Value
    move_out() const override
    {
        return std::move(data_->untracked_nonconst_ref());
    }

    Value&
    destructive_ref() const override
    {
        return data_->untracked_nonconst_ref();
    }

    bool
    ready_to_write() const override
    {
        return true;
    }

    void
    write(Value value) const override
    {
        data_->set(std::move(value), ctx_);
    }

    void
    clear() const override
    {
        data_->clear(ctx_);
    }

 private:
    state_storage<Value>* data_;
    alia_context* ctx_;
};

template<class Value>
state_binding<Value>
make_state_binding(state_storage<Value>& data, alia_context* ctx)
{
    return state_binding<Value>(&data, ctx);
}

// `use_state(ctx, initial)` returns a binding to persistent substrate-backed
// state whose initial value is determined by the `initial` signal.
// The returned signal will not have a value until `initial` has one or one is
// explicitly written to it.
template<class Initial>
auto
use_state(alia_context* ctx, Initial&& initial)
{
    auto initial_signal = signalize(std::forward<Initial>(initial));
    using Value = typename decltype(initial_signal)::value_type;

    auto storage = use_object<state_storage<Value>>(ctx);
    if (storage.is_init() && signal_has_value(initial_signal))
    {
        storage->untracked_nonconst_ref() = forward_signal(initial_signal);
    }

    return make_state_binding(*storage, ctx);
}

// TODO: Sort out transient vs persistent state APIs.

} // namespace alia
