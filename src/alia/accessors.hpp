#ifndef ALIA_ACCESSORS_HPP
#define ALIA_ACCESSORS_HPP

#include <alia/common.hpp>
#include <alia/id.hpp>
#include <cassert>
#include <cmath>
#include <memory>
#include <type_traits>

namespace alia {

// Is x readable?
template<class T>
[[deprecated(
    "Use _is_readable or .is_readable() directly, depending on needs.")]] bool
is_readable(accessor<T> const& x) { return x.is_readable(); }

// read(x) asserts that x is readable and reads its value.
template<class T>
T const& read(accessor<T> const& x)
{
    assert(x.is_readable());
    return x.read();
}

// Is a writable?
template<class T>
[[deprecated(
    "Use _is_writable or .is_writable() directly, depending on needs.")]] bool
is_writable(accessor<T> const& a) { return a.is_writable(); }

// write(a, value) writes a to value iff a is writable.
template<class Dst, class Src>
void write(accessor<Dst> const& a, Src const& value)
{
    if (a.is_writable())
        a.write(value);
}

// accessor_value_type<Accessor>::type yields the value type of an accessor.
template<class Accessor>
struct accessor_value_type
{
    typedef typename Accessor::value_type type;
};
template<class Accessor>
struct accessor_value_type<Accessor const> : accessor_value_type<Accessor>
{
};
template<class Accessor>
struct accessor_value_type<Accessor&> : accessor_value_type<Accessor>
{
};
template<class Accessor>
struct accessor_value_type<Accessor const&> : accessor_value_type<Accessor>
{
};

// When an accessor is write to a value, it's allowed to throw a validation
// error if the value is not acceptable.
// It should include a message that's presentable to the user.
struct validation_error : exception
{
    validation_error(string const& message) : exception(message)
    {
    }
    ~validation_error() throw()
    {
    }
};

// empty_accessor is an accessor that contains no value.
template<class T>
struct empty_accessor : accessor<T>
{
    empty_accessor()
    {
    }
    id_interface const&
    id() const
    {
        return no_id;
    }
    bool
    is_readable() const
    {
        return false;
    }
    T const&
    read() const
    {
        assert(false);
        return *(T*) (0);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(T const& value) const
    {
    }
};

// regular_accessor is a partial implementation of the accessor interface for
// cases where the ID of the accessor is simply the value itself.
template<class T>
struct regular_accessor : accessor<T>
{
    id_interface const&
    id() const
    {
        if (this->is_readable())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return no_id;
    }

