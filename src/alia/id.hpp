#ifndef ALIA_ID_HPP
#define ALIA_ID_HPP

#include <ostream>
#include <memory>
#include <alia/common.hpp>

// This file implements the concept of IDs in alia.

namespace alia {

// IDs in alia have an associated context which specifies the scope in which
// they are valid. IDs won't be used outside of their specified context.
enum id_context
{
    // The ID is not valid at all.
    ID_CONTEXT_NOWHERE,
    // The ID is only valid within the current application instance.
    ID_CONTEXT_APP_INSTANCE,
    // The ID is valid across application instances, but only on this machine.
    ID_CONTEXT_LOCAL_MACHINE,
    // The ID is universally valid.
    ID_CONTEXT_UNIVERSAL
};

// id_interface defines the interface required of all ID types.
struct id_interface
{
 public:
    virtual ~id_interface() {}

    // Get the context in which this ID is valid.
    virtual id_context context() const = 0;

    // Create a stand-alone copy of the ID.
    virtual id_interface* clone() const = 0;

    // Given another ID of the same type, set it equal to a stand-alone copy
    // of this ID.
    virtual void deep_copy(id_interface* copy) const = 0;

    // Given another ID of the same type, return true iff it's equal to this
    // one.
    virtual bool equals(id_interface const& other) const = 0;

    // Given another ID of the same type, return true iff it's less than this
    // one.
    virtual bool less_than(id_interface const& other) const = 0;

    // Write a textual representation of the ID to the given ostream.
    virtual void stream(std::ostream& o) const = 0;
};

// The following convert the interface to the ID operations into the usual form
// that one would expect, as free functions.
static inline bool operator==(id_interface const& a, id_interface const& b)
{
    // Apparently it's faster to compare the name pointers for equality before
    // resorting to actually comparing the typeid objects themselves.
    return (typeid(a).name() == typeid(b).name() || typeid(a) == typeid(b)) &&
        a.equals(b);
}
static inline bool operator!=(id_interface const& a, id_interface const& b)
{ return !(a == b); }
bool operator<(id_interface const& a, id_interface const& b);
static inline std::ostream& operator<<(std::ostream& o, id_interface const& id)
{ id.stream(o); return o; }

// Get the context of the given ID.
static inline id_context get_context(id_interface const& id)
{ return id.context(); }
// Is the ID valid?
static inline bool is_valid(id_interface const& id)
{ return get_context(id) != ID_CONTEXT_NOWHERE; }

// Given an ID and some storage, attempts to deep copy the ID into the storage
// if the types are compatible. Otherwise, deletes the storage and returns
// a clone.
void clone_into(id_interface*& storage, id_interface const* id);

// owned_id is used to store an ID over the long-term. If you'll need to
// reference an ID outside the current stack frame, there's no guarantee that
// it'll be valid then, so you need to store it in an owned_id, which will
// clone it and assume ownership of the clone.
struct owned_id
{
    owned_id() : id_(0) {}
    owned_id(owned_id const& other)
      : id_(0)
    {
        clone_into(id_, other.id_);
    }
    ~owned_id() { delete id_; }
    owned_id& operator=(owned_id const& other)
    {
        clone_into(id_, other.id_);
        return *this;
    }
    void clear() { delete id_; id_ = 0; }
    void store(id_interface const& new_id)
    {
        clone_into(id_, &new_id);
    }
    bool is_initialized() const { return id_ != 0; }
    id_interface const& get() const { return *id_; }
    bool matches(id_interface const& id) const
    { return id_ != 0 && *id_ == id; }
    friend void swap(owned_id& a, owned_id& b)
    {
        id_interface* tmp = a.id_;
        a.id_ = b.id_;
        b.id_ = tmp;
    }
 private:
    id_interface* id_;
};
bool operator==(owned_id const& a, owned_id const& b);
bool operator!=(owned_id const& a, owned_id const& b);
bool operator<(owned_id const& a, owned_id const& b);
std::ostream& operator<<(std::ostream& o, owned_id const& id);

// value_id<Value> takes a regular type (of type Value) and implements
// id_interface for it.
// The type Value must be copyable, comparable for equality and ordering
// (i.e., supply == and < operators), and convertible to a string.
template<class Value>
struct value_id : id_interface
{
 public:
    value_id() : context_(ID_CONTEXT_NOWHERE) {}

    value_id(Value value, id_context context)
      : value_(value), context_(context) {}

    id_interface* clone() const { return new value_id(value_, context_); }

    id_context context() const { return context_; }

    bool equals(id_interface const& other) const
    {
        value_id const& other_id = static_cast<value_id const&>(other);
        return value_ == other_id.value_ && context_ == other_id.context_;
    }

