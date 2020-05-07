#ifndef ALIA_FLOW_COMPONENTS_HPP
#define ALIA_FLOW_COMPONENTS_HPP

#include <alia/context/interface.hpp>
#include <alia/signals/core.hpp>

namespace alia {

struct component_container;

typedef std::shared_ptr<component_container> component_container_ptr;

struct component_container
{
    component_container_ptr parent;
    // The component is dirty and needs to be refreshed immediately.
    bool dirty = false;
    // The component is animating and would like to be refreshed soon.
    bool animating = false;
};

void
mark_dirty_component(component_container_ptr const& container);

void
mark_dirty_component(dataless_context ctx);

void
mark_animating_component(component_container_ptr const& container);

void
mark_animating_component(dataless_context ctx);

struct scoped_component_container
{
    scoped_component_container()
    {
    }
    scoped_component_container(context ctx)
    {
        begin(ctx);
    }
    scoped_component_container(context ctx, component_container_ptr* container)
    {
        begin(ctx, container);
    }
    ~scoped_component_container()
    {
        end();
    }

    void
    begin(dataless_context ctx, component_container_ptr* container);

    void
    begin(context ctx);

    void
    end();

    bool
    is_on_route() const
    {
        return is_on_route_;
    }

    bool
    is_dirty() const
    {
        return is_dirty_;
    }

    bool
    is_animating() const
    {
        return is_animating_;
    }

 private:
    optional_context<dataless_context> ctx_;
    component_container_ptr* container_;
    component_container_ptr* parent_;
    bool is_on_route_;
    bool is_dirty_;
    bool is_animating_;
};

// component_data<Value> is designed to be stored persistently within the
// component tree to represent application state or other data that needs to be
// tracked similarly. It contains a 'version' number that counts changes and
// serves as a signal value ID, and it also takes care of mark the component
// tree as 'dirty' when it's updated.
template<class Value>
struct component_data
{
    component_data() : version_(0)
    {
    }

    explicit component_data(Value value) : value_(std::move(value)), version_(1)
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
struct component_data_signal
    : signal<component_data_signal<Value, Direction>, Value, Direction>
{
    explicit component_data_signal(component_data<Value>* data) : data_(data)
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
    write(Value const& value) const
    {
        data_->set(value);
    }

 private:
    component_data<Value>* data_;
    mutable simple_id<unsigned> id_;
};

} // namespace alia

#endif
