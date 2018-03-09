#ifndef ALIA_CONTEXTS_HPP
#define ALIA_CONTEXTS_HPP

#include <alia/common.hpp>

#include <type_traits>
#include <typeindex>
#include <unordered_map>

namespace alia {

template<class Tag, class Data>
struct context_component
{
};

struct context_storage
{
    std::unordered_map<std::type_index, any> data;
};

template<class... Components>
struct context;

// tag_in_component_list<Tag,Components...>::value yields a compile-time boolean
// indicating whether or not :Components contains a component with a tag
// matching :Tag.
template<class Tag, class... Components>
struct tag_in_component_list
{
};
// base case (list is empty, so :Tag not found)
template<class Tag>
struct tag_in_component_list<Tag> : std::false_type
{
};
// case where tag matches
template<class Tag, class Data, class... Rest>
struct tag_in_component_list<Tag, context_component<Tag, Data>, Rest...>
    : std::true_type
{
};
// non-matching (recursive) case
template<class Tag, class OtherTag, class Data, class... Rest>
struct tag_in_component_list<Tag, context_component<OtherTag, Data>, Rest...>
    : tag_in_component_list<Tag, Rest...>
{
};

// context_contains_tag<Context,Tag>::value yields a compile-time boolean
// indicating whether or not :Context contains an entry with a tag matching
// :Tag.
template<class Context, class Tag>
struct context_contains_tag
{
};
template<class Tag, class... Components>
struct context_contains_tag<context<Components...>, Tag>
    : tag_in_component_list<Tag, Components...>
{
};

// component_in_list<Component,Components...>::value yields a compile-time
// boolean indicating whether or not :Component appears in :Components.
template<class Component, class... Components>
struct component_in_list
{
};
// base case (list is empty, so :Tag not found)
template<class Component>
struct component_in_list<Component> : std::false_type
{
};
// case where component matches
template<class Component, class... Rest>
struct component_in_list<Component, Component, Rest...> : std::true_type
{
};
// non-matching (recursive) case
template<class Component, class OtherComponent, class... Rest>
struct component_in_list<Component, OtherComponent, Rest...>
    : component_in_list<Component, Rest...>
{
};

// context_contains_component<Context,Component>::value yields a
// compile-time boolean indicating whether or not :Context contains :Component.
template<class Context, class Component>
struct context_contains_component
{
};
template<class Component, class... Components>
struct context_contains_component<context<Components...>, Component>
    : component_in_list<Component, Components...>
{
};

// context_contains_all_components<Context,Components...>::value yields a
// compile-time boolean indicating whether or not :Context contains all
// components in :Components.
template<class Context, class... Components>
struct context_contains_all_components
{
};
// base case (list is empty)
template<class Context>
struct context_contains_all_components<Context> : std::true_type
{
};
// recursive case
template<class Context, class Component, class... Rest>
struct context_contains_all_components<Context, Component, Rest...>
    : std::conditional_t<
          context_contains_component<Context, Component>::value,
          context_contains_all_components<Context, Rest...>,
          std::false_type>
{
};

// context_is_convertible<From,To>::value yields a
// compile-time boolean indicating whether or not the context type :From can be
// converted to the type :To.
template<class From, class To>
struct context_is_convertible
{
};
template<class From, class... ToComponents>
struct context_is_convertible<From, context<ToComponents...>>
    : context_contains_all_components<From, ToComponents...>
{
};

template<class... Components>
struct context
{
    context()
    {
    }

    template<class Other>
    context(
        Other other,
        std::enable_if_t<context_is_convertible<Other, context>::value>* = 0)
        : storage_(other.storage_)
    {
    }

    context_storage* storage_;
};

// Extend a context by adding a new component.
// :Tag is the tag of the component.
// :data is the data associated with the new component.
//
// template<class Tag, class Data, class... Components>
// auto
// extend_context(std::tuple<Components...>& original_context, Data&& data)
// {
//     return std::tuple_cat(
//         std::make_tuple(context_entry<Tag, Data>{std::move(data)}),
//         original_context);
// }

// extended_context_type<Context,Tag,Data> gives the result of extending
// :Context with the component defined by :Tag and :Data.
//
// namespace impl {
// template<class Context, class Tag, class Data>
// struct extended_context_type
// {
// };
// template<class Tag, class Data, class... Components>
// struct extended_context_type<std::tuple<Entries...>, Tag, Data>
// {
//     static_assert(
//         !entry_list_contains_tag<Entries..., Tag>, "duplicate context tag");
//     typedef typename std::tuple<context_entry<Tag, Data>, Entries...> type;
// };
// template<class Context, class Tag, class Data>
// using extended_context_type
//     = extended_context_type_impl<Context, Tag, Data>::type;

// Remove an entry from a context.
// :Tag is the Tag to remove.
//
// template<class Tag, class... Entries>
// auto
// trim_context(std::tuple<Entries...>& original_context)
// {
//     return std::tuple_cat(
//         std::make_tuple(context_entry<Tag, Data>{std::move(data)}),
//         original_context);
// }

// Get a reference to the data associated with :Tag in :ctx.
// This generates a compile-time error if :Tag isn't contained in :ctx.
//
// template<class Tag, class... Entries>
// auto&
// get_context_data(std::tuple<Entries...>& ctx)
// {
//     return std::get<context_tag_lookup<Tag, Frames...>::index>(ctx).data;
// }

} // namespace alia

#endif