    bool less_than(id_interface const& other) const
    {
        value_id const& other_id = static_cast<value_id const&>(other);
        return value_ < other_id.value_ ||
            value_ == other_id.value_ && context_ < other_id.context_;
    }

    void stream(std::ostream& o) const { o << value_; }

    void deep_copy(id_interface* copy) const
    { *static_cast<value_id*>(copy) = *this; }

 private:
    Value value_;
    id_context context_;
};

// make_id(value, context) creates a value_id with the given value and context.
template<class Value>
value_id<Value> make_id(Value value, id_context context)
{ return value_id<Value>(value, context); }

// get_id_context(value) is used by make_id(value) to determine the context
// for the ID if none is given explicitly.
// All primitive types are universally valid.
#define ALIA_UNIVERSAL_TYPE(T) \
    static inline alia::id_context get_id_context(T value) \
    { return ID_CONTEXT_UNIVERSAL; }
ALIA_UNIVERSAL_TYPE(bool)
ALIA_UNIVERSAL_TYPE(signed char)
ALIA_UNIVERSAL_TYPE(unsigned char)
ALIA_UNIVERSAL_TYPE(signed short)
ALIA_UNIVERSAL_TYPE(unsigned short)
ALIA_UNIVERSAL_TYPE(signed int)
ALIA_UNIVERSAL_TYPE(unsigned int)
ALIA_UNIVERSAL_TYPE(signed long)
ALIA_UNIVERSAL_TYPE(unsigned long)
ALIA_UNIVERSAL_TYPE(signed long long)
ALIA_UNIVERSAL_TYPE(unsigned long long)
ALIA_UNIVERSAL_TYPE(wchar_t)
ALIA_UNIVERSAL_TYPE(float)
ALIA_UNIVERSAL_TYPE(double)
// All pointers are local to the application instance.
template<class T>
id_context get_id_context(T const* value)
{ return ID_CONTEXT_APP_INSTANCE; }
// Conservatively make all other values local to the application instance.
template<class T>
id_context get_id_context(T const& value)
{ return ID_CONTEXT_APP_INSTANCE; }

// make_id(value)
template<class Value>
value_id<Value> make_id(Value value)
{ return value_id<Value>(value, get_id_context(value)); }

// value_id_by_reference is like value_id but takes a pointer to the value.
// The value is only copied if the ID is cloned or deep-copied.
template<class Value>
struct value_id_by_reference : id_interface
{
    value_id_by_reference()
      : value_(0), storage_(0), context_(ID_CONTEXT_NOWHERE)
    {}

    value_id_by_reference(value_id_by_reference const& other)
    {
        copy(other);
    }

    value_id_by_reference& operator=(value_id_by_reference const& other)
    {
        delete storage_;
        copy(other);
        return *this;
    }

    value_id_by_reference(Value const* value, id_context context)
      : value_(value), storage_(0), context_(context)
    {}

    ~value_id_by_reference() { delete storage_; }

    id_interface* clone() const
    {
        Value* storage = new Value(*value_);
        return new value_id_by_reference(storage, storage, context_);
    }

    id_context context() const { return context_; }

    bool equals(id_interface const& other) const
    {
        value_id_by_reference const& other_id =
            static_cast<value_id_by_reference const&>(other);
        return *value_ == *other_id.value_ && context_ == other_id.context_;
    }

    bool less_than(id_interface const& other) const
    {
        value_id_by_reference const& other_id =
            static_cast<value_id_by_reference const&>(other);
        return *value_ < *other_id.value_ ||
            *value_ == *other_id.value_ && context_ < other_id.context_;
    }

    void stream(std::ostream& o) const { o << *value_; }

    void deep_copy(id_interface* copy) const
    {
        Value* storage = new Value(*value_);
        *static_cast<value_id_by_reference*>(copy) =
            value_id_by_reference(storage, storage, context_);
    }

 private:
    value_id_by_reference(Value const* value, Value* storage,
        id_context context)
      : value_(value), storage_(storage), context_(context)
    {}

    void copy(value_id_by_reference const& other)
    {
        if (other.storage_)
        {
            value_ = storage_ = new Value(*other.value_);
        }
        else
        {
            storage_ = 0;
            value_ = other.value_;
        }
        context_ = other.context_;
    }

    Value const* value_;
    Value* storage_;
    id_context context_;
};

// make_id_by_reference
template<class Value>
value_id_by_reference<Value> make_id_by_reference(
    Value const& value, id_context context)
{ return value_id_by_reference<Value>(&value, context); }
template<class Value>
value_id_by_reference<Value> make_id_by_reference(Value const& value)
{ return value_id_by_reference<Value>(&value, get_id_context(value)); }

// id_pair implements the ID interface for a pair of IDs.
template<class Id0, class Id1>
struct id_pair : id_interface
{
    id_pair() {}

