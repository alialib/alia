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

// the structure we use to store context objects - It provides direct storage of
// the commonly-used objects in the core of alia.

struct context_storage
{
    // directly-stored objects
    system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;
    timing_subsystem* timing = nullptr;

    // generic storage for other objects
    impl::generic_tagged_storage<impl::any_ref> generic;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(context_storage)
};

ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, system_tag, sys)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, event_traversal_tag, event)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, data_traversal_tag, data)
ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(context_storage, timing_tag, timing)

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
        std::enable_if_t<impl::structural_collection_is_convertible<
            OtherContents,
            Contents>::value>* = 0)
        : contents_(other.contents_)
    {
    }

    // assignment operator (from convertible collections)
    template<class OtherContents>
    std::enable_if_t<
        impl::structural_collection_is_convertible<OtherContents, Contents>::
            value,
        context_interface&>
    operator=(context_interface<OtherContents> other)
    {
        contents_ = other.contents_;
        return *this;
    }

    typedef Contents contents_type;

    template<class Tag>
    bool
    has() const
    {
        return impl::has_tagged_data<Tag>(contents_);
    }

    template<class Tag, class Data>
    auto
    add(Data& data)
    {
        auto new_contents
            = impl::add_tagged_data<Tag>(contents_, std::ref(data));
        return context_interface<decltype(new_contents)>(
            std::move(new_contents));
    }

    template<class Tag>
    auto
    remove()
    {
        auto new_contents = impl::remove_tagged_data<Tag>(contents_);
        return context_interface<decltype(new_contents)>(
            std::move(new_contents));
    }

    template<class Tag>
    decltype(auto)
    get()
    {
        return impl::get_tagged_data<Tag>(contents_);
    }

    Contents contents_;
};

template<class Context, class... Tag>
struct add_context_tag
{
    typedef context_interface<
        impl::add_tagged_data_types_t<typename Context::contents_type, Tag...>>
        type;
};

template<class Context, class... Tag>
using add_context_tag_t = typename add_context_tag<Context, Tag...>::type;

template<class Context, class... Tag>
struct remove_context_tag
{
    typedef context_interface<impl::remove_tagged_data_types_t<
        typename Context::contents_type,
        Tag...>>
        type;
};

template<class Context, class... Tag>
using remove_context_tag_t = typename remove_context_tag<Context, Tag...>::type;

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

typedef context_interface<impl::add_tagged_data_types_t<
    impl::empty_structural_collection<context_storage>,
    system_tag,
    event_traversal_tag,
    timing_tag>>
    dataless_context;

typedef add_context_tag_t<dataless_context, data_traversal_tag> context;

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

template<class Tag, class Contents>
decltype(auto)
get(context_interface<Contents> context)
{
    return context.template get<Tag>();
}

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return ctx.template get<event_traversal_tag>();
}

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return ctx.template get<data_traversal_tag>();
}

} // namespace alia

#endif
