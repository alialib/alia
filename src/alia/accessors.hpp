#ifndef ALIA_ACCESSORS_HPP
#define ALIA_ACCESSORS_HPP

#include <cassert>
#include <cmath>
#include <alia/common.hpp>
#include <alia/id.hpp>
#include <memory>

namespace alia {

// Accessors are the means by which UI elements access model state and values
// computed by the application for display in the UI.
// The goals of the accessor library are as follows.
// - Unify the interface to C-style data structures and OOP-style classes
//   (whose data members may be protected behind accessor functions).
// - Provide standard mechanisms for transforming the UI's view of model state
//   or applying constraints to its manipulations of that state.
// - Provide mechanisms for efficiently detecting changes in displayed values.
// - Ensure that the passing of values to the UI is as efficient and lazy as
//   possible.
//
// All reading of model state goes through the getter interface below, and all
// writing of model state goes through the accessor interface.
// Both are abstract base classes.
//
// Accessors are passed by const reference into UI functions.
// They're typically created directly at the call site as function arguments
// and are only valid for the life of the function call.
// Accessor wrappers are templated and store copies of the actual wrapped
// accessor, which allows them to be easily composed at the call site, without
// requiring any memory allocation.
// UI functions are untemplated and lose the actual type of the accessor.
// One consequence of this is that a UI container cannot store its accessor
// for its entire scope and thus only has access to it within its begin
// function. If a container needs to set its accessor's value from within its
// scope, it can do so by reinvoking the UI context with a set_value_event
// that is processed by the container's begin function.

// getter defines the interface for retrieving values.
template<class T>
struct getter
{
    typedef T value_type;

    // If this returns false, the underlying state has no value, so get()
    // should not be called.
    virtual bool is_gettable() const = 0;

    // Get the value. The reference returned here is only guaranteed to be
    // valid as long as the getter itself is valid.
    virtual T const& get() const = 0;

    // In some cases, the entity accessing the value may need to store a local
    // copy of it. If done through get(), this would require copying the value
    // itself, but that may be expensive if the value is large. Thus, accessors
    // can also provide a version which returns a shared_ptr to the value, so
    // the accessing entity can share ownership of it. The pointed-to value
    // must be guaranteed to remain constant.
    virtual alia__shared_ptr<T const> get_ptr() const
    { return alia__shared_ptr<T const>(new T(get())); }

    // A getter must supply an ID which uniquely identifies its value.
    // The ID is required to be valid if is_gettable() returns true.
    // (It may be valid even if is_gettable() returns false, which would mean
    // that the getter can identify its value but doesn't know it yet.)
    // The ID reference is only valid as long as the getter itself is valid.
    virtual id_interface const& id() const = 0;
};

// get(x) asserts that x is gettable and gets its value.
template<class T>
T const& get(getter<T> const& x)
{
    assert(x.is_gettable());
    return x.get();
}

// The full accessor interface extends the getter interface with methods for
// setting the value of the underlying model state.
template<class T>
struct accessor : getter<T>
{
    // If is_settable() returns false, the accessor is currently read-only and
    // any UI controls associated with it should disallow user input.
    // (And thus, set() should not be called.)
    virtual bool is_settable() const = 0;
    virtual void set(T const& value) const = 0;
};

// set(a, value) sets a to value iff a is settable.
template<class T>
void set(accessor<T> const& a, T const& value)
{
    if (a.is_settable())
        a.set(value);
}

// accessor_value_type<Accessor>::type yields the value type of an accessor.
template<class Accessor>
struct accessor_value_type
{
    typedef typename Accessor::value_type type;
};
template<class Accessor>
struct accessor_value_type<Accessor const>
  : accessor_value_type<Accessor>
{};
template<class Accessor>
struct accessor_value_type<Accessor&>
  : accessor_value_type<Accessor>
{};
template<class Accessor>
struct accessor_value_type<Accessor const&>
  : accessor_value_type<Accessor>
{};

// regular_accessor is a partial implementation of the accessor interface for
// cases where the ID of the accessor is simply the value itself.
template<class T>
struct regular_accessor : accessor<T>
{
    id_interface const& id() const
    {
        id_ = make_id_by_reference(this->get());
        return id_;
    }
 private:
    mutable value_id_by_reference<T> id_;
};

// inout(&x) creates an accessor for direct access to a non-const variable x.
template<class T>
struct inout_accessor : regular_accessor<T>
{
    inout_accessor(T* v) : v_(v) {}
    bool is_gettable() const { return true; }
    T const& get() const { return *v_; }
    bool is_settable() const { return true; }
    void set(T const& value) const { *v_ = value; }
 private:
    T* v_;
};
template<class T>
inout_accessor<T>
inout(T* value)
{
    return inout_accessor<T>(value);
}

// in(x) creates a read-only accessor for the value of x.
// A copy of x is stored within the accessor.
template<class T>
struct input_accessor : regular_accessor<T>
{
    input_accessor(T const& v) : v_(v) {}
    bool is_gettable() const { return true; }
    T const& get() const { return v_; }
    bool is_settable() const { return false; }
    void set(T const& value) const {}
 private:
    T v_;
};
template<class T>
input_accessor<T>
in(T const& value)
{
    return input_accessor<T>(value);
}

// in_ptr(&x) creates a read-only accessor for the value of x.
// x is passed by pointer and must stay valid for the life of the accessor.
template<class T>
struct input_pointer_accessor : regular_accessor<T>
{
    input_pointer_accessor(T const* v) : v_(v) {}
    bool is_gettable() const { return true; }
    T const& get() const { return *v_; }
    bool is_settable() const { return false; }
    void set(T const& value) const {}
 private:
    T const* v_;
};
template<class T>
input_pointer_accessor<T>
in_ptr(T const* value)
{
    return input_pointer_accessor<T>(value);
}

// A state_proxy object is used when direct access is not possible and you
// don't want to write a custom accessor for the state. It's used as follows.
// * Create a temporary state_proxy object, initialized with a copy of the
// model state. (If uninitialized, it will report having no value.)
// * Make accessors for it as necessary using make_accessor(s), where s is the
// state_proxy object.
// * Call s.was_set() to check if it was set, and if it was, retrieve its value
// with s.get() and write it back to the model state.
template<class T>
struct state_proxy
{
    state_proxy(T const& value)
      : initialized_(true)
      , value_(value)
      , was_set_(false)
    {}
    state_proxy()
      : initialized_(false)
      , was_set_(false)
    {}
    bool is_initialized() const { return initialized_; }
    void initialize(T const& value)
    {
        initialized_ = true;
        value_ = value;
    }
    bool was_set() const { return was_set_; }
    T const& get() const { return value_; }
 private:
    template<class U>
    friend struct state_proxy_accessor;

