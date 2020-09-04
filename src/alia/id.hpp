#ifndef ALIA_ID_HPP
#define ALIA_ID_HPP

#include <alia/common.hpp>
#include <functional>
#include <memory>
#include <sstream>

// This file implements the concept of IDs in alia.

namespace alia {

// id_interface defines the interface required of all ID types.
struct id_interface
{
    virtual ~id_interface()
    {
    }

    // Create a standalone copy of the ID.
    virtual id_interface*
    clone() const = 0;

    // Given another ID of the same type, set it equal to a standalone copy
    // of this ID.
    virtual void
    deep_copy(id_interface* copy) const = 0;

    // Given another ID of the same type, return true iff it's equal to this
    // one.
    virtual bool
    equals(id_interface const& other) const = 0;

    // Given another ID of the same type, return true iff it's less than this
    // one.
    virtual bool
    less_than(id_interface const& other) const = 0;
};

// The following convert the interface of the ID operations into the usual form
// that one would expect, as free functions.

inline bool
operator==(id_interface const& a, id_interface const& b)
{
    // Apparently it's faster to compare the name pointers for equality before
    // resorting to actually comparing the typeid objects themselves.
    return (typeid(a).name() == typeid(b).name() || typeid(a) == typeid(b))
           && a.equals(b);
}

inline bool
operator!=(id_interface const& a, id_interface const& b)
{
    return !(a == b);
}

bool
operator<(id_interface const& a, id_interface const& b);

// The following allow the use of IDs as keys in a map.
// The IDs are stored separately as captured_ids in the mapped values and
// pointers are used as keys. This allows searches to be done on pointers to
// other IDs.

struct id_interface_pointer_less_than_test
{
    bool
    operator()(id_interface const* a, id_interface const* b) const
    {
        return *a < *b;
    }
};

// Given an ID and some storage, clone the ID into the storage as efficiently
// as possible. Specifically, if :storage already contains an ID of the same
// type, perform a deep copy into the existing ID. Otherwise, delete the
// existing ID (if any) and create a new clone to store there.
void
clone_into(id_interface*& storage, id_interface const* id);
// Same, but where the storage is a unique_ptr.
void
clone_into(std::unique_ptr<id_interface>& storage, id_interface const* id);

// captured_id is used to capture an ID for long-term storage (beyond the point
// where the id_interface reference will be valid).
struct captured_id
{
    captured_id()
    {
    }
    captured_id(id_interface const& id)
    {
        this->capture(id);
    }
    captured_id(captured_id const& other)
    {
        if (other.is_initialized())
            this->capture(other.get());
    }
    captured_id(captured_id&& other) noexcept
    {
        id_ = std::move(other.id_);
    }
    captured_id&
    operator=(captured_id const& other)
    {
        if (other.is_initialized())
            this->capture(other.get());
        else
            this->clear();
        return *this;
    }
    captured_id&
    operator=(captured_id&& other) noexcept
    {
        id_ = std::move(other.id_);
        return *this;
    }
    void
    clear()
    {
        id_.reset();
    }
    void
    capture(id_interface const& new_id)
    {
        clone_into(id_, &new_id);
    }
    bool
    is_initialized() const
    {
        return id_ ? true : false;
    }
    id_interface const&
    get() const
    {
        return *id_;
    }
    bool
    matches(id_interface const& id) const
    {
        return id_ && *id_ == id;
    }
    friend void
    swap(captured_id& a, captured_id& b) noexcept
    {
        swap(a.id_, b.id_);
    }

 private:
    std::unique_ptr<id_interface> id_;
};
bool
operator==(captured_id const& a, captured_id const& b);
bool
operator!=(captured_id const& a, captured_id const& b);
bool
operator<(captured_id const& a, captured_id const& b);

// ref(id) wraps a reference to an id_interface so that it can be combined.
struct id_ref : id_interface
{
    id_ref() : id_(0), ownership_()
    {
    }

    id_ref(id_interface const& id) : id_(&id), ownership_()
    {
    }

    id_interface*
    clone() const
    {
        id_ref* copy = new id_ref;
        this->deep_copy(copy);
        return copy;
    }

    bool
    equals(id_interface const& other) const
    {
        id_ref const& other_id = static_cast<id_ref const&>(other);
        return *id_ == *other_id.id_;
    }

    bool
    less_than(id_interface const& other) const
    {
        id_ref const& other_id = static_cast<id_ref const&>(other);
        return *id_ < *other_id.id_;
    }

    void
    deep_copy(id_interface* copy) const
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

