#ifndef ALIA_COMPONENT_COLLECTION_HPP
#define ALIA_COMPONENT_COLLECTION_HPP

#include <alia/common.hpp>

#include <type_traits>
#include <typeindex>
#include <unordered_map>

// This file provides a means for defining arbitrary collections of components
// which constitute the context in which an application operates. The set of
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
// context. Similarly, a component of the application may decide to extend
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

template<class Components, class Storage>
struct component_collection;

#ifdef ALIA_STATIC_COMPONENT_CHECKING

namespace detail {

// A component has a tag type and a data type associated with it. The tag
// type is used only at compile time to identify the component while the data
// type actually stores any run-time data associated with the component.
template<class Tag, class Data>
struct component
{
    typedef Tag tag;
    typedef Data data_type;
};

// component_list<Components...> defines a simple compile-time list of
// components. This is held by a component_collection to provide compile-time
// tracking of its contents.
template<class... Components>
struct component_list
{
};

// component_list_contains_tag<Tag,Components>::value yields a compile-time
// boolean indicating whether or not :Components contains a component with a
// tag matching :Tag.
template<class List, class Tag>
struct component_list_contains_tag
{
};
// base case (list is empty, so :Tag not found)
template<class Tag>
struct component_list_contains_tag<component_list<>, Tag> : std::false_type
{
};
// case where tag matches
template<class Tag, class Data, class... Rest>
struct component_list_contains_tag<
    component_list<component<Tag, Data>, Rest...>,
    Tag> : std::true_type
{
};
// non-matching (recursive) case
template<class Tag, class OtherTag, class Data, class... Rest>
struct component_list_contains_tag<
    component_list<component<OtherTag, Data>, Rest...>,
    Tag> : component_list_contains_tag<component_list<Rest...>, Tag>
{
};

// component_collection_contains_tag<Collection,Tag>::value yields a
// compile-time boolean indicating whether or not :Collection contains an entry
// with a tag matching :Tag.
template<class Collection, class Tag>
struct component_collection_contains_tag
{
};
template<class Components, class Storage, class Tag>
struct component_collection_contains_tag<
    component_collection<Components, Storage>,
    Tag> : component_list_contains_tag<Components, Tag>
{
};

// list_contains_component<List,Component>::value yields a compile-time
// boolean indicating whether or not :Component appears in the component_list
// :List.
template<class List, class Component>
struct list_contains_component
{
};
// base case (list is empty, so :Component not found)
template<class Component>
struct list_contains_component<component_list<>, Component> : std::false_type
{
};
// case where component matches
template<class Component, class... Rest>
struct list_contains_component<component_list<Component, Rest...>, Component>
    : std::true_type
{
};
// non-matching (recursive) case
template<class Component, class OtherComponent, class... Rest>
struct list_contains_component<
    component_list<OtherComponent, Rest...>,
    Component> : list_contains_component<component_list<Rest...>, Component>
{
};

// collection_contains_component<Collection,Component>::value yields a
// compile-time boolean indicating whether or not :Collection contains
// :Component.
template<class Collection, class Component>
struct collection_contains_component
{
};
template<class Components, class Storage, class Component>
struct collection_contains_component<
    component_collection<Components, Storage>,
    Component> : list_contains_component<Components, Component>
{
};

// add_component_to_list<List,Component>::type yields the list that results
// from adding :Component to the head of :List.
// Note that this doesn't perform any checks for duplicate tags.
template<class List, class Component>
struct add_component_to_list
{
};
template<class Component, class... Components>
struct add_component_to_list<component_list<Components...>, Component>
{
    typedef component_list<Component, Components...> type;
};

// remove_component_from_list<List,Tag>::type yields the list that results from
// removing the component matching :Tag from :List.
// Note that removing a component that's not actually in the list is not
// considered an error.
template<class List, class Tag>
struct remove_component_from_list
{
};
// base case (list is empty)
template<class Tag>
struct remove_component_from_list<component_list<>, Tag>
{
    typedef component_list<> type;
};
// case where component matches
template<class Tag, class Data, class... Rest>
struct remove_component_from_list<
    component_list<component<Tag, Data>, Rest...>,
    Tag> : remove_component_from_list<component_list<Rest...>, Tag>
{
};
// non-matching case
template<class Tag, class OtherTag, class Data, class... Rest>
struct remove_component_from_list<
    component_list<component<OtherTag, Data>, Rest...>,
    Tag>
    : add_component_to_list<
          typename remove_component_from_list<component_list<Rest...>, Tag>::
              type,
          component<OtherTag, Data>>
{
};

// collection_contains_all_components<Collection,Components...>::value yields a
// compile-time boolean indicating whether or not :Collection contains all
// components in :Components.
template<class Collection, class... Components>
struct collection_contains_all_components
{
};
// base case (list is empty)
template<class Collection>
struct collection_contains_all_components<Collection> : std::true_type
{
};
// recursive case
template<class Collection, class Component, class... Rest>
struct collection_contains_all_components<Collection, Component, Rest...>
    : std::conditional_t<
          collection_contains_component<Collection, Component>::value,
          collection_contains_all_components<Collection, Rest...>,
          std::false_type>
{
};

// merge_component_lists<A,B>::type yields a list of components that includes
// all components from :A and :B (with no duplicates).
template<class A, class B>
struct merge_component_lists
{
};
// base case (:A is empty)
template<class B>
struct merge_component_lists<component_list<>, B>
{
    typedef B type;
};
// recursive case
template<class B, class AHead, class... ARest>
struct merge_component_lists<component_list<AHead, ARest...>, B>
    : add_component_to_list<
          // Ensure that :AHead isn't duplicated. (This may be a noop.)
          typename remove_component_from_list<
              typename merge_component_lists<component_list<ARest...>, B>::type,
              typename AHead::tag>::type,
          AHead>
{
};

// component_collection_is_convertible<From,To>::value yields a
// compile-time boolean indicating whether or not the type :From can be
// converted to the type :To (both must be component_collections).
// The requirements for this are that a) the storage types are the same and b)
// :From has a superset of the components of :To.
template<class From, class To>
struct component_collection_is_convertible
{
};
// case where storage types differ
template<
    class FromComponents,
    class FromStorage,
    class ToComponents,
    class ToStorage>
struct component_collection_is_convertible<
    component_collection<FromComponents, FromStorage>,
    component_collection<ToComponents, ToStorage>> : std::false_type
{
};
// case where storage types are the same, so components must be checked
template<class Storage, class FromComponents, class... ToComponents>
struct component_collection_is_convertible<
    component_collection<FromComponents, Storage>,
    component_collection<component_list<ToComponents...>, Storage>>
    : collection_contains_all_components<
          component_collection<FromComponents, Storage>,
          ToComponents...>
{
};

} // namespace detail