    void set(T const& value)
    {
        value_ = value;
        was_set_ = true;
    }

    bool initialized_;
    T value_;
    bool was_set_;
};
template<class T>
struct state_proxy_accessor : regular_accessor<T>
{
    state_proxy_accessor(state_proxy<T>* s) : s_(s) {}
    bool is_gettable() const { return s_->is_initialized(); }
    T const& get() const { return s_->get(); }
    bool is_settable() const { return true; }
    void set(T const& value) const { s_->set(value); }
 private:
    state_proxy<T>* s_;
};
template<class T>
state_proxy_accessor<T>
make_accessor(state_proxy<T>& state)
{
    return state_proxy_accessor<T>(&state);
}

// state is designed to be stored persistently as actual application state.
// Accessors for it will track changes in it and report its ID based on that.
template<class T>
struct state
{
    state() {}
    state(T const& value) {}
    T const& get() const { return value_; }
    value_id_by_reference<local_id> id() const
    { return get_id(identity_); }
    void set(T const& value)
    {
        inc_version(identity_);
        value_ = value;
    }
 private:
    T value_;
    local_identity identity_;
};
template<class T>
struct state_accessor : accessor<T>
{
    state_accessor() {}
    state_accessor(state<T>* s) : s_(s) {}
    bool is_gettable() const { return true; }
    T const& get() const { return s_->get(); }
    id_interface const& id() const
    {
        id_ = s_->id();
        return id_;
    }
    bool is_settable() const { return true; }
    void set(T const& value) const
    { s_->set(value); }
 private:
    state<T>* s_;
    mutable value_id_by_reference<local_id> id_;
};
template<class T>
state_accessor<T>
make_accessor(state<T>& state)
{
    return state_accessor<T>(&state);
}

// ref(accessor) wraps a reference to an accessor so that it can be passed into
// another wrapper. The referenced accessor must remain valid for the life of
// the wrapper.
template<class T>
struct indirect_accessor : accessor<T>
{
    indirect_accessor(accessor<T> const& wrapped) : wrapped_(&wrapped) {}
    bool is_gettable() const { return wrapped_->is_gettable(); }
    T const& get() const { return wrapped_->get(); }
    alia__shared_ptr<T const> get_ptr() const
    { return wrapped_->get_ptr(); }
    id_interface const& id() const { return wrapped_->id(); }
    bool is_settable() const { return wrapped_->is_settable(); }
    void set(T const& value) const { wrapped_->set(value); }
 private:
    accessor<T> const* wrapped_;
};
template<class T>
indirect_accessor<T>
ref(accessor<T> const& accessor)
{ return indirect_accessor<T>(accessor); }

// lazy_getter is used to create getters that lazily generate their values.
// It provides storage for the computed value and ensures that it's only
// computed once.
template<class T>
struct lazy_getter
{
    lazy_getter()
      : already_generated_(false)
    {}
    template<class Generator>
    T const& get(Generator const& generator) const
    {
        if (!already_generated_)
        {
            value_ = generator.generate();
            already_generated_ = true;
        }
        return value_;
    }
 private:
    mutable bool already_generated_;
    mutable T value_;
};

// cast<T>(a) creates a new accessor to a's underlying value that interfaces
// with values of type T by applying static_casts.
template<class Wrapped, class To>
struct accessor_caster : regular_accessor<To>
{
    accessor_caster(Wrapped wrapped) : wrapped_(wrapped) {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    To const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(To const& value) const
    {
        return wrapped_.set(
            static_cast<typename accessor_value_type<Wrapped>::type>(value));
    }
 private:
    friend struct lazy_getter<To>;
    To generate() const { return static_cast<To>(wrapped_.get()); }
    Wrapped wrapped_;
    lazy_getter<To> lazy_getter_;
};
template<class To, class Wrapped>
accessor_caster<Wrapped,To>
accessor_cast(Wrapped accessor)
{ return accessor_caster<Wrapped,To>(accessor); }

// make_readonly(accessor) creates a copy of the given accessor with the write
// function disabled.
template<class Wrapped>
struct readonly_accessor_wrapper
  : accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    readonly_accessor_wrapper(Wrapped wrapped) : wrapped_(wrapped) {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    wrapped_value_type const& get() const { return wrapped_.get(); }
    alia__shared_ptr<wrapped_value_type const> get_ptr() const
    { return wrapped_.get_ptr(); }
    id_interface const& id() const { return wrapped_.id(); }
    bool is_settable() const { return false; }
    void set(wrapped_value_type const& value) const {}
 private:
    Wrapped wrapped_;
};
template<class Wrapped>
readonly_accessor_wrapper<Wrapped>
make_readonly(Wrapped accessor)
{ return readonly_accessor_wrapper<Wrapped>(accessor); }

// select_accessor(condition, t, f), where t and f are accessors, yields t if
// condition is true and f if it's false.
// Note that this is a normal function call, so, unlike an if statement or the
// ternary operator, both t and f are fully evaluated. However, they are only
// accessed if they're selected.
// t and f must have the same value type.
template<class T, class F>
struct accessor_mux : accessor<typename accessor_value_type<T>::type>
{
    accessor_mux(bool condition, T t, F f)
      : condition_(condition), t_(t), f_(f)
    {}
    bool is_gettable() const
    { return condition_ ? t_.is_gettable() : f_.is_gettable(); }
    typename accessor_value_type<T>::type const& get() const
    { return condition_ ? t_.get() : f_.get(); }
    alia__shared_ptr<typename accessor_value_type<T>::type const>
    get_ptr() const { return condition_ ? t_.get_ptr() : f_.get_ptr(); }
    id_interface const& id() const
    { return condition_ ? t_.id() : f_.id(); }
    bool is_settable() const
    { return condition_ ? t_.is_settable() : f_.is_settable(); }
    void set(typename accessor_value_type<T>::type const& value) const
    {
        if (condition_)
            t_.set(value);
        else
            f_.set(value);
    }
 private:
    bool condition_;
    T t_;
    F f_;
};
template<class T, class F>
accessor_mux<T,F>
select_accessor(bool condition, T t, F f)
{ return accessor_mux<T,F>(condition, t, f); }

// scale(a, factor) creates a new accessor that presents a scaled view of a,
// where a is an accessor to a numeric value.
template<class Wrapped>
struct scaling_accessor_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    scaling_accessor_wrapper(Wrapped wrapped, wrapped_value_type scale_factor)
      : wrapped_(wrapped), scale_factor_(scale_factor)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    wrapped_value_type const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(wrapped_value_type const& value) const
    { wrapped_.set(value / scale_factor_); }
 private:
    friend struct lazy_getter<wrapped_value_type>;
    wrapped_value_type generate() const
    { return wrapped_.get() * scale_factor_; }
    Wrapped wrapped_;
    wrapped_value_type scale_factor_;
    lazy_getter<wrapped_value_type> lazy_getter_;
};
template<class Wrapped>
scaling_accessor_wrapper<Wrapped>
scale(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type scale_factor)
{ return scaling_accessor_wrapper<Wrapped>(accessor, scale_factor); }

// offset(a, offset) presents an offset view of a, where a is an accessor to
// a numeric value.
template<class Wrapped>
struct offset_accessor_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    offset_accessor_wrapper(Wrapped wrapped,
        typename accessor_value_type<Wrapped>::type offset)
      : wrapped_(wrapped), offset_(offset)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    wrapped_value_type const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    { wrapped_.set(value - offset_); }
 private:
    friend struct lazy_getter<wrapped_value_type>;
    wrapped_value_type generate() const
    { return wrapped_.get() + offset_; }
    Wrapped wrapped_;
    wrapped_value_type offset_;
    lazy_getter<wrapped_value_type> lazy_getter_;
};
template<class Wrapped>
offset_accessor_wrapper<Wrapped>
offset(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type offset)
{ return offset_accessor_wrapper<Wrapped>(accessor, offset); }

