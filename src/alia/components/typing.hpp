#ifndef ALIA_COMPONENTS_TYPING_HPP
#define ALIA_COMPONENTS_TYPING_HPP

#include <alia/common.hpp>

#include <type_traits>

// This file provides a means for defining arbitrary collections of components
// that constitute the context in which an application operates. The set of
// components required for an application obviously varies across applications
// (and even across modules within a complex application). The goal here is to
// allow applications to freely mix together components from multiple sources
// (which may not know about one another).
//
// Some additional design considerations follow. Note that these refer to
// contexts rather than component collections, since that is the primary
// motivating example of a component collection.
//
// 1. An application should be able to easily define its own components and mix
// those into its context. This includes application-level state. If there is
// state that is essentially global to the application (e.g., the active user),
// application code should be able to retrieve this from the application
// context. Similarly, a component of the application should be able to extend
// the application's context with state that is specific to that component
// (but ubiquitous within it).
//
// 2. Functions that take contexts as arguments should be able to define the set
// of components that they require as part of the type signature of the context.
// Any caller whose context includes a superset of those components should be
// able to call the function with an implicit conversion of the context
// parameter. This should all be possible without needing to define functions as
// templates (otherwise alia-based applications would end up being entirely
// header-based) and with minimal (ideally zero) runtime overhead in converting
// the caller's context to the type expected by the function.
//
// 3. Retrieving frames/capabilities from a context should require minimal
// (ideally zero) runtime overhead.
//
// 4. Static type checking of context conversions/retrievals should be
// possible but optional. Developers should not have to pay these compile-time
// costs unless it's desired. It should be possible to use a mixed workflow
// where these checks are replaced by runtime checks for iterative development
// but enabled for CI/release builds.
//
// In order to satisfy #4, this file looks for a #define called
// ALIA_DYNAMIC_COMPONENT_CHECKING. If this is set, code related to statically
// checking components is omitted and dynamic checks are substituted where
// appropriate. Note that when ALIA_DYNAMIC_COMPONENT_CHECKING is NOT set,
// ALIA_STATIC_COMPONENT_CHECKING is set and static checks are included.
//
// The statically typed component_collection object is a simple wrapper around
// the dynamically typed storage object. It adds a compile-time type list
// indicating what's actually supposed to be in the collection. This allows
// collections to be converted to other collection types without any run-time
// overhead. This does imply some run-time overhead for retrieving components
// from the collection, but that can be mitigated by providing zero-cost
// retrieval for select (core) components. This also implies that the collection
// object must be passed by value (or const& - though there's no real point in
// that) whereas passing by reference would be more obvious, but that seems
// unavoidable given the requirements.

#ifndef ALIA_DYNAMIC_COMPONENT_CHECKING
#define ALIA_STATIC_COMPONENT_CHECKING
#endif

