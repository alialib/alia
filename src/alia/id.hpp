#ifndef ALIA_ID_HPP
#define ALIA_ID_HPP

#include <sstream>
#include <memory>
#include <functional>
#include <alia/common.hpp>

// This file implements the concept of IDs in alia.

namespace alia {

// id_interface defines the interface required of all ID types.
struct id_interface
{
 public:
    virtual ~id_interface() {}

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

    // Generate a hash of the ID.
    virtual size_t hash() const = 0;
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

// The following allow for using IDs as keys in a map or unordered_map.
// The IDs are stored separately as owned_ids in the mapped values and pointers
// are used as keys. This allows searches to be done on pointers to other IDs.

struct id_interface_pointer_less_than_test
{
    bool operator()(id_interface const* a, id_interface const* b) const
    { return *a < *b; }
};

struct id_interface_pointer_equality_test
{
    bool operator()(id_interface const* a, id_interface const* b) const
    { return *a == *b; }
};

struct id_interface_pointer_hash
{
    size_t operator()(id_interface const* id) const
    { return id->hash(); }
};

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
    void clear() { id_.reset(); }
    void store(id_interface const& new_id) { id_.reset(new_id.clone()); }
    bool is_initialized() const { return id_ ? true : false; }
    id_interface const& get() const { return *id_; }
    bool matches(id_interface const& id) const { return id_ && *id_ == id; }
    friend void swap(owned_id& a, owned_id& b) { swap(a.id_, b.id_); }
 private:
    alia__shared_ptr<id_interface> id_;
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
    value_id() {}

    value_id(Value value)
      : value_(value) {}

    Value const& value() const { return value_; }

    id_interface* clone() const { return new value_id(value_); }

    bool equals(id_interface const& other) const
    {
        value_id const& other_id = static_cast<value_id const&>(other);
        return value_ == other_id.value_;
    }

    bool less_than(id_interface const& other) const
    {
        value_id const& other_id = static_cast<value_id const&>(other);
        return value_ < other_id.value_;
    }

    void stream(std::ostream& o) const { o << value_; }

    void deep_copy(id_interface* copy) const
    { *static_cast<value_id*>(copy) = *this; }

    size_t hash() const
    { return std::hash<Value>()(value_); }

 private:
    Value value_;
};

// make_id(value) creates a value_id with the given value.
template<class Value>
value_id<Value> make_id(Value value)
{ return value_id<Value>(value); }

// value_id_by_reference is like value_id but takes a pointer to the value.
// The value is only copied if the ID is cloned or deep-copied.
template<class Value>
struct value_id_by_reference : id_interface
{
    value_id_by_reference()
      : value_(0), storage_()
    {}

    value_id_by_reference(Value const* value)
      : value_(value), storage_()
    {}

    id_interface* clone() const
    {
        value_id_by_reference* copy = new value_id_by_reference;
        this->deep_copy(copy);
        return copy;
    }

    bool equals(id_interface const& other) const
    {
        value_id_by_reference const& other_id =
            static_cast<value_id_by_reference const&>(other);
        return *value_ == *other_id.value_;
    }

    bool less_than(id_interface const& other) const
    {
        value_id_by_reference const& other_id =
            static_cast<value_id_by_reference const&>(other);
        return *value_ < *other_id.value_;
    }

    void stream(std::ostream& o) const { o << *value_; }

    void deep_copy(id_interface* copy) const
    {
        auto& typed_copy = *static_cast<value_id_by_reference*>(copy);
        if (storage_)
        {
            typed_copy.storage_ = this->storage_;
            typed_copy.value_ = this->value_;
        }
        else
        {
            typed_copy.storage_.reset(new Value(*this->value_));
            typed_copy.value_ = typed_copy.storage_.get();
        }
    }

    size_t hash() const
    { return std::hash<Value>()(*value_); }

 private:
    Value const* value_;
    alia__shared_ptr<Value> storage_;
};

// make_id_by_reference
template<class Value>
value_id_by_reference<Value> make_id_by_reference(Value const& value)
{ return value_id_by_reference<Value>(&value); }

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

    size_t hash() const
    { return id0_.hash() ^ id1_.hash(); }

 private:
    Id0 id0_;
    Id1 id1_;
};

// combine_ids(id0, id1) combines id0 and id1 into a single ID pair.
template<class Id0, class Id1>
auto combine_ids(Id0 const& id0, Id1 const& id1)
{ return id_pair<Id0,Id1>(id0, id1); }

// Combine more than two IDs into nested pairs.
template<class Id0, class Id1, class ...Rest>
auto combine_ids(Id0 const& id0, Id1 const& id1, Rest const& ...rest)
{ return combine_ids(combine_ids(id0, id1), rest...); }

// Allow combine_ids() to take a single argument for variadic purposes.
template<class Id0>
auto combine_ids(Id0 const& id0)
{ return id0; }

// ref(id) wraps a reference to an id_interface so that it can be combined.
struct id_ref : id_interface
{
    id_ref() : id_(0), ownership_() {}

    id_ref(id_interface const* id) : id_(id), ownership_() {}

    id_interface* clone() const
    {
        id_ref* copy = new id_ref;
        this->deep_copy(copy);
        return copy;
    }

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
        auto& typed_copy = *static_cast<id_ref*>(copy);
        if (ownership_)
        {
            typed_copy.ownership_ = ownership_;
            typed_copy.id_ = id_;
        }
        else
        {
            typed_copy.ownership_.reset(id_->clone());
            typed_copy.id_ = typed_copy.ownership_.get();
        }
    }

    size_t hash() const
    { return id_->hash(); }

 private:
    id_interface const* id_;
    alia__shared_ptr<id_interface> ownership_;
};
static inline id_ref ref(id_interface const* id)
{ return id_ref(id); }

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

}

namespace std
{
    template<>
    struct hash<alia::local_id>
    {
        size_t operator()(alia::local_id const& id) const
        {
            return hash<int*>()(id.tag.get()) ^
                hash<alia::counter_type>()(id.version);
        }
    };
}

namespace alia {

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
{ return make_id_by_reference(identity.id); }

// no_id can be used when you have nothing to identify.
struct no_id_type {};
static value_id<no_id_type*> const no_id(0);

}

#endif