// add_input_rounder(accessor, step) rounds input from the UI to the given
// accessor so that its always a multiple of step.
template<class Wrapped>
struct rounding_accessor_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    rounding_accessor_wrapper(Wrapped wrapped,
        typename accessor_value_type<Wrapped>::type step)
      : wrapped_(wrapped), step_(step)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    typename accessor_value_type<Wrapped>::type const& get() const
    { return wrapped_.get(); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    { wrapped_.set(std::floor(value / step_ +
        typename accessor_value_type<Wrapped>::type(0.5)) * step_); }
 private:
    Wrapped wrapped_;
    typename accessor_value_type<Wrapped>::type step_;
};
template<class Wrapped>
rounding_accessor_wrapper<Wrapped>
add_input_rounder(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type step)
{ return rounding_accessor_wrapper<Wrapped>(accessor, step); }

// add_input_clamp(accessor, min, max) clamps input from the UI to the
// accessor so that it's always between min and max (inclusive).
template<class Wrapped>
struct clamping_accessor_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    clamping_accessor_wrapper(Wrapped wrapped,
        typename accessor_value_type<Wrapped>::type min,
        typename accessor_value_type<Wrapped>::type max)
      : wrapped_(wrapped), min_(min), max_(max)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    typename accessor_value_type<Wrapped>::type const& get() const
    { return wrapped_.get(); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    { wrapped_.set(value < min_ ? min_ : (value > max_ ? max_ : value)); }
 private:
    Wrapped wrapped_;
    typename accessor_value_type<Wrapped>::type min_, max_;
};
template<class Wrapped>
clamping_accessor_wrapper<Wrapped>
add_input_clamp(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type min,
    typename accessor_value_type<Wrapped>::type max)
{ return clamping_accessor_wrapper<Wrapped>(accessor, min, max); }