template<class Components, class Storage>
struct component_collection
{
    typedef Components components;
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
    = component_collection<detail::component_list<>, Storage>;

// add_component_type<Collection,Tag,Data>::type gives the type that results
// from extending :Collection with the component defined by :Tag and :Data.
template<class Collection, class Tag, class Data>
struct add_component_type
{
};
template<class Tag, class Data, class Storage, class... Components>
struct add_component_type<
    component_collection<detail::component_list<Components...>, Storage>,
    Tag,
    Data>
{
    static_assert(
        !detail::component_list_contains_tag<
            detail::component_list<Components...>,
            Tag>::value,
        "duplicate component tag");
    typedef component_collection<
        detail::component_list<detail::component<Tag, Data>, Components...>,
        Storage>
        type;
};
template<class Collection, class Tag, class Data>
using add_component_type_t =
    typename add_component_type<Collection, Tag, Data>::type;

// remove_component_type<Collection,Tag,Data>::type yields the type that results
// from removing the component associated with :Tag from :Collection.
template<class Collection, class Tag>
struct remove_component_type
{
};
template<class Tag, class Storage, class Components>
struct remove_component_type<component_collection<Components, Storage>, Tag>
{
    static_assert(
        detail::component_list_contains_tag<Components, Tag>::value,
        "attempting to remove a component tag that doesn't exist");
    typedef component_collection<
        typename detail::remove_component_from_list<Components, Tag>::type,
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
        typename detail::merge_component_lists<
            typename A::components,
            typename B::components>::type,
        typename A::storage_type>
        type;
};
template<class A, class B>
using merge_components_t = typename merge_components<A, B>::type;

#endif

#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING

struct dynamic_component_list
{
};

template<class Components, class Storage>
struct component_collection
{
    typedef Components components;
    typedef Storage storage_type;