 private:
    mutable value_id_by_reference<T> id_;
};

// inout(&x) creates an accessor for direct access to a non-const variable x.
template<class T>
struct inout_accessor : regular_accessor<T>
{
    inout_accessor()
    {
    }
    explicit inout_accessor(T* v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    T const&
    read() const
    {
        return *v_;
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(T const& value) const
    {
        *v_ = value;
    }

 private:
    T* v_;
};
template<class T>
inout_accessor<T>
inout(T* value)
{
    return inout_accessor<T>(value);
}

// in_ptr(&x) creates a read-only accessor for the value of x.
// x is passed by pointer and must stay valid for the life of the accessor.
template<class T>
struct input_pointer_accessor : regular_accessor<T>
{
    input_pointer_accessor()
    {
    }
    input_pointer_accessor(T const* v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    T const&
    read() const
    {
        return *v_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(T const& value) const
    {
    }

 private:
    T const* v_;
};
template<class T>
input_pointer_accessor<T>
in_ptr(T const* value)
{
    return input_pointer_accessor<T>(value);
}

// optional_in(x) creates a read-only accessor to an optional value.
template<class T>
struct optional_input_accessor : accessor<T>
{
    optional_input_accessor()
    {
    }
    optional_input_accessor(optional<T> const& value) : value_(value)
    {
    }
    id_interface const&
    id() const
    {
        if (value_)
        {
            id_ = make_id_by_reference(alia::read(value_));
            return id_;
        }
        else
            return no_id;
    }
    bool
    is_readable() const
    {
        return value_ ? true : false;
    }
    T const&
    read() const
    {
        return alia::read(value_);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(T const& value) const
    {
    }

 private:
    mutable value_id_by_reference<T> id_;
    optional<T> value_;
};
template<class T>
optional_input_accessor<T>
optional_in(optional<T> const& value)
{
    return optional_input_accessor<T>(value);
}

// make_custom_readter(&x, id) gives you the most flexibility in creating an
// input accessor (short of implementing your own accessor type).
// x is the value, and id is a custom ID.
// The value is stored by pointer, so must remain valid, but the ID is copied.
template<class T, class Id>
struct custom_readter : accessor<T>
{
    custom_readter()
    {
    }
    custom_readter(T const* value, Id const& id) : value_(value), id_(id)
    {
    }
    id_interface const&
    id() const
    {
        return id_;
    }
    bool
    is_readable() const
    {
        return true;
    }
    T const&
    read() const
    {
        return *value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(T const& value) const
    {
    }

 private:
    Id id_;
    T const* value_;
};
template<class T, class Id>
custom_readter<T, Id>
make_custom_readter(T const* value, Id const& id)
{
    return custom_readter<T, Id>(value, id);
}

// Same as above, but the value is optional.
template<class T, class Id>
struct custom_optional_readter : accessor<T>
{
    custom_optional_readter()
    {
    }
    custom_optional_readter(optional<T> const* value, Id const& id)
        : value_(value), id_(id)
    {
    }
    id_interface const&
    id() const
    {
        return id_;
    }
    bool
    is_readable() const
    {
        return *value_ ? true : false;
    }
    T const&
    read() const
    {
        return alia::read(*value_);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(T const& value) const
    {
    }

 private:
    Id id_;
    optional<T> const* value_;
};
template<class T, class Id>
custom_optional_readter<T, Id>
make_custom_readter(optional<T> const* value, Id const& id)
{
    return custom_optional_readter<T, Id>(value, id);
}

// A state_proxy object is used when direct access is not possible and you
// don't want to write a custom accessor for the state. It's used as follows.
// * Create a temporary state_proxy object, initialized with a copy of the
// model state. (If uninitialized, it will report having no value.)
// * Make accessors for it as necessary using make_accessor(s), where s is the
// state_proxy object.
// * Call s.was_write() to check if it was write, and if it was, retrieve its
// value with s.read() and write it back to the model state.
template<class T>
struct state_proxy
{
    state_proxy(T const& value)
        : initialized_(true), value_(value), was_write_(false)
    {
    }
    state_proxy() : initialized_(false), was_write_(false)
    {
    }
    bool
    is_initialized() const
    {
        return initialized_;
    }
    void
    initialize(T const& value)
    {
        initialized_ = true;
        value_ = value;
    }
    bool
    was_write() const
    {
        return was_write_;
    }
    T const&
    read() const
    {
        return value_;
    }

 private:
    template<class U>
    friend struct state_proxy_accessor;

    void
    write(T const& value)
    {
        value_ = value;
        was_write_ = true;
    }

    bool initialized_;
    T value_;
    bool was_write_;
};
template<class T>
struct state_proxy_accessor : regular_accessor<T>
{
    state_proxy_accessor(state_proxy<T>* s) : s_(s)
    {
    }
    bool
    is_readable() const
    {
        return s_->is_initialized();
    }
    T const&
    read() const
    {
        return s_->read();
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(T const& value) const
    {
        s_->write(value);
    }

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
    state()
    {
    }
    state(T const& value) : value_(value)
    {
    }
    T const&
    read() const
    {
        return value_;
    }
    value_id_by_reference<local_id>
    id() const
    {
        return read_id(identity_);
    }
    void
    write(T const& value)
    {
        inc_version(identity_);
        value_ = value;
    }
    // If you REALLY need direct, non-const access to the underlying state,
    // you can use this. It returns a non-const reference to the value and
    // increments the version number of the associated ID.
    // Note that you should be careful to use this atomically. In other words,
    // call this to read a reference, do your update, and then discard the
    // reference before anyone else observes the state. If you hold onto the
    // reference and continue making changes while UI elements are accessing
    // it, you'll cause them to go out-of-sync.
    T&
    nonconst_read()
    {
        inc_version(identity_);
        return value_;
    }

 private:
    T value_;
    local_identity identity_;
};
template<class T>
struct state_accessor : accessor<T>
{
    state_accessor()
    {
    }
    state_accessor(state<T>* s) : s_(s)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    T const&
    read() const
    {
        return s_->read();
    }
    id_interface const&
    id() const
    {
        id_ = s_->id();
        return id_;
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(T const& value) const
    {
        s_->write(value);
    }

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
    indirect_accessor()
    {
    }
    indirect_accessor(accessor<T> const* wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_->is_readable();
    }
    T const&
    read() const
    {
        return wrapped_->read();
    }
    id_interface const&
    id() const
    {
        return wrapped_->id();
    }
    bool
    is_writable() const
    {
        return wrapped_->is_writable();
    }
    void
    write(T const& value) const
    {
        wrapped_->write(value);
    }
    accessor<T> const&
    wrapped() const
    {
        return *wrapped_;
    }

 private:
    accessor<T> const* wrapped_;
};
template<class T>
indirect_accessor<T>
ref(accessor<T> const& accessor)
{
    return indirect_accessor<T>(&accessor);
}

// copyable_accessor_helper is a utility for allowing accessor wrappers to
// store copies of other accessors if they are passed by concrete value and
// pointers if they're passed as references to accessor<T>.
template<class T>
struct copyable_accessor_helper
{
    typedef T result_type;
    static T const&
    apply(T const& x)
    {
        return x;
    }
};
template<class T>
struct copyable_accessor_helper<T const&>
{
    typedef T result_type;
    static T const&
    apply(T const& x)
    {
        return x;
    }
};
template<class Value>
struct copyable_accessor_helper<accessor<Value> const&>
{
    typedef indirect_accessor<Value> result_type;
    static result_type
    apply(accessor<Value> const& x)
    {
        return alia::ref(x);
    }
};

// make_accessor_copyable(x) converts x to its copyable equivalent.
template<class Accessor>
typename copyable_accessor_helper<Accessor const&>::result_type
make_accessor_copyable(Accessor const& x)
{
    return copyable_accessor_helper<Accessor const&>::apply(x);
}

// lazy_readter is used to create readters that lazily generate their values.
// It provides storage for the computed value and ensures that it's only
// computed once.
template<class T>
struct lazy_readter
{
    lazy_readter() : already_generated_(false)
    {
    }
    template<class Generator>
    T const&
    read(Generator const& generator) const
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

// accessor_cast<T>(a) creates a new accessor to a's underlying value that
// interfaces with values of type T by applying static_casts.
template<class Wrapped, class To>
struct accessor_caster : regular_accessor<To>
{
    accessor_caster()
    {
    }
    accessor_caster(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    To const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(To const& value) const
    {
        return wrapped_.write(
            static_cast<typename accessor_value_type<Wrapped>::type>(value));
    }

 private:
    friend struct lazy_readter<To>;
    To
    generate() const
    {
        return static_cast<To>(wrapped_.read());
    }
    Wrapped wrapped_;
    lazy_readter<To> lazy_readter_;
};
template<class To, class Wrapped>
accessor_caster<
    typename copyable_accessor_helper<Wrapped const&>::result_type,
    To>
accessor_cast(Wrapped const& wrapped)
{
    return accessor_caster<
        typename copyable_accessor_helper<Wrapped const&>::result_type,
        To>(make_accessor_copyable(wrapped));
}

// make_readonly(accessor) creates a copy of the given accessor with the write
// function disabled.
template<class Wrapped>
struct readonly_accessor_wrapper
    : accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    readonly_accessor_wrapper()
    {
    }
    readonly_accessor_wrapper(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    wrapped_value_type const&
    read() const
    {
        return wrapped_.read();
    }
    id_interface const&
    id() const
    {
        return wrapped_.id();
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(wrapped_value_type const& value) const
    {
    }

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
readonly_accessor_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type>
make_readonly(Wrapped const& wrapped)
{
    return readonly_accessor_wrapper<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped));
}

// select_accessor(condition, t, f), where condition, t and f are accessors,
// yields t if read(condition) is true and f otherwise.
// Note that this is a normal function call, so, unlike an if statement or the
// ternary operator, both t and f are fully evaluated. However, they are only
// accessed if they're selected.
// t and f must have the same value type, and condition's value type must be
// testable in a boolean context.
template<class Condition, class T, class F>
struct accessor_mux : accessor<typename accessor_value_type<T>::type>
{
    accessor_mux()
    {
    }
    accessor_mux(Condition condition, T t, F f)
        : condition_(condition), t_(t), f_(f)
    {
    }
    bool
    is_readable() const
    {
        return condition_.is_readable() && condition_.read() ? t_.is_readable()
                                                             : f_.is_readable();
    }
    typename accessor_value_type<T>::type const&
    read() const
    {
        return condition_.read() ? t_.read() : f_.read();
    }
    id_interface const&
    id() const
    {
        return condition_.read() ? t_.id() : f_.id();
    }
    bool
    is_writable() const
    {
        return condition_.is_readable() && condition_.read() ? t_.is_writable()
                                                             : f_.is_writable();
    }
    void
    write(typename accessor_value_type<T>::type const& value) const
    {
        if (condition_.read())
            t_.write(value);
        else
            f_.write(value);
    }

 private:
    Condition condition_;
    T t_;
    F f_;
};
template<class Condition, class T, class F>
accessor_mux<
    typename copyable_accessor_helper<Condition const&>::result_type,
    typename copyable_accessor_helper<T const&>::result_type,
    typename copyable_accessor_helper<F const&>::result_type>
select_accessor(Condition const& condition, T const& t, F const& f)
{
    return accessor_mux<
        typename copyable_accessor_helper<Condition const&>::result_type,
        typename copyable_accessor_helper<T const&>::result_type,
        typename copyable_accessor_helper<F const&>::result_type>(
        make_accessor_copyable(condition),
        make_accessor_copyable(t),
        make_accessor_copyable(f));
}

// scale(a, factor) creates a new accessor that presents a scaled view of a,
// where a is an accessor to a numeric value.
template<class Wrapped>
struct scaling_accessor_wrapper
    : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    scaling_accessor_wrapper()
    {
    }
    scaling_accessor_wrapper(Wrapped wrapped, wrapped_value_type scale_factor)
        : wrapped_(wrapped), scale_factor_(scale_factor)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    wrapped_value_type const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(wrapped_value_type const& value) const
    {
        wrapped_.write(value / scale_factor_);
    }

 private:
    friend struct lazy_readter<wrapped_value_type>;
    wrapped_value_type
    generate() const
    {
        return wrapped_.read() * scale_factor_;
    }
    Wrapped wrapped_;
    wrapped_value_type scale_factor_;
    lazy_readter<wrapped_value_type> lazy_readter_;
};
template<class Wrapped>
scaling_accessor_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type>
scale(
    Wrapped const& wrapped,
    typename accessor_value_type<Wrapped>::type scale_factor)
{
    return scaling_accessor_wrapper<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped), scale_factor);
}

// offwrite(a, offwrite) presents an offwrite view of a, where a is an accessor
// to a numeric value.
template<class Wrapped>
struct offwrite_accessor_wrapper
    : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    offwrite_accessor_wrapper()
    {
    }
    offwrite_accessor_wrapper(
        Wrapped wrapped, typename accessor_value_type<Wrapped>::type offwrite)
        : wrapped_(wrapped), offwrite_(offwrite)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    wrapped_value_type const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename accessor_value_type<Wrapped>::type const& value) const
    {
        wrapped_.write(value - offwrite_);
    }

 private:
    friend struct lazy_readter<wrapped_value_type>;
    wrapped_value_type
    generate() const
    {
        return wrapped_.read() + offwrite_;
    }
    Wrapped wrapped_;
    wrapped_value_type offwrite_;
    lazy_readter<wrapped_value_type> lazy_readter_;
};
template<class Wrapped>
offwrite_accessor_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type>
offwrite(
    Wrapped const& wrapped,
    typename accessor_value_type<Wrapped>::type offwrite)
{
    return offwrite_accessor_wrapper<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped), offwrite);
}

// add_input_rounder(accessor, step) rounds input from the UI to the given
// accessor so that its always a multiple of step.
template<class Wrapped>
struct rounding_accessor_wrapper
    : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    rounding_accessor_wrapper()
    {
    }
    rounding_accessor_wrapper(
        Wrapped wrapped, typename accessor_value_type<Wrapped>::type step)
        : wrapped_(wrapped), step_(step)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    typename accessor_value_type<Wrapped>::type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename accessor_value_type<Wrapped>::type const& value) const
    {
        wrapped_.write(
            std::floor(
                value / step_ +
                typename accessor_value_type<Wrapped>::type(0.5))
            * step_);
    }

 private:
    Wrapped wrapped_;
    typename accessor_value_type<Wrapped>::type step_;
};
template<class Wrapped>
rounding_accessor_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type>
add_input_rounder(
    Wrapped const& wrapped, typename accessor_value_type<Wrapped>::type step)
{
    return rounding_accessor_wrapper<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped), step);
}

// Given an accessor to a structure, select_field(accessor, field_ptr) returns
// an accessor to the specified field within the structure.
template<class StructureAccessor, class Field>
struct field_accessor : accessor<Field>
{
    typedef
        typename accessor_value_type<StructureAccessor>::type structure_type;
    typedef Field structure_type::*field_ptr;
    field_accessor()
    {
    }
    field_accessor(StructureAccessor structure, field_ptr field)
        : structure_(structure), field_(field)
    {
    }
    bool
    is_readable() const
    {
        return structure_.is_readable();
    }
    Field const&
    read() const
    {
        structure_type const& structure = structure_.read();
        return structure.*field_;
    }
    id_interface const&
    id() const
    {
        // Apparently pointers-to-members aren't comparable for order, so
        // instead we use the address of the field if it were in a structure
        // that started at address 0.
        id_ = combine_ids(
            ref(structure_.id()), make_id(&(((structure_type*) 0)->*field_)));
        return id_;
    }
    bool
    is_writable() const
    {
        return structure_.is_writable();
    }
    void
    write(Field const& x) const
    {
        // Allowing a field to be write when the rest of the structure isn't
        // readable is questionable.
        structure_type s
            = structure_.is_readable() ? structure_.read() : structure_type();
        s.*field_ = x;
        structure_.write(s);
    }

