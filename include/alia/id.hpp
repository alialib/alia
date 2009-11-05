#ifndef ALIA_ID_HPP
#define ALIA_ID_HPP

#include <boost/noncopyable.hpp>
#include <string>

namespace alia {

class id_interface : boost::noncopyable
{
 public:
    virtual ~id_interface() {}
    virtual id_interface* clone() const = 0;
    virtual bool equals(id_interface const& other) const = 0;
    virtual bool less_than(id_interface const& other) const = 0;
};

struct id_ref
{
    explicit id_ref(id_interface const* id) : id(id) {}
    id_ref() : id(0) {}
    id_interface const* id;
};
bool operator==(id_ref const& a, id_ref const& b);
bool operator!=(id_ref const& a, id_ref const& b);
bool operator<(id_ref const& a, id_ref const& b);

struct owned_id : boost::noncopyable
{
    owned_id() : id(0) {}
    owned_id(id_interface* id) : id(id) {}
    ~owned_id() { delete id; }
    id_interface* id;
};

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

template<class Value>
static inline typed_id<Value> make_id(Value const& value)
{ return typed_id<Value>(value); }
static inline typed_id<std::string> make_id(char const* value)
{ return typed_id<std::string>(value); }

}

#endif