    id_pair(Id0 const& id0, Id1 const& id1)
      : id0_(id0), id1_(id1) {}

    id_interface* clone() const
    {
        id_pair* copy = new id_pair;
        this->deep_copy(copy);
        return copy;
    }

    id_context context() const
    {
        id_context ctx0 = id0_.context();
        id_context ctx1 = id1_.context();
        return ctx0 < ctx1 ? ctx0 : ctx1;
    }

    bool equals(id_interface const& other) const
    {
        id_pair const& other_id = static_cast<id_pair const&>(other);
        return id0_.equals(other_id.id0_) && id1_.equals(other_id.id1_);
    }

    bool less_than(id_interface const& other) const
    {
        id_pair const& other_id = static_cast<id_pair const&>(other);
        return id0_.less_than(other_id.id0_) ||
            id0_.equals(other_id.id0_) && id1_.less_than(other_id.id1_);
    }

    void stream(std::ostream& o) const
    { o << "(" << id0_ << "," << id1_ << ")"; }

    void deep_copy(id_interface* copy) const
    {
        id_pair* typed_copy = static_cast<id_pair*>(copy);
        id0_.deep_copy(&typed_copy->id0_);
        id1_.deep_copy(&typed_copy->id1_);
    }

 private:
    Id0 id0_;
    Id1 id1_;
};

// combine_ids(id0, id1) combines id0 and id1 into a single ID pair.
template<class Id0, class Id1>
id_pair<Id0,Id1> combine_ids(Id0 const& id0, Id1 const& id1)
{ return id_pair<Id0,Id1>(id0, id1); }

// ref(id) wraps a reference to an id_interface so that it can be combined.
struct id_ref : id_interface
{
    id_ref() : id_(0), owner_(false) {}

    id_ref(id_ref const& other)
    {
        owner_ = other.owner_;
        id_ = owner_ ? other.id_->clone() : other.id_;
    }

    id_ref& operator=(id_ref const& other)
    {
        if (owner_)
            delete id_;
        owner_ = other.owner_;
        id_ = owner_ ? other.id_->clone() : other.id_;
        return *this;
    }

    id_ref(id_interface const& id, bool owner)
      : id_(&id), owner_(owner)
    {}

    ~id_ref() { if (owner_) delete id_; }

    id_interface* clone() const
    { return new id_ref(*id_->clone(), true); }

    id_context context() const
    { return id_->context(); }

    bool equals(id_interface const& other) const
    {
        id_ref const& other_id = static_cast<id_ref const&>(other);
        return *id_ == *other_id.id_;
    }

    bool less_than(id_interface const& other) const
    {
        id_ref const& other_id = static_cast<id_ref const&>(other);
        return *id_ < *other_id.id_;
    }

    void stream(std::ostream& o) const
    { o << *id_; }

    void deep_copy(id_interface* copy) const
    {
        id_ref* typed_copy = static_cast<id_ref*>(copy);
        assert(typed_copy->owner_);
        clone_into(const_cast<id_interface*&>(typed_copy->id_), id_);
    }

 private:
    id_interface const* id_;
    // If this is true, the id_ref provides ownership of the ID.
    bool owner_;
};
static inline id_ref ref(id_interface const& id)
{ return id_ref(id, false); }

// local_identity establishes an identity that's unique within the local
// application instance.
// The identity is versioned. Calling inc_version(identity) increases the
// identity's version number.
// Calling get_id(identity) returns an ID which identifies the current version
// of the identity.

struct local_id
{
    alia__shared_ptr<int> tag;
    counter_type version;
};
static inline bool operator==(local_id const& a, local_id const& b)
{ return a.tag == b.tag && a.version == b.version; }
static inline bool operator!=(local_id const& a, local_id const& b)
{ return !(a == b); }
static inline bool operator<(local_id const& a, local_id const& b)
{ return a.tag < b.tag || a.tag == b.tag && a.version < b.version; }

std::ostream& operator<<(std::ostream& o, local_id const& id);

local_id generate_local_id();

struct local_identity
{
    local_identity() : id(generate_local_id()) {}

    // Copying/assignment must produce a new identity.
    local_identity(local_identity const& other)
      : id(generate_local_id())
    {}
    local_identity& operator=(local_identity const& other)
    { id = generate_local_id(); return *this; }

    local_id id;
};

static inline void inc_version(local_identity& identity)
{ ++identity.id.version; }

static inline value_id_by_reference<local_id>
get_id(local_identity const& identity)
{ return make_id_by_reference(identity.id, ID_CONTEXT_APP_INSTANCE); }

// no_id can be used when you have nothing to identify.
struct no_id_type {};
static value_id<no_id_type*> const no_id(0, ID_CONTEXT_APP_INSTANCE);

}

#endif
