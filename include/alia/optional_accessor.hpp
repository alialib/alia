#ifndef ALIA_OPTIONAL_ACCESSOR_HPP
#define ALIA_OPTIONAL_ACCESSOR_HPP

#include <alia/accessor.hpp>
#include <boost/optional/optional.hpp>

namespace alia {

template<class T>
struct optional_inout_accessor : accessor<T>
{
    optional_inout_accessor(boost::optional<T>* v) : v_(v) {}
    bool is_valid() const { return *v_; }
    T get() const { return v_->get(); }
    void set(T const& value) const { *v_ = value; }
 private:
    boost::optional<T>* v_;
};
template<class T>
optional_inout_accessor<T>
inout(boost::optional<T>* value)
{
    return optional_inout_accessor<T>(value);
}

template<class T>
struct optional_in_accessor : accessor<T>
{
    optional_in_accessor(boost::optional<T> const* v) : v_(v) {}
    bool is_valid() const { return *v_; }
    T get() const { return v_->get(); }
    void set(T const& value) const {}
 private:
    boost::optional<T> const* v_;
};
template<class T>
optional_in_accessor<T>
in(boost::optional<T> const* value)
{
    return optional_in_accessor<T>(value);
}

template<class T>
struct optional_copying_accessor : accessor<T>
{
    optional_copying_accessor(boost::optional<T> const& v) : v_(v) {}
    bool is_valid() const { return v_; }
    T get() const { return v_.get(); }
    void set(T const& value) const {}
 private:
    boost::optional<T> v_;
};
template<class T>
optional_copying_accessor<T>
in(boost::optional<T> const& value)
{
    return optional_copying_accessor<T>(value);
}

}

#endif
