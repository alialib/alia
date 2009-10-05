#ifndef ALIA_ACCESSOR_HPP
#define ALIA_ACCESSOR_HPP

namespace alia {

// accessor interface

template<class T>
struct accessor
{
    typedef T value_type;
    virtual bool is_valid() const = 0;
    virtual T get() const = 0;
    virtual void set(T const& value) const = 0;
};

template<class T>
struct accessor_value_type
{
    typedef typename T::value_type type;
};
template<class T>
struct accessor_value_type<T const>
  : accessor_value_type<T>
{};
template<class T>
struct accessor_value_type<T&>
  : accessor_value_type<T>
{};
template<class T>
struct accessor_value_type<T const&>
  : accessor_value_type<T>
{};

// normal accessors

template<class T>
struct inout_accessor : accessor<T>
{
    inout_accessor(T* v) : v_(v) {}
    bool is_valid() const { return true; }
    T get() const { return *v_; }
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

template<class T>
struct in_accessor : accessor<T>
{
    in_accessor(T const* v) : v_(v) {}
    bool is_valid() const { return true; }
    T get() const { return *v_; }
    void set(T const& value) const {}
 private:
    T const* v_;
};
template<class T>
in_accessor<T>
in(T const* value)
{
    return in_accessor<T>(&value);
}

template<class T>
struct copying_accessor : accessor<T>
{
    copying_accessor(T const& v) : v_(v) {}
    bool is_valid() const { return true; }
    T get() const { return v_; }
    void set(T const& value) const {}
 private:
    T v_;
};
template<class T>
copying_accessor<T>
in(T const& value)
{
    return copying_accessor<T>(value);
}

// flagged accessors - These have a boolean flag to indicate if the
// accompanying is valid.

template<class T>
struct flagged_inout_accessor : accessor<T>
{
    flagged_inout_accessor(T* v, bool* flag) : v_(v), flag_(flag) {}
    bool is_valid() const { return *flag_; }
    T get() const { return *v_; }
    void set(T const& value) const { *v_ = value; *flag_ = true; }
 private:
    T* v_;
    bool* flag_;
};
template<class T>
flagged_inout_accessor<T>
inout(T* value, bool* flag)
{
    return flagged_inout_accessor<T>(value, flag);
}

template<class T>
struct flagged_in_accessor : accessor<T>
{
    flagged_in_accessor(T const* v, bool flag) : v_(v), flag_(flag) {}
    bool is_valid() const { return flag_; }
    T get() const { return *v_; }
    void set(T const& value) const {}
 private:
    T* v_;
    bool flag_;
};
template<class T>
flagged_in_accessor<T>
in(T const* value, bool flag)
{
    return flagged_in_accessor<T>(value, flag);
}

template<class T>
struct flagged_copying_accessor : accessor<T>
{
    flagged_copying_accessor(T const& v, bool flag) : v_(v), flag_(flag) {}
    bool is_valid() const { return flag_; }
    T get() const { return v_; }
    void set(T const& value) const {}
 private:
    T v_;
    bool flag_;
};
template<class T>
flagged_copying_accessor<T>
in(T const& value, bool flag)
{
    return flagged_copying_accessor<T>(value, flag);
}

// accessor wrappers...

// wraps a reference to an accessor so that it can be passed into another
// wrapper
template<class T>
struct indirect_accessor : accessor<T>
{
    indirect_accessor(accessor<T> const& wrapped) : wrapped_(&wrapped) {}
    bool is_valid() const { return wrapped_->is_valid(); }
    T get() const { return wrapped_->get(); }
    void set(T const& value) const { wrapped_->set(value); }
 private:
    accessor<T> const* wrapped_;
};
template<class T>
indirect_accessor<T>
ref(accessor<T> const& accessor)
{ return indirect_accessor<T>(accessor); }

// accessor cast
template<class Wrapped, class To>
struct accessor_caster : accessor<To>
{
    accessor_caster(Wrapped wrapped) : wrapped_(wrapped) {}
    bool is_valid() const { return wrapped_.is_valid(); }
    To get() const { return static_cast<To>(wrapped_.get()); }
    void set(To const& value) const
    {
        return wrapped_.set(
            static_cast<typename accessor_value_type<Wrapped>::type>(value));
    }
 private:
    Wrapped wrapped_;
};
template<class To, class Wrapped>
accessor_caster<Wrapped,To>
accessor_cast(Wrapped accessor)
{ return accessor_caster<Wrapped,To>(accessor); }

// disables the set() operation on an accessor
template<class Wrapped>
struct readonly_accessor_wrapper
  : accessor<typename accessor_value_type<Wrapped>::type>
{
    readonly_accessor_wrapper(Wrapped wrapped) : wrapped_(wrapped) {}
    bool is_valid() const { return wrapped_.is_valid(); }
    typename accessor_value_type<Wrapped>::type get() const
    { return wrapped_.get(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const {}
 private:
    Wrapped wrapped_;
};
template<class Wrapped>
readonly_accessor_wrapper<Wrapped>
make_readonly(Wrapped accessor)
{ return readonly_accessor_wrapper<Wrapped>(accessor); }

// presents a scaled view of the value in an accessor
template<class Wrapped>
struct scaling_accessor_wrapper
  : accessor<typename accessor_value_type<Wrapped>::type>
{
    scaling_accessor_wrapper(Wrapped wrapped,
        typename accessor_value_type<Wrapped>::type scale_factor)
      : wrapped_(wrapped), scale_factor_(scale_factor)
    {}
    bool is_valid() const { return wrapped_.is_valid(); }
    typename accessor_value_type<Wrapped>::type get() const
    { return wrapped_.get() * scale_factor_; }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    { wrapped_.set(value / scale_factor_); }
 private:
    Wrapped wrapped_;
    typename accessor_value_type<Wrapped>::type scale_factor_;
};
template<class Wrapped>
scaling_accessor_wrapper<Wrapped>
scale(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type scale_factor)
{ return scaling_accessor_wrapper<Wrapped>(accessor, scale_factor); }

// presents an offset view of the value in an accessor
template<class Wrapped>
struct offset_accessor_wrapper
  : accessor<typename accessor_value_type<Wrapped>::type>
{
    offset_accessor_wrapper(Wrapped wrapped,
        typename accessor_value_type<Wrapped>::type offset)
      : wrapped_(wrapped), offset_(offset)
    {}
    bool is_valid() const { return wrapped_.is_valid(); }
    typename accessor_value_type<Wrapped>::type get() const
    { return wrapped_.get() + offset_; }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    { wrapped_.set(value - offset_); }
 private:
    Wrapped wrapped_;
    typename accessor_value_type<Wrapped>::type offset_;
};
template<class Wrapped>
offset_accessor_wrapper<Wrapped>
offset(Wrapped accessor,
    typename accessor_value_type<Wrapped>::type offset)
{ return offset_accessor_wrapper<Wrapped>(accessor, offset); }

// TODO: does this belong here?
template<class T>
struct control_result
{
    bool changed;
    T new_value;
    // allows use within if statements without other unintended conversions
    typedef bool control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &control_result::changed : 0; }
};

}

#endif