 private:
    id_interface const* id_;
    std::shared_ptr<id_interface> ownership_;
};
inline id_ref
ref(id_interface const& id)
{
    return id_ref(id);
}

// simple_id<Value> takes a regular type (Value) and implements id_interface
// for it. The type Value must be copyable and comparable for equality and
// ordering (i.e., supply == and < operators).
template<class Value>
struct simple_id : id_interface
{
    simple_id()
    {
    }

    simple_id(Value value) : value_(value)
    {
    }

    Value const&
    value() const
    {
        return value_;
    }

    id_interface*
    clone() const
    {
        return new simple_id(value_);
    }

    bool
    equals(id_interface const& other) const
    {
        simple_id const& other_id = static_cast<simple_id const&>(other);
        return value_ == other_id.value_;
    }

    bool
    less_than(id_interface const& other) const
    {
        simple_id const& other_id = static_cast<simple_id const&>(other);
        return value_ < other_id.value_;
    }

    void
    deep_copy(id_interface* copy) const
    {
        *static_cast<simple_id*>(copy) = *this;
    }

    Value value_;
};

// make_id(value) creates a simple_id with the given value.
template<class Value>
simple_id<Value>
make_id(Value value)
{
    return simple_id<Value>(value);
}

// simple_id_by_reference is like simple_id but takes a pointer to the value.
// The value is only copied if the ID is cloned or deep-copied.
template<class Value>
struct simple_id_by_reference : id_interface
{
    simple_id_by_reference() : value_(0), storage_()
    {
    }

    simple_id_by_reference(Value const* value) : value_(value), storage_()
    {
    }

    id_interface*
    clone() const
    {
        simple_id_by_reference* copy = new simple_id_by_reference;
        this->deep_copy(copy);
        return copy;
    }

    bool
    equals(id_interface const& other) const
    {
        simple_id_by_reference const& other_id
            = static_cast<simple_id_by_reference const&>(other);
        return *value_ == *other_id.value_;
    }

    bool
    less_than(id_interface const& other) const
    {
        simple_id_by_reference const& other_id
            = static_cast<simple_id_by_reference const&>(other);
        return *value_ < *other_id.value_;
    }

    void
    deep_copy(id_interface* copy) const
    {
        auto& typed_copy = *static_cast<simple_id_by_reference*>(copy);
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

 private:
    Value const* value_;
    std::shared_ptr<Value> storage_;
};

// make_id_by_reference(value) creates a simple_id_by_reference for :value.
template<class Value>
simple_id_by_reference<Value>
make_id_by_reference(Value const& value)
{
    return simple_id_by_reference<Value>(&value);
}

// id_pair implements the ID interface for a pair of IDs.
template<class Id0, class Id1>
struct id_pair : id_interface
{
    id_pair()
    {
    }

    id_pair(Id0 id0, Id1 id1) : id0_(std::move(id0)), id1_(std::move(id1))
    {
    }

    id_interface*
    clone() const
    {
        id_pair* copy = new id_pair;
        this->deep_copy(copy);
        return copy;
    }

    bool
    equals(id_interface const& other) const
    {
        id_pair const& other_id = static_cast<id_pair const&>(other);
        return id0_.equals(other_id.id0_) && id1_.equals(other_id.id1_);
    }

    bool
    less_than(id_interface const& other) const
    {
        id_pair const& other_id = static_cast<id_pair const&>(other);
        return id0_.less_than(other_id.id0_)
               || (id0_.equals(other_id.id0_)
                   && id1_.less_than(other_id.id1_));
    }

    void
    deep_copy(id_interface* copy) const
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
auto
combine_ids(Id0 id0, Id1 id1)
{
    return id_pair<Id0, Id1>(std::move(id0), std::move(id1));
}

// Combine more than two IDs into nested pairs.
template<class Id0, class Id1, class... Rest>
auto
combine_ids(Id0 id0, Id1 id1, Rest... rest)
{
    return combine_ids(
        combine_ids(std::move(id0), std::move(id1)), std::move(rest)...);
}

// Allow combine_ids() to take a single argument for variadic purposes.
template<class Id0>
auto
combine_ids(Id0 id0)
{
    return id0;
}

// null_id can be used when you have nothing to identify.
struct null_id_type
{
};
static simple_id<null_id_type*> const null_id(nullptr);

// unit_id can be used when there is only possible identify.
struct unit_id_type
{
};
static simple_id<unit_id_type*> const unit_id(nullptr);

} // namespace alia

#endif