// Given an accessor to a structure, select_field(accessor, field_ptr) returns
// an accessor to the specified field within the structure.
template<class StructureAccessor, class Field>
struct field_accessor : accessor<Field>
{
    typedef typename accessor_value_type<StructureAccessor>::type
        structure_type;
    typedef Field structure_type::*field_ptr;
    field_accessor(StructureAccessor structure, field_ptr field)
      : structure_(structure), field_(field)
    {}
    bool is_gettable() const { return structure_.is_gettable(); }
    Field const& get() const
    {
        structure_type const& structure = structure_.get();
        return structure.*field_;
    }
    id_interface const& id() const
    {
        // Apparently pointers-to-members aren't comparable for order, so
        // instead we use the address of the field if it were in a structure
        // that started at address 0.
        id_ = combine_ids(ref(structure_.id()),
            make_id(&(((structure_type*)0)->*field_)));
        return id_;
    }
    bool is_settable() const
    { return structure_.is_gettable() && structure_.is_settable(); }
    void set(Field const& x) const
    {
        structure_type s = structure_.get();
        s.*field_ = x;
        structure_.set(s);
    }
 private:
    StructureAccessor structure_;
    field_ptr field_;
    mutable id_pair<id_ref,value_id<Field*> > id_;
};
template<class StructureAccessor, class Field>
field_accessor<StructureAccessor,Field>
select_field(
    StructureAccessor structure,
    Field accessor_value_type<StructureAccessor>::type::*field)
{
    return field_accessor<StructureAccessor,Field>(structure, field);
}

// const_text(x), where x is a string constant, creates a read-only accessor
// for accessing x as a string. Its ID is the pointer to the text.
struct const_text : accessor<string>
{
    const_text(char const* text)
      : text_(text), id_(text, ID_CONTEXT_APP_INSTANCE)
    {}
    id_interface const& id() const { return id_; }
    bool is_gettable() const { return true; }
    string const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const { return false; }
    void set(string const& value) const {}
 private:
    char const* text_;
    value_id<char const*> id_;
    friend struct lazy_getter<string>;
    string generate() const { return string(text_); }
    lazy_getter<string> lazy_getter_;
};

}

#endif
