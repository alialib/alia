#ifndef ALIA_ID_HPP
#define ALIA_ID_HPP

#include <boost/noncopyable.hpp>
#include <string>

// This file implements the concept of IDs in alia.
// IDs are used in alia to identify portions of the UI that require explicit
// naming.  IDs can be any type, but they must be copyable and comparable for
// equality and ordering (i.e., supply == and < operators).

// Note that the utilities provided in this file are meant to be used
// internally to implement alia library features.  A typical library user
// should never have to use anything in here directly.

namespace alia {

// id_interface provides a run-time polymorphic interface to the functionality
// of an ID. This allows the use of IDs without excessive templating.
class id_interface
{
 public:
    virtual ~id_interface() {}
    virtual id_interface* clone() const = 0;
    virtual bool equals(id_interface const& other) const = 0;
    virtual bool less_than(id_interface const& other) const = 0;
};

// typed_id<Value> takes a normal ID type (of type Value) and implements
// id_interface for it.
template<class Value>
class typed_id : public id_interface
{
 public:
    explicit typed_id(Value const& value) : value_(value) {}

    id_interface* clone() const { return new typed_id(value_); }

    bool equals(id_interface const& other) const
    { return value_ == static_cast<typed_id const&>(other).value_; }

    bool less_than(id_interface const& other) const
    { return value_ < static_cast<typed_id const&>(other).value_; }

 private:
    Value value_;
};

// id_ref is a light-weight reference to an ID with no ownership.
// Objects of type id_ref can be tested for equality and ordering.
struct id_ref
{
    explicit id_ref(id_interface const* id) : id(id) {}
    id_ref() : id(0) {}
    id_interface const* id;
};
bool operator==(id_ref const& a, id_ref const& b);
bool operator!=(id_ref const& a, id_ref const& b);
bool operator<(id_ref const& a, id_ref const& b);

// owned_id is used to store an ID over the long-term (i.e., for more than a
// single pass). Once you've obtained an ID from the application (via make_id),
// you call clone() and store the clone in an owned_id.
struct owned_id : boost::noncopyable
{				
    owned_id() : id(0) {}
    owned_id(id_interface* id) : id(id) {}
    ~owned_id() { delete id; }
    id_interface* id;
};

// make_id() is a utility function for wrapping normal ID types in typed_id.
// All IDs that come from application code should pass through this.
// TODO: It's unclear if the exceptional behavior for char const* values is
// really a good idea.
template<class Value>
static inline typed_id<Value> make_id(Value const& value)
{ return typed_id<Value>(value); }
static inline typed_id<std::string> make_id(char const* value)
{ return typed_id<std::string>(value); }

}

#endif
