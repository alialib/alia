#ifndef ALIA_VERSIONING_HPP
#define ALIA_VERSIONING_HPP

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

// alia considers the UI to be potentially 100% dynamic from frame to frame,
// but in reality, it is more like 1% dynamic, and redoing the 99% of work that
// doesn't need to be redone would unacceptably reduce the responsiveness of
// reasonably complex UIs.  Thus, alia must cache previously computed UI
// display data.  The functions for doing this are defined in data.hpp.

// The key to this cache is the data supplied by the user.  If the data is
// small, a copy of the data can be stored and tested each frame for equality
// with the new data value.  However, for large data, even this comparison
// can be prohibitively expensive.  Instead, we need a smaller, stand-in value
// that uniquely identifies the full value of the data.  For example, if the
// data is immutable, it is uniquely identified by its ID (and the context).
// This stand-in value is referred to as a value ID.

// There is already a framework for working with IDs defined in id.hpp, so we
// reuse that framework for value IDs as well.  Unfortunately, the requirements
// in id.hpp are a bit more strict than we need (they require that the ID
// type defines a less than operator), but this is not a big problem, and it
// allows for the possibility of caching data in maps.

// This file provides a mechanism for cheaply obtaining value IDs for a mutable
// object.  It associates a version number with the object and increments that
// number whenever a change is made.

namespace alia {

// The value ID associated with a versioned object must hold both a pointer
// to the underlying version number (to ensure that we're still talking about
// the same object) and the last observed version number (to detect changes).
// The use of a smart pointer ensures that the version number storage isn't
// accidentally reused by a different object as long as there are still
// observers of the original object.

struct version_tag
{
    boost::int64_t version;
};

typedef boost::shared_ptr<version_tag> version_tag_ptr;

struct version_id
{
    version_tag_ptr tag;
    boost::int64_t version;
};
static inline bool operator==(version_id const& a, version_id const& b)
{ return a.tag == b.tag && a.version == b.version; }
static inline bool operator!=(version_id const& a, version_id const& b)
{ return !(a == b); }
static inline bool operator<(version_id const& a, version_id const& b)
{ return a.tag < b.tag || a.tag == b.tag && a.version < b.version; }

template<class Object>
class versioned
{
 public:
    versioned()
    {
        init_tag();
    }
    versioned(Object const& obj)
      : obj_(obj)
    {
        init_tag();
    }
    versioned(versioned const& other)
      : obj_(other.obj_)
    {
        init_tag();
    }

    versioned& operator=(versioned const& other)
    {
        obj_ = other.get();
        inc_version();
        return *this;
    }

    versioned& operator=(Object const& obj)
    {
        obj_ = obj;
        inc_version();
        return *this;
    }

    version_id value_id()
    {
        version_id id;
        id.tag = tag_;
        id.version = tag_->version;
        return id;
    }

    Object const& get() const { return obj_; }

    // This is provided to allow for efficient partial updates to the object.
    // Note, however, that this is somewhat dangerous to use.
    // If you use it, you must ensure that you stop using the returned
    // reference before any observers might check for changes in the object.
    Object& nonconst_get()
    {
        inc_version();
        return obj_;
    }

 private:
    void init_tag()
    {
        tag_.reset(new version_tag);
        tag_->version = 0;
    }

    void inc_version()
    {
        ++tag_->version;
    }

    version_tag_ptr tag_;
    Object obj_;
};

}

#endif
