#ifndef ALIA_CORE_CONTEXT_INTERFACE_HPP
#define ALIA_CORE_CONTEXT_INTERFACE_HPP

#include <alia/core/context/storage.hpp>
#include <alia/core/signals/basic.hpp>

#include <type_traits>

namespace alia {

struct untyped_system;
ALIA_DEFINE_TAGGED_TYPE(core_system_tag, untyped_system&)

struct data_traversal;
ALIA_DEFINE_TAGGED_TYPE(data_traversal_tag, data_traversal&)

struct event_traversal;
ALIA_DEFINE_TAGGED_TYPE(event_traversal_tag, event_traversal&)

struct timing_subsystem;
ALIA_DEFINE_TAGGED_TYPE(timing_tag, timing_subsystem&)

// the structure we use to store the core context objects - It provides direct
// storage of the commonly-used objects in the core of alia.

struct core_context_storage
{
    // directly-stored objects
    untyped_system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;
    timing_subsystem* timing = nullptr;

    // generic storage for other objects
    detail::generic_tagged_storage<std::any> generic;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(core_context_storage)

    // an ID to track changes in the context contents
    id_interface const* content_id = nullptr;
};

#define ALIA_ADD_CORE_CONTEXT_ACCESSORS(storage)                              \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, core_system_tag, sys)         \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, event_traversal_tag, event)   \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, data_traversal_tag, data)     \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, timing_tag, timing)

ALIA_ADD_CORE_CONTEXT_ACCESSORS(core_context_storage)

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

namespace detail {

template<class Contents>
Contents
get_structural_collection(context_interface<Contents> ctx)
{
    return ctx.contents_;
}

template<class Contents>
auto&
get_storage_object(context_interface<Contents> ctx)
{
    return *get_structural_collection(ctx).storage;
}

template<class Tag, class Contents, class Object>
auto
add_context_object(context_interface<Contents> ctx, Object object)
{
    auto new_contents = detail::add_tagged_data<Tag>(
        get_structural_collection(ctx), std::move(object));
    return context_interface<decltype(new_contents)>(std::move(new_contents));
}

template<class Tag, class Contents>
bool
has_context_object(context_interface<Contents> ctx)
{
    return detail::has_tagged_data<Tag>(get_structural_collection(ctx));
}

template<class Tag, class Contents>
auto
remove_context_object(context_interface<Contents> ctx)
{
    auto new_contents
        = detail::remove_tagged_data<Tag>(get_structural_collection(ctx));
    return context_interface<decltype(new_contents)>(std::move(new_contents));
}

} // namespace detail

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

template<class Context, class Tag>
struct context_has_tag : detail::structural_collection_contains_tag<
                             typename Context::contents_type,
                             Tag>
{
};

template<class Contents>
auto
make_context(Contents contents)
{
    return context_interface<Contents>(std::move(contents));
}

template<class System, class Storage>
auto
make_context(
    Storage* storage,
    System& sys,
    event_traversal& event,
    data_traversal& data,
    timing_subsystem& timing)
{
    storage->content_id = &unit_id;
    return detail::add_context_object<data_traversal_tag>(
        detail::add_context_object<timing_tag>(
            detail::add_context_object<event_traversal_tag>(
                detail::add_context_object<core_system_tag>(
                    make_context(
                        detail::make_empty_structural_collection(storage)),
                    std::ref(sys)),
                std::ref(event)),
            std::ref(timing)),
        std::ref(data));
}

template<class Context>
Context
copy_context(Context ctx)
{
    typename Context::contents_type::storage_type* new_storage;
    get_data(ctx, &new_storage);

    *new_storage = *ctx.contents_.storage;

    return Context(typename Context::contents_type(new_storage));
}

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

template<class Storage>
using dataless_context_type_for_storage_t
    = context_interface<detail::add_tagged_data_types_t<
        detail::empty_structural_collection<Storage>,
        core_system_tag,
        event_traversal_tag,
        timing_tag>>;

template<class Storage>
using context_type_for_storage_t = extend_context_type_t<
    dataless_context_type_for_storage_t<Storage>,
    data_traversal_tag>;

using dataless_core_context
    = dataless_context_type_for_storage_t<core_context_storage>;

using core_context = context_type_for_storage_t<core_context_storage>;

// And various small functions for working with contexts...

namespace detail {

template<class Object, class = std::void_t<>>
struct has_value_id : std::false_type
{
};
template<class Object>
struct has_value_id<
    Object,
    std::void_t<decltype(static_cast<id_interface const&>(
        std::declval<Object>().value_id()))>> : std::true_type
{
};

} // namespace detail

template<class Object>
std::enable_if_t<detail::has_value_id<Object>::value, id_interface const&>
get_alia_value_id(Object const& object)
{
    return object.value_id();
}

namespace detail {

template<class Object, class = std::void_t<>>
struct has_alia_value_id : std::false_type
{
};
template<class Object>
struct has_alia_value_id<
    Object,
    std::void_t<decltype(static_cast<id_interface const&>(
        get_alia_value_id(std::declval<Object>())))>> : std::true_type
{
};

void
fold_in_content_id(core_context ctx, id_interface const& id);

template<class Object>
std::enable_if_t<has_alia_value_id<Object>::value>
fold_in_object_id(core_context ctx, Object const& object)
{
    fold_in_content_id(ctx, get_alia_value_id(object));
}

template<class Object>
std::enable_if_t<!has_value_id<Object>::value>
fold_in_object_id(core_context, Object const&)
{
}

} // namespace detail

template<
    class Tag,
    class Contents,
    std::enable_if_t<std::is_reference_v<typename Tag::data_type>, int> = 0>
auto
extend_context(context_interface<Contents> ctx, typename Tag::data_type object)
{
    auto extended_ctx = detail::add_context_object<Tag>(
        copy_context(ctx),
        std::ref<std::remove_reference_t<typename Tag::data_type>>(object));
    fold_in_object_id(extended_ctx, object);
    return extended_ctx;
}
template<
    class Tag,
    class Contents,
    std::enable_if_t<!std::is_reference_v<typename Tag::data_type>, int> = 0>
auto
extend_context(context_interface<Contents> ctx, typename Tag::data_type object)
{
    auto extended_ctx
        = detail::add_context_object<Tag>(copy_context(ctx), object);
    fold_in_object_id(extended_ctx, object);
    return extended_ctx;
}

template<class Tag, class Context, class Content>
void
with_extended_context(
    Context ctx, typename Tag::data_type object, Content&& content)
{
    std::forward<Content>(content)(extend_context<Tag>(ctx, object));
}

template<class Tag, class Contents>
decltype(auto)
get(context_interface<Contents> ctx)
{
    return detail::get_tagged_data<Tag>(ctx.contents_);
}

template<class Contents>
void
recalculate_content_id(context_interface<Contents> ctx)
{
    detail::get_storage_object(ctx).content_id = &unit_id;
    alia::fold_over_collection(
        ctx.contents_,
        [](auto, auto& object, auto ctx) {
            fold_in_object_id(ctx, object);
            return ctx;
        },
        ctx);
}

// convenience accessors...

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return get<event_traversal_tag>(ctx);
}

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return get<data_traversal_tag>(ctx);
}

inline id_interface const&
get_content_id(core_context ctx)
{
    return *ctx.contents_.storage->content_id;
}

// optional_context is basically just std::optional for contexts.
// (Its existence may or may not be justified now that alia requires C++17.)
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

} // namespace alia

#endif