    component_collection(Storage* storage) : storage(storage)
    {
    }

    Storage* storage;
};

// empty_component_collection<Storage> yields a component collection with
// no components and :Storage as its storage type.
template<class Storage>
using empty_component_collection
    = component_collection<dynamic_component_list, Storage>;

// add_component_type<Collection,Tag,Data>::type gives the type that results
// from extending :Collection with the component defined by :Tag and :Data.
template<class Collection, class Tag, class Data>
struct add_component_type
{
    typedef Collection type;
};
template<class Collection, class Tag, class Data>
using add_component_type_t =
    typename add_component_type<Collection, Tag, Data>::type;

// remove_component_type<Collection,Tag,Data>::type yields the type that results
// from removing the component associated with :Tag from :Collection.
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
// :new_storage is a pointer to the storage object to use for the extended
// collection. It must outlive the returned collection. (Its contents before
// calling this function don't matter.)
template<class Tag, class Data, class Collection>
add_component_type_t<Collection, Tag, Data>
add_component(
    typename Collection::storage_type* new_storage,
    Collection collection,
    Data data)
{
    // Copy over existing components.
    *new_storage = *collection.storage;
    // Add the new data to the storage object.
    add_component<Tag>(*new_storage, data);
    // Create a collection to reference the new storage object.
    return add_component_type_t<Collection, Tag, Data>(new_storage);
}

// Remove a component from a collection.
// :Tag is the tag of the component.
// :new_storage is a pointer to the storage object to use for the new
// collection. It must outlive the returned collection. (Its contents before
// calling this function don't matter.)
// Note that this function is allowed to reuse the storage object from the
// input collection, so that is also required to outlive the returned
// collection.
template<class Tag, class Collection>
remove_component_type_t<Collection, Tag>
remove_component(
    typename Collection::storage_type* new_storage, Collection collection)
{
#ifdef ALIA_STATIC_COMPONENT_CHECKING
    // Since we're using static checking, it doesn't matter if the runtime
    // storage includes an extra component. Static checks will prevent its use.
    return remove_component_type_t<Collection, Tag>(collection.storage);
#else
    // Copy over existing components.
    *new_storage = *collection.storage;
    // Remove the component from the storage object.
    remove_component<Tag>(*new_storage);
    // Create a collection to reference the new storage object.
    return remove_component_type_t<Collection, Tag>(new_storage);
#endif
}

// Get a reference to the data associated with a component in a collection.
// :Tag identifies the component.
// If static checking is enabled, this generates a compile-time error if :Tag
// isn't contained in :collection.
template<class Tag, class Collection>
auto
get_component(Collection collection)
{
#ifdef ALIA_STATIC_COMPONENT_CHECKING
    static_assert(
        detail::component_collection_contains_tag<Collection, Tag>::value,
        "component not found in collection");
#endif
    return get_component<Tag>(*collection.storage);
}

// generic_component_storage is one possible implementation of the underlying
// container for storing components and their associated data.
// :Data is the type used to store component data.
template<class Data>
struct generic_component_storage
{
    std::unordered_map<std::type_index, Data> components;
};

// The following functions constitute the interface expected of storage objects.

// Does the storage object have a component with the given tag?
template<class Tag, class Data>
bool
has_component(generic_component_storage<Data>& storage)
{
    return storage.components.find(std::type_index(typeid(Tag)))
           != storage.components.end();
}

// Add a component.
template<class Tag, class StorageData, class ComponentData>
void
add_component(
    generic_component_storage<StorageData>& storage, ComponentData&& data)
{
    storage.components[std::type_index(typeid(Tag))]
        = std::forward<ComponentData&&>(data);
}

// Remove a component.
template<class Tag, class Data>
void
remove_component(generic_component_storage<Data>& storage)
{
    storage.components.erase(std::type_index(typeid(Tag)));
}

// Get the data for a component.
template<class Tag, class Data>
Data&
get_component(generic_component_storage<Data>& storage)
{
    return storage.components.at(std::type_index(typeid(Tag)));
}

} // namespace alia

#endif
