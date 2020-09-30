#ifndef ALIA_CONTEXT_INTERFACE_HPP
#define ALIA_CONTEXT_INTERFACE_HPP

#include <alia/context/storage.hpp>
#include <alia/signals/basic.hpp>

#include <type_traits>

namespace alia {

struct data_traversal;
ALIA_DEFINE_TAGGED_TYPE(data_traversal_tag, data_traversal&)

struct event_traversal;
ALIA_DEFINE_TAGGED_TYPE(event_traversal_tag, event_traversal&)

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

struct timing_subsystem;
ALIA_DEFINE_TAGGED_TYPE(timing_tag, timing_subsystem&)

// the structure we use to store context objects - It provides direct storage
// of the commonly-used objects in the core of alia.

struct context_storage
{
    // directly-stored objects
    system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;
    timing_subsystem* timing = nullptr;

    // generic storage for other objects
    detail::generic_tagged_storage<detail::any_ref> generic;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(context_storage)

    // an ID to track changes in the context contents
    id_interface const* content_id = nullptr;
};

ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, system_tag, sys)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, event_traversal_tag, event)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, data_traversal_tag, data)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, timing_tag, timing)

// Context objects implement this interface if they provide state that might
// influence the component-level application code.
struct stateful_context_object
{
    virtual id_interface const&
    value_id()
        = 0;
};

// the context interface wrapper
template<class Contents>
struct context_interface
{
    context_interface(Contents contents) : contents_(std::move(contents))
    {
    }

    // copy constructor (from convertible collections)
    template<class OtherContents>
    context_interface(
        context_interface<OtherContents> other,
        std::enable_if_t<detail::structural_collection_is_convertible<
            OtherContents,
            Contents>::value>* = 0)
        : contents_(other.contents_)
    {
    }

    // assignment operator (from convertible collections)
    template<class OtherContents>
    std::enable_if_t<
        detail::structural_collection_is_convertible<OtherContents, Contents>::
            value,
        context_interface&>
    operator=(context_interface<OtherContents> other)
    {
        contents_ = other.contents_;
        return *this;
    }

    typedef Contents contents_type;

    Contents contents_;
};

// manipulation of context objects...

template<class Tag, class Contents>
bool
has_object(context_interface<Contents> ctx)
{
    return detail::has_tagged_data<Tag>(ctx.contents_);
}

template<class Tag, class Contents, class Data>
auto
add_object(context_interface<Contents> ctx, Data& data)
{
    auto new_contents
        = detail::add_tagged_data<Tag>(ctx.contents_, std::ref(data));
    return context_interface<decltype(new_contents)>(std::move(new_contents));
}

template<class Tag, class Contents>
auto
remove_object(context_interface<Contents> ctx)
{
    auto new_contents = detail::remove_tagged_data<Tag>(ctx.contents_);
    return context_interface<decltype(new_contents)>(std::move(new_contents));
}

template<class Tag, class Contents>
decltype(auto)
get_object(context_interface<Contents> ctx)
{
    return detail::get_tagged_data<Tag>(ctx.contents_);
}

template<class Context, class... Tag>
struct extend_context_type
{
    typedef context_interface<detail::add_tagged_data_types_t<
        typename Context::contents_type,
        Tag...>>
        type;
};

template<class Context, class... Tag>
using extend_context_type_t =
    typename extend_context_type<Context, Tag...>::type;

template<class Context, class Tag>
struct remove_context_tag
{
    typedef context_interface<detail::remove_tagged_data_type_t<
        typename Context::contents_type,
        Tag>>
        type;
};

template<class Context, class Tag>
using remove_context_tag_t = typename remove_context_tag<Context, Tag>::type;

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

typedef context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<context_storage>,
    system_tag,
    event_traversal_tag,
    timing_tag>>
    dataless_context;

typedef extend_context_type_t<dataless_context, data_traversal_tag> context;

// And some convenience functions...

template<class Contents>
auto
make_context(Contents contents)
{
    return context_interface<Contents>(std::move(contents));
}

context
make_context(
    context_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data,
    timing_subsystem& timing);

template<class Context>
Context
copy_context(Context ctx)
{
    context_storage* new_storage;
    get_data(ctx, &new_storage);

    *new_storage = *ctx.contents_.storage;

    return Context(typename Context::contents_type(new_storage));
}

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return get_object<event_traversal_tag>(ctx);
}

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return get_object<data_traversal_tag>(ctx);
}

inline id_interface const&
get_content_id(context ctx)
{
    return *ctx.contents_.storage->content_id;
}

// optional_context is basically just std::optional for contexts.
// (alia supports C++14, which is why we don't use std::optional directly.)
template<class Context>
struct optional_context
{
    optional_context() : ctx_(typename Context::contents_type(nullptr))
    {
    }

    bool
    has_value() const
    {
        return ctx_.contents_.storage != nullptr;
    }

    explicit operator bool() const
    {
        return has_value();
    }

    Context const
    operator*() const
    {
        assert(has_value());
        return ctx_;
    }
    Context
    operator*()
    {
        assert(has_value());
        return ctx_;
    }

    Context const*
    operator->() const
    {
        assert(has_value());
        return &ctx_;
    }
    Context*
    operator->()
    {
        assert(has_value());
        return &ctx_;
    }

    void
    reset(Context ctx)
    {
        ctx_ = ctx;
    }

    void
    reset()
    {
        ctx_ = Context(typename Context::contents_type(nullptr));
    }

 private:
    Context ctx_;
};

// scoped_context_content_id is used to fold another ID into the overall ID for
// a context's content.
struct scoped_context_content_id
{
    scoped_context_content_id()
    {
    }
    scoped_context_content_id(context ctx, id_interface const& id)
    {
        begin(ctx, id);
    }
    ~scoped_context_content_id()
    {
        end();
    }

    void
    begin(context ctx, id_interface const& id);

    void
    end();

 private:
    optional_context<dataless_context> ctx_;
    simple_id<unsigned> this_id_;
    id_interface const* parent_id_;
};

} // namespace alia

#endif