namespace alia {

#define ALIA_DEFINE_COMPONENT_TYPE(tag, data)                                  \
    struct tag                                                                 \
    {                                                                          \
        typedef data data_type;                                                \
    };

template<class Tags, class Storage>
struct component_collection;

#ifdef ALIA_STATIC_COMPONENT_CHECKING

namespace detail {

// tag_list<Tags...> defines a simple compile-time list of tags. This is held by
// a component_collection to provide compile-time tracking of its contents.
template<class... Tags>
struct tag_list
{
};

// list_contains_tag<List,Tag>::value yields a compile-time boolean indicating
// whether or not the tag_list :List contains :Tag.
template<class List, class Tag>
struct list_contains_tag
{
};
// base case - The list is empty, so the tag isn't in there.
template<class Tag>
struct list_contains_tag<tag_list<>, Tag> : std::false_type
{
};
// case where tag matches
template<class Tag, class... Rest>
struct list_contains_tag<tag_list<Tag, Rest...>, Tag> : std::true_type
{
};
// non-matching (recursive) case
template<class Tag, class OtherTag, class... Rest>
struct list_contains_tag<tag_list<OtherTag, Rest...>, Tag>
    : list_contains_tag<tag_list<Rest...>, Tag>
{
};

// component_collection_contains_tag<Collection,Tag>::value yields a
// compile-time boolean indicating whether or not :Collection contains a
// component with the tag :Tag.
template<class Collection, class Tag>
struct component_collection_contains_tag
{
};
template<class Tags, class Storage, class Tag>
struct component_collection_contains_tag<
    component_collection<Tags, Storage>,
    Tag> : list_contains_tag<Tags, Tag>
{
};

// add_tag_to_list<List,Tag>::type yields the list that results from adding :Tag
// to the head of :List.
//
// Note that this doesn't perform any checks for duplicates.
//
template<class List, class Tag>
struct add_tag_to_list
{
};
template<class Tag, class... Tags>
struct add_tag_to_list<tag_list<Tags...>, Tag>
{
    typedef tag_list<Tag, Tags...> type;
};

// remove_tag_from_list<List,Tag>::type yields the list that results from
// removing the tag matching :Tag from :List.
//
// Note that removing a tag that's not actually in the list is not considered an
// error.
//
template<class List, class Tag>
struct remove_tag_from_list
{
};
// base case (list is empty)
template<class Tag>
struct remove_tag_from_list<tag_list<>, Tag>
{
    typedef tag_list<> type;
};
// case where component matches
template<class Tag, class... Rest>
struct remove_tag_from_list<tag_list<Tag, Rest...>, Tag>
    : remove_tag_from_list<tag_list<Rest...>, Tag>
{
};
// non-matching case
template<class Tag, class OtherTag, class... Rest>
struct remove_tag_from_list<tag_list<OtherTag, Rest...>, Tag>
    : add_tag_to_list<
          typename remove_tag_from_list<tag_list<Rest...>, Tag>::type,
          OtherTag>
{
};

// collection_contains_all_tags<Collection,Tags...>::value yields a compile-time
// boolean indicating whether or not :Collection contains all tags in :Tags.
template<class Collection, class... Tags>
struct collection_contains_all_tags
{
};
// base case - The list of tags to search for is empty, so this is trivially
// true.
template<class Collection>
struct collection_contains_all_tags<Collection> : std::true_type
{
};
// recursive case
template<class Collection, class Tag, class... Rest>
struct collection_contains_all_tags<Collection, Tag, Rest...>
    : std::conditional_t<
          component_collection_contains_tag<Collection, Tag>::value,
          collection_contains_all_tags<Collection, Rest...>,
          std::false_type>
{
};

// merge_tag_lists<A,B>::type yields a list of tags that includes all tags from
// :A and :B (with no duplicates).
template<class A, class B>
struct merge_tag_lists
{
};
// base case (:A is empty)
template<class B>
struct merge_tag_lists<tag_list<>, B>
{
    typedef B type;
};
// recursive case
template<class B, class AHead, class... ARest>
struct merge_tag_lists<tag_list<AHead, ARest...>, B>
    : add_tag_to_list<
          // Ensure that :AHead isn't duplicated. (This may be a noop.)
          typename remove_tag_from_list<
              typename merge_tag_lists<tag_list<ARest...>, B>::type,
              AHead>::type,
          AHead>
{
};

// component_collection_is_convertible<From,To>::value yields a
// compile-time boolean indicating whether or not the type :From can be
// converted to the type :To (both must be component_collections).
// The requirements for this are that a) the storage types are the same and b)
// the component tags of :From are a superset of those of :To.
template<class From, class To>
struct component_collection_is_convertible
{
};
// case where storage types differ
template<class FromTags, class FromStorage, class ToTags, class ToStorage>
struct component_collection_is_convertible<
    component_collection<FromTags, FromStorage>,
    component_collection<ToTags, ToStorage>> : std::false_type
{
};
// case where storage types are the same, so components must be checked
template<class Storage, class FromTags, class... ToTags>
struct component_collection_is_convertible<
    component_collection<FromTags, Storage>,
    component_collection<tag_list<ToTags...>, Storage>>
    : collection_contains_all_tags<
          component_collection<FromTags, Storage>,
          ToTags...>
{
};

} // namespace detail

template<class Tags, class Storage>
struct component_collection
{
    typedef Tags tags;
    typedef Storage storage_type;