 private:
    StructureAccessor structure_;
    field_ptr field_;
    mutable id_pair<id_ref, value_id<Field*>> id_;
};
template<class StructureAccessor, class Field>
field_accessor<
    typename copyable_accessor_helper<StructureAccessor const&>::result_type,
    Field>
select_field(
    StructureAccessor const& structure,
    Field accessor_value_type<StructureAccessor>::type::*field)
{
    return field_accessor<
        typename copyable_accessor_helper<
            StructureAccessor const&>::result_type,
        Field>(make_accessor_copyable(structure), field);
}

// text(x), where x is a string constant, creates a read-only accessor
// for accessing x as a string. Its ID is the pointer to the text.
struct text : accessor<string>
{
    text()
    {
    }
    text(char const* x) : text_(x), id_(x)
    {
    }
    id_interface const&
    id() const
    {
        return id_;
    }
    bool
    is_readable() const
    {
        return true;
    }
    string const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(string const& value) const
    {
    }

 private:
    char const* text_;
    value_id<char const*> id_;
    friend struct lazy_readter<string>;
    string
    generate() const
    {
        return string(text_);
    }
    lazy_readter<string> lazy_readter_;
};

// unwrap_optional(accessor) takes an accessor to an optional value
// and creates an accessor to the underlying value. It's only readable if the
// wrapped accessor is readable and contains a valid value.
template<class OptionalAccessor>
struct optional_accessor_unwrapper
    : accessor<typename accessor_value_type<OptionalAccessor>::type::value_type>
{
    typedef typename accessor_value_type<OptionalAccessor>::type::value_type
        underlying_value_type;
    optional_accessor_unwrapper()
    {
    }
    optional_accessor_unwrapper(OptionalAccessor const& accessor)
        : accessor_(accessor)
    {
    }
    bool
    is_readable() const
    {
        return accessor_.is_readable() && accessor_.read();
    }
    underlying_value_type const&
    read() const
    {
        return accessor_.read().read();
    }
    bool
    is_writable() const
    {
        return accessor_.is_writable();
    }
    void
    write(underlying_value_type const& value) const
    {
        accessor_.write(value);
    }
    id_interface const&
    id() const
    {
        return accessor_.id();
    }

 private:
    OptionalAccessor accessor_;
};
template<class OptionalAccessor>
optional_accessor_unwrapper<
    typename copyable_accessor_helper<OptionalAccessor const&>::result_type>
unwrap_optional(OptionalAccessor const& accessor)
{
    return optional_accessor_unwrapper<typename copyable_accessor_helper<
        OptionalAccessor const&>::result_type>(
        make_accessor_copyable(accessor));
}

// lazy_apply(f, args...), where :args are all accessors, yields an accessor
// to the result of lazily applying the function :f to the values of :args.
// Note that doing this in true variadic fashion is a little insane, so I'm
// just doing the two overloads I need for now...

template<class Result, class Function, class Arg>
struct lazy_apply1_accessor : accessor<Result>
{
    lazy_apply1_accessor()
    {
    }
    lazy_apply1_accessor(Function const& f, Arg const& arg) : f_(f), arg_(arg)
    {
    }
    id_interface const&
    id() const
    {
        return arg_.id();
    }
    bool
    is_readable() const
    {
        return arg_.is_readable();
    }
    Result const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Result const& value) const
    {
    }

