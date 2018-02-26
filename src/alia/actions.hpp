#ifndef ALIA_ACTIONS_HPP
#define ALIA_ACTIONS_HPP

#include <alia/accessors.hpp>

// This file defines the alia action interface, some common implementations of it, and
// some utilities for working with it.
//
// An action is essentially a response to an event that happens within alia. When
// specifying a UI element that can generate events, the application supplies the action
// that should be performed when the corresponding event is generated. Using this style
// allows event handling to be written in a safer and more reactive manner.
//
// Actions are very similar to accessors in the way that they are used in an application.
// Like accessors, they're typically created directly at the call site as function
// arguments and are only valid for the life of the function call.

namespace alia {

// This is the interface required of actions.
struct action
{
    // Is this action ready to be performed?
    virtual bool is_ready() const = 0;

    // Perform this action.
    virtual void perform() const = 0;
};

// Perform an action.
void static inline
perform_action(action const& a)
{
    assert(a.is_ready());
    a.perform();
}

// action_ref is a reference to an action that implements the action interface itself.
struct action_ref : action
{
    action_ref(action const* ref = nullptr) : ref_(ref) {}

    bool is_ready() const
    {
        assert(ref_);
        return ref_->is_ready();
    }

    void perform() const
    {
        assert(ref_);
        ref_->perform();
    }

 private:
    action const* ref_;
};

// ref(action_ptr) wraps a pointer to an action so that it can be passed around as an
// action. The referenced action must remain valid for the life of the wrapper.
action_ref static inline
ref(action const* action_ptr)
{ return action_ref(action_ptr); }

// copyable_action_helper is a utility for allowing action wrappers to store copies of
// other actions if they are passed by concrete value and pointers if they're passed as
// references to the action base class.
template<class T>
struct copyable_action_helper
{
    typedef T result_type;
    static T const& apply(T const& x) { return x; }
};
template<class T>
struct copyable_action_helper<T const&>
{
    typedef T result_type;
    static T const& apply(T const& x) { return x; }
};
template<>
struct copyable_action_helper<action const&>
{
    typedef action_ref result_type;
    static result_type apply(action const& x)
    { return alia::ref(&x); }
};

// make_action_copyable(x) converts x to its copyable equivalent.
template<class Action>
typename copyable_action_helper<Action const&>::result_type
make_action_copyable(Action const& x)
{
    return copyable_action_helper<Action const&>::apply(x);
}

// combine_actions(first, second, ...) combines multiple actions into a single action.
// The returned action will perform all of its arguments in the order they're supplied.

template<class First, class Second>
struct action_pair : action
{
    action_pair() {}

    action_pair(First const& first, Second const& second)
      : first_(first), second_(second)
    {}

    bool is_ready() const
    {
        return first_.is_ready() && second_.is_ready();
    }

    void perform() const
    {
        first_.perform();
        second_.perform();
    }

 private:
    First first_;
    Second second_;
};

template<class First, class Second>
auto
combine_actions(First const& first, Second const& second)
{
    return
        action_pair<
            typename copyable_action_helper<First const&>::result_type,
            typename copyable_action_helper<Second const&>::result_type
          >(make_action_copyable(first),
            make_action_copyable(second));
}

template<class First, class Second, class ...Rest>
auto
combine_actions(First const& first, Second const& second, Rest const& ...rest)
{
    return combine_actions(combine_actions(first, second), rest...);
}

// make_setter(sink, source) creates an action that will set the value of :sink to the
// value held in :source. :sink and :source are both accessors. In order for the action
// to be considered ready, :sink must be settable and :source must be gettable.

template<class Sink, class Source>
struct setter_action : action
{
    setter_action(Sink const& sink, Source const& source)
      : sink_(sink), source_(source)
    {}

    bool is_ready() const
    {
        return source_.is_gettable() && sink_.is_settable();
    }

    void perform() const
    {
        sink_.set(source_.get());
    }

 private:
    Sink sink_;
    Source source_;
};

template<class Sink, class Source>
auto
make_setter(Sink const& sink, Source const& source)
{
    return
        setter_action<
            typename copyable_accessor_helper<Sink const&>::result_type,
            typename copyable_accessor_helper<Source const&>::result_type
          >(make_accessor_copyable(sink),
            make_accessor_copyable(source));
}

// make_toggle_action(flag), where :flag is an accessor to a boolean, creates an action
// that will toggle the value of :flag between true and false.
//
// Note that this could also be used with other value types as long as the ! operator
// provides a reasonable "toggle" function.
//
template<class Flag>
auto
make_toggle_action(Flag const& flag)
{
    return make_setter(flag, !flag);
}

// make_push_back_action(collection, item), where both :collection and :item are
// accessors, creates an action that will push the value of :item onto the back of
// :collection.

template<class Collection, class Item>
struct push_back_action : action
{
    push_back_action(Collection const& collection, Item const& item)
      : collection_(collection), item_(item)
    {}

    bool is_ready() const
    {
        return collection_.is_gettable() && collection_.is_settable() &&
            item_.is_gettable();
    }

    void perform() const
    {
        auto new_collection = collection_.get();
        new_collection.push_back(item_.get());
        collection_.set(new_collection);
    }

 private:
    Collection collection_;
    Item item_;
};

template<class Collection, class Item>
auto
make_push_back_action(Collection const& collection, Item const& item)
{
    return
        push_back_action<
            typename copyable_accessor_helper<Collection const&>::result_type,
            typename copyable_accessor_helper<Item const&>::result_type
          >(make_accessor_copyable(collection),
            make_accessor_copyable(item));
}

}

#endif