    component_collection(Storage* storage) : storage(storage)
    {
    }

    // copy constructor (from convertible collections)
    template<class Other>
    component_collection(
        Other other,
        std::enable_if_t<detail::component_collection_is_convertible<
            Other,
            component_collection>::value>* = 0)
        : storage(other.storage)
    {
    }

    // assignment operator (from convertible collections)
    template<class Other>
    std::enable_if_t<
        detail::component_collection_is_convertible<
            Other,
            component_collection>::value,
        component_collection&>
    operator=(Other other)
    {
        storage = other.storage;
        return *this;
    }

    Storage* storage;
};

// empty_component_collection<Storage> yields a component collection with
// no components and :Storage as its storage type.
template<class Storage>
using empty_component_collection
    = component_collection<detail::tag_list<>, Storage>;

// add_component_type<Collection,Tag>::type gives the type that results
// from extending :Collection with the component defined by :Tag.
template<class Collection, class Tag>
struct add_component_type
{
};
template<class Tag, class Storage, class... Tags>
struct add_component_type<
    component_collection<detail::tag_list<Tags...>, Storage>,
    Tag>
{
    static_assert(
        !detail::list_contains_tag<detail::tag_list<Tags...>, Tag>::value,
        "duplicate component tag");
    typedef component_collection<detail::tag_list<Tag, Tags...>, Storage> type;
};
template<class Collection, class Tag>
using add_component_type_t = typename add_component_type<Collection, Tag>::type;

// remove_component_type<Collection,Tag>::type yields the type that results
// from removing the component associated with :Tag from :Collection.
template<class Collection, class Tag>
struct remove_component_type
{
};
template<class Tag, class Storage, class Tags>
struct remove_component_type<component_collection<Tags, Storage>, Tag>
{
    static_assert(
        detail::list_contains_tag<Tags, Tag>::value,
        "attempting to remove a component tag that doesn't exist");
    typedef component_collection<
        typename detail::remove_tag_from_list<Tags, Tag>::type,
        Storage>
        type;
};
template<class Collection, class Tag>
using remove_component_type_t =
    typename remove_component_type<Collection, Tag>::type;

// merge_components<A,B>::type yields a component collection type that contains
// all the components from :A and :B (but no duplicates).
// Note that the resulting component collection inherits the storage type of :A.
template<class A, class B>
struct merge_components
{
    typedef component_collection<
        typename detail::merge_tag_lists<typename A::tags, typename B::tags>::
            type,
        typename A::storage_type>
        type;
};
template<class A, class B>
using merge_components_t = typename merge_components<A, B>::type;

#else

struct dynamic_tag_list
{
};

template<class Tags, class Storage>
struct component_collection
{
    typedef Tags tags;
    typedef Storage storage_type;

    component_collection(Storage* storage) : storage(storage)
    {
    }