 private:
    friend struct lazy_readter<Result>;
    Result
    generate() const
    {
        return f_(arg_.read());
    }
    Function f_;
    Arg arg_;
    lazy_readter<Result> lazy_readter_;
};
template<class Function, class Arg>
auto
lazy_apply(Function const& f, Arg const& arg)
{
    return lazy_apply1_accessor<
        decltype(f(read(arg))),
        Function,
        typename copyable_accessor_helper<Arg const&>::result_type>(
        f, make_accessor_copyable(arg));
}

template<class Result, class Function, class Arg0, class Arg1>
struct lazy_apply2_accessor : accessor<Result>
{
    lazy_apply2_accessor()
    {
    }
    lazy_apply2_accessor(Function const& f, Arg0 const& arg0, Arg1 const& arg1)
        : f_(f), arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    id() const
    {
        id_ = combine_ids(ref(arg0_.id()), ref(arg1_.id()));
        return id_;
    }
    bool
    is_readable() const
    {
        return arg0_.is_readable() && arg1_.is_readable();
    }
    Result const&
    read() const
    {
        return lazy_readter_.read(*this);
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Result const& value) const
    {
    }

 private:
    friend struct lazy_readter<Result>;
    Result
    generate() const
    {
        return f_(arg0_.read(), arg1_.read());
    }
    Function f_;
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    lazy_readter<Result> lazy_readter_;
};
template<class Function, class Arg0, class Arg1>
auto
lazy_apply(Function const& f, Arg0 const& arg0, Arg1 const& arg1)
{
    return lazy_apply2_accessor<
        decltype(f(read(arg0), read(arg1))),
        Function,
        typename copyable_accessor_helper<Arg0 const&>::result_type,
        typename copyable_accessor_helper<Arg1 const&>::result_type>(
        f, make_accessor_copyable(arg0), make_accessor_copyable(arg1));
}

// Define various operators for accessors.

#define ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(op)                               \
    template<                                                                  \
        class A,                                                               \
        class B,                                                               \
        std::enable_if_t<                                                      \
            std::is_base_of<untyped_accessor_base, A>::value                   \
                && std::is_base_of<untyped_accessor_base, B>::value,           \
            int> = 0>                                                          \
    auto operator op(A const& a, B const& b)                                   \
    {                                                                          \
        return lazy_apply([](auto a, auto b) { return a op b; }, a, b);        \
    }

ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(+)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(-)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(*)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(/)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR (^)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(%)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(&)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(|)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(<<)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(>>)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(==)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(!=)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(<)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(<=)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(>)
ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR(>=)

#undef ALIA_DEFINE_BINARY_ACCESSOR_OPERATOR

#define ALIA_DEFINE_UNARY_ACCESSOR_OPERATOR(op)                                \
    template<                                                                  \
        class A,                                                               \
        std::enable_if_t<                                                      \
            std::is_base_of<untyped_accessor_base, A>::value,                  \
            int> = 0>                                                          \
    auto operator op(A const& a)                                               \
    {                                                                          \
        return lazy_apply([](auto a) { return op a; }, a);                     \
    }

ALIA_DEFINE_UNARY_ACCESSOR_OPERATOR(-)
ALIA_DEFINE_UNARY_ACCESSOR_OPERATOR(!)

#undef ALIA_DEFINE_UNARY_ACCESSOR_OPERATOR

// The || and && operators require special implementations because they don't
// necessarily need to evaluate both of their arguments...

template<class Arg0, class Arg1>
struct logical_or_accessor : accessor<bool>
{
    logical_or_accessor()
    {
    }
    logical_or_accessor(Arg0 const& arg0, Arg1 const& arg1)
        : arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    id() const
    {
        id_ = combine_ids(ref(arg0_.id()), ref(arg1_.id()));
        return id_;
    }
    bool
    is_readable() const
    {
        // Obviously, this is readable if both of its arguments are readable.
        // However, it's also readable if only one is readable and its value is
        // true.
        return arg0_.is_readable() && arg1_.is_readable()
               || arg0_.is_readable() && arg0_.read()
               || arg1_.is_readable() && arg1_.read();
    }
    bool const&
    read() const
    {
        value_ = arg0_.is_readable() && arg0_.read()
                 || arg1_.is_readable() && arg1_.read();
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    mutable bool value_;
};
template<
    class A,
    class B,
    std::enable_if_t<
        std::is_base_of<untyped_accessor_base, A>::value
            && std::is_base_of<untyped_accessor_base, B>::value,
        int> = 0>
auto
operator||(A const& a, B const& b)
{
    return logical_or_accessor<
        typename copyable_accessor_helper<A const&>::result_type,
        typename copyable_accessor_helper<B const&>::result_type>(
        make_accessor_copyable(a), make_accessor_copyable(b));
}

template<class Arg0, class Arg1>
struct logical_and_accessor : accessor<bool>
{
    logical_and_accessor()
    {
    }
    logical_and_accessor(Arg0 const& arg0, Arg1 const& arg1)
        : arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    id() const
    {
        id_ = combine_ids(ref(arg0_.id()), ref(arg1_.id()));
        return id_;
    }
    bool
    is_readable() const
    {
        // Obviously, this is readable if both of its arguments are readable.
        // However, it's also readable if only one is readable and its value is
        // false.
        return arg0_.is_readable() && arg1_.is_readable()
               || arg0_.is_readable() && !arg0_.read()
               || arg1_.is_readable() && !arg1_.read();
    }
    bool const&
    read() const
    {
        value_
            = !(arg0_.is_readable() && !arg0_.read()
                || arg1_.is_readable() && !arg1_.read());
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    mutable bool value_;
};
template<
    class A,
    class B,
    std::enable_if_t<
        std::is_base_of<untyped_accessor_base, A>::value
            && std::is_base_of<untyped_accessor_base, B>::value,
        int> = 0>
auto
operator&&(A const& a, B const& b)
{
    return logical_and_accessor<
        typename copyable_accessor_helper<A const&>::result_type,
        typename copyable_accessor_helper<B const&>::result_type>(
        make_accessor_copyable(a), make_accessor_copyable(b));
}

// _is_readable(x) yields an accessor to a boolean which indicates whether or
// not x is readable. (The returned accessor is always readable itself.)
template<class Wrapped>
struct readtability_accessor : regular_accessor<bool>
{
    readtability_accessor()
    {
    }
    readtability_accessor(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.is_readable();
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
_is_readable(Wrapped const& wrapped)
{
    return readtability_accessor<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped));
}

// _is_writable(x) yields an accessor to a boolean which indicates whether or
// not x is writable. (The returned accessor is always readable.)
template<class Wrapped>
struct writetability_accessor : regular_accessor<bool>
{
    writetability_accessor()
    {
    }
    writetability_accessor(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.is_writable();
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
_is_writable(Wrapped const& wrapped)
{
    return writetability_accessor<
        typename copyable_accessor_helper<Wrapped const&>::result_type>(
        make_accessor_copyable(wrapped));
}

} // namespace alia

#endif