    Storage* storage;
};

// empty_component_collection<Storage> yields a component collection with no
// components and :Storage as its storage type.
template<class Storage>
using empty_component_collection
    = component_collection<dynamic_tag_list, Storage>;

// add_component_type<Collection,Tag>::type gives the type that results from
// extending :Collection with the component defined by :Tag and :Data.
template<class Collection, class Tag>
struct add_component_type
{
    typedef Collection type;
};
template<class Collection, class Tag>
using add_component_type_t = typename add_component_type<Collection, Tag>::type;

// remove_component_type<Collection,Tag>::type yields the type that results from
// removing the component associated with :Tag from :Collection.
template<class Collection, class Tag>
struct remove_component_type
{
    typedef Collection type;
};
template<class Collection, class Tag>
using remove_component_type_t =
    typename remove_component_type<Collection, Tag>::type;

// merge_components<A,B>::type yields a component collection type that contains
// all the components from :A and :B (but no duplicates).
// Note that the resulting component collection inherits the storage type of :A.
template<class A, class B>
struct merge_components
{
    typedef A type;
};
template<class A, class B>
using merge_components_t = typename merge_components<A, B>::type;

#endif

// Extend a collection by adding a new component.
// :Tag is the tag of the component.
// :data is the data associated with the new component.
//
// Note that although this returns a new collection (with the correct type), the
// new collection shares the storage of the original, so this should be used
// with caution.
//
template<class Tag, class Collection>
add_component_type_t<Collection, Tag>
add_component(Collection collection, typename Tag::data_type data)
{
    auto* storage = collection.storage;
    // Add the new data to the storage object.
    storage->template add<Tag>(data);
    // Create a collection with the proper type to reference the storage.
    return add_component_type_t<Collection, Tag>(storage);
}

// Remove a component from a collection.
// :Tag is the tag of the component.
//
// As with add_component(), although this returns a new collection for typing
// purposes, the new collection shares the storage of the original, so use with
// caution.
//
template<class Tag, class Collection>
remove_component_type_t<Collection, Tag>
remove_component(Collection collection)
{
    typename Collection::storage_type* storage = collection.storage;
    // We only actually have to remove the component if we're using dynamic
    // component checking. With static checking, it doesn't matter if the
    // runtime storage includes an extra component. Static checks will prevent
    // its use.
#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING
    // Remove the component from the storage object.
    storage->template remove<Tag>();
#endif
    return remove_component_type_t<Collection, Tag>(storage);
}

// Remove a component from a collection.
//
// With this version, you supply a new storage object, and the function uses it
// if needed to ensure that the original collection's storage is left untouched.
//
template<class Tag, class Collection, class Storage>
remove_component_type_t<Collection, Tag>
remove_component(Collection collection, Storage* new_storage)
{
#ifdef ALIA_STATIC_COMPONENT_CHECKING
    return remove_component_type_t<Collection, Tag>(collection.storage);
#else
    *new_storage = *collection.storage;
    new_storage->template remove<Tag>();
    return remove_component_type_t<Collection, Tag>(new_storage);
#endif
}

// Determine if a component is in a collection.
// :Tag identifies the component.
template<class Tag, class Collection>
bool
has_component(Collection collection)
{
#ifdef ALIA_STATIC_COMPONENT_CHECKING
    return detail::component_collection_contains_tag<Collection, Tag>::value;
#else
    return collection.storage->template has<Tag>();
#endif
}

#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING

// When using dynamic component checking, this error is thrown when trying to
// retrieve a component that's not actually present in a collection.
template<class Tag>
struct component_not_found : exception
{
    component_not_found()
        : exception(
              std::string("component not found in collection:\n")
              + typeid(Tag).name())
    {
    }
};

#endif

// component_caster should be specialized so that it properly casts from stored
// component values to the expected types.

template<class Stored, class Expected>
struct component_caster
{
};

// If we're stored what's expected, then the cast is trivial.
template<class T>
struct component_caster<T, T>
{
    static T
    apply(T stored)
    {
        return stored;
    }
};
template<class T>
struct component_caster<T&, T>
{
    static T&
    apply(T& stored)
    {
        return stored;
    }
};

// Get a reference to the data associated with a component in a
// collection. :Tag identifies the component. If static checking is
// enabled, this generates a compile-time error if :Tag isn't contained
// in :collection.
template<class Tag, class Collection>
auto
get_component(Collection collection)
{
#ifdef ALIA_STATIC_COMPONENT_CHECKING
    static_assert(
        detail::component_collection_contains_tag<Collection, Tag>::value,
        "component not found in collection");
#else
    if (!has_component<Tag>(collection))
        throw component_not_found<Tag>();
#endif
    return component_caster<
        decltype(collection.storage->template get<Tag>()),
        typename Tag::data_type>::apply(collection.storage
                                            ->template get<Tag>());
}

} // namespace alia

#endif
