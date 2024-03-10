#ifndef ALIA_CORE_CONTEXT_STRUCTURAL_TYPING_HPP
#define ALIA_CORE_CONTEXT_STRUCTURAL_TYPING_HPP

#include <alia/core/common.hpp>

#include <type_traits>

// This file provides the underlying type mechanics that allow for defining the
// context of an application as an arbitrary collection of data from disparate
// sources (the alia core, a UI library adaptor, services that the application
// is dependent on, higher-level parts of the application, etc.). This set of
// data obviously varies across applications (and even across modules within a
// complex application). The goal here is to allow applications to freely mix
// together data/objects from multiple sources (which may not know about one
// another).
//
// Some additional design considerations follow.
//
// 1. An application should be able to easily define its own data types and mix
//    those into its context. This includes application-level state. If there
//    is state that is essentially global to the application (e.g., the active
//    user), application code should be able to retrieve this from the
//    application context. Similarly, a component of the application should be
//    able to extend the application's context with state that is specific to
//    that component (but ubiquitous within it).
//
// 2. Functions that take contexts as arguments should be able to define the
//    set of context elements that they require as part of the type signature
//    of the context. (Context elements would be identified by compile-time
//    type tags.) Any caller whose context includes a superset of those tags
//    should be able to call the function with an implicit conversion of the
//    context parameter. This should all be possible without needing to define
//    functions as templates (otherwise alia-based applications would end up
//    being entirely header-based) and with minimal (ideally zero) runtime
//    overhead in converting the caller's context to the type expected by the
//    function.
//
// 3. Retrieving frames/capabilities from a context should require minimal
//    (ideally zero) runtime overhead.
//
// The statically typed structural_collection object is a simple wrapper around
// the dynamically typed storage object. It adds a compile-time type list
// indicating what's actually supposed to be in the collection. This allows
// collections to be converted to other collection types without any run-time
// overhead. This does imply some run-time overhead for retrieving data from
// the collection, but that can be mitigated by providing zero-cost retrieval
// for select (core) data. This also implies that the collection object must be
// passed by value (or const& - though there's no real point in that) whereas
// passing by reference would be more obvious, but that seems unavoidable given
// the requirements.

namespace alia {

namespace detail {

#define ALIA_DEFINE_TAGGED_TYPE(tag, data)                                    \
    struct tag                                                                \
    {                                                                         \
        typedef data data_type;                                               \
    };

template<class Tags, class Storage>
struct structural_collection;

// tag_list<Tags...> defines a simple compile-time list of tags. This is held
// by a structural_collection to provide compile-time tracking of its contents.
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

// structural_collection_contains_tag<Collection,Tag>::value yields a
// compile-time boolean indicating whether or not :Collection contains an item
// with the tag :Tag.
template<class Collection, class Tag>
struct structural_collection_contains_tag
{
};
template<class Tags, class Storage, class Tag>
struct structural_collection_contains_tag<
    structural_collection<Tags, Storage>,
    Tag> : list_contains_tag<Tags, Tag>
{
};

// add_tag_to_list<List,Tag>::type yields the list that results from adding
// :Tag to the head of :List.
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
// Note that removing a tag that's not actually in the list is not considered
// an error.
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
// case where an item matches
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

// collection_contains_all_tags<Collection,Tags...>::value yields a
// compile-time boolean indicating whether or not :Collection contains all tags
// in :Tags.
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
          structural_collection_contains_tag<Collection, Tag>::value,
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

// structural_collection_is_convertible<From,To>::value yields a
// compile-time boolean indicating whether or not the type :From can be
// converted to the type :To (both must be structural_collections).
//
// The requirements for this are that:
// a) :From::storage_type* is convertible to :To::storage_type*
// and
// b) the tags of :From are a superset of those of :To.
//
template<class From, class To>
struct structural_collection_is_convertible
{
};
template<class FromTags, class FromStorage, class ToStorage, class... ToTags>
struct structural_collection_is_convertible<
    structural_collection<FromTags, FromStorage>,
    structural_collection<tag_list<ToTags...>, ToStorage>>
    : std::conditional_t<
          std::is_convertible_v<FromStorage*, ToStorage*>,
          collection_contains_all_tags<
              structural_collection<FromTags, FromStorage>,
              ToTags...>,
          std::false_type>
{
};

template<class Tags, class Storage>
struct structural_collection
{
    typedef Tags tags;
    typedef Storage storage_type;

    structural_collection(Storage* storage) : storage(storage)
    {
    }

    // copy constructor (from convertible collections)
    template<class Other>
    structural_collection(
        Other other,
        std::enable_if_t<structural_collection_is_convertible<
            Other,
            structural_collection>::value>* = 0)
        : storage(other.storage)
    {
    }

    // assignment operator (from convertible collections)
    template<class Other>
    std::enable_if_t<
        structural_collection_is_convertible<Other, structural_collection>::
            value,
        structural_collection&>
    operator=(Other other)
    {
        storage = other.storage;
        return *this;
    }

    Storage* storage;
};

// empty_structural_collection<Storage> yields a structural collection with
// no data and :Storage as its storage type.
template<class Storage>
using empty_structural_collection = structural_collection<tag_list<>, Storage>;

// add_tagged_data_type<Collection,Tag>::type gives the type that results
// from extending :Collection with the data type defined by :Tag.
template<class Collection, class Tag>
struct add_tagged_data_type
{
};
template<class Tag, class Storage, class... Tags>
struct add_tagged_data_type<
    structural_collection<tag_list<Tags...>, Storage>,
    Tag>
{
    static_assert(
        !list_contains_tag<tag_list<Tags...>, Tag>::value,
        "duplicate context tag");
    typedef structural_collection<tag_list<Tag, Tags...>, Storage> type;
};
template<class Collection, class Tag>
using add_tagged_data_type_t =
    typename add_tagged_data_type<Collection, Tag>::type;

// add_tagged_data_types<Collection,Tag...>::type gives the type that results
// from extending :Collection with the data types defined by the given list of
// tags.
template<class Collection, class... Tag>
struct add_tagged_data_types
{
};
template<class Collection>
struct add_tagged_data_types<Collection>
{
    typedef Collection type;
};
template<class Collection, class Tag, class... Rest>
struct add_tagged_data_types<Collection, Tag, Rest...>
    : add_tagged_data_types<
          typename add_tagged_data_type<Collection, Tag>::type,
          Rest...>
{
};
template<class Collection, class... Tag>
using add_tagged_data_types_t =
    typename add_tagged_data_types<Collection, Tag...>::type;

// remove_tagged_data_type<Collection,Tag>::type yields the type that results
// from removing the data type associated with :Tag from :Collection.
template<class Collection, class Tag>
struct remove_tagged_data_type
{
};
template<class Tag, class Storage, class Tags>
struct remove_tagged_data_type<structural_collection<Tags, Storage>, Tag>
{
    // Note that it's considered OK to remove a tag that's not actually in the
    // collection.
    typedef structural_collection<
        typename remove_tag_from_list<Tags, Tag>::type,
        Storage>
        type;
};
template<class Collection, class Tag>
using remove_tagged_data_type_t =
    typename remove_tagged_data_type<Collection, Tag>::type;

// merge_structural_collections<A,B>::type yields a structural collection type
// that contains all the tags from :A and :B (but no duplicates). Note that the
// resulting collection inherits the storage type of :A.
template<class A, class B>
struct merge_structural_collections
{
    typedef structural_collection<
        typename merge_tag_lists<typename A::tags, typename B::tags>::type,
        typename A::storage_type>
        type;
};
template<class A, class B>
using merge_structural_collections_t =
    typename merge_structural_collections<A, B>::type;

// Make an empty structural collection for the given storage object.
template<class Storage>
empty_structural_collection<Storage>
make_empty_structural_collection(Storage* storage)
{
    return empty_structural_collection<Storage>(storage);
}

// Extend a collection by adding a new data object.
// :Tag is the tag of the new data.
// :data is the data.
//
// Note that although this returns a new collection (with the correct type),
// the new collection shares the storage of the original, so this should be
// used with caution.
//
template<class Tag, class Collection, class Data>
add_tagged_data_type_t<Collection, Tag>
add_tagged_data(Collection collection, Data&& data)
{
    auto* storage = collection.storage;
    // Add the new data to the storage object.
    storage->template add<Tag>(std::forward<Data&&>(data));
    // Create a collection with the proper type to reference the storage.
    return add_tagged_data_type_t<Collection, Tag>(storage);
}

// Remove an item from a collection.
// :Tag is the tag of the item.
//
// As with add_tagged_data(), although this returns a new collection for typing
// purposes, the new collection shares the storage of the original, so use with
// caution.
//
template<class Tag, class Collection>
remove_tagged_data_type_t<Collection, Tag>
remove_tagged_data(Collection collection)
{
    return remove_tagged_data_type_t<Collection, Tag>(collection.storage);
}

// Determine if a tag is in a collection.
template<class Tag, class Collection>
bool
has_tagged_data(Collection)
{
    return structural_collection_contains_tag<Collection, Tag>::value;
}

// tagged_data_caster should be specialized so that it properly casts from
// stored data to the expected types.

template<class Stored, class Expected>
struct tagged_data_caster
{
};

// If we've stored what's expected, then the cast is trivial.
template<class T>
struct tagged_data_caster<T, T>
{
    static T
    apply(T stored)
    {
        return stored;
    }
};
template<class T>
struct tagged_data_caster<T&, T>
{
    static T&
    apply(T& stored)
    {
        return stored;
    }
};

// Get a reference to the data associated with a tag in a collection. If static
// checking is enabled, this generates a compile-time error if :Tag isn't
// contained in :collection.
template<class Tag, class Collection>
decltype(auto)
get_tagged_data(Collection collection)
{
    static_assert(
        structural_collection_contains_tag<Collection, Tag>::value,
        "tag not found in context");
    return tagged_data_caster<
        decltype(collection.storage->template get<Tag>()),
        typename Tag::data_type>::apply(collection.storage
                                            ->template get<Tag>());
}

} // namespace detail

// fold_over_collection(collection, f, z) performs a functional fold over the
// objects in a structural collection, invoking f as f(tag, data, z) for each
// object (and accumulating in z).

template<class Collection, class Function, class Initial>
auto
fold_over_collection(Collection collection, Function f, Initial z);

namespace detail {

// collection_folder is a helper struct for doing compile-time component
// collection folding...
template<class Collection>
struct collection_folder
{
};
// the empty case
template<class Storage>
struct collection_folder<structural_collection<tag_list<>, Storage>>
{
    template<class Collection, class Function, class Initial>
    static auto
    apply(Collection, Function, Initial z)
    {
        return z;
    }
};
// the recursive case
template<class Storage, class Tag, class... Rest>
struct collection_folder<
    structural_collection<tag_list<Tag, Rest...>, Storage>>
{
    template<class Collection, class Function, class Initial>
    static auto
    apply(Collection collection, Function f, Initial z)
    {
        return f(
            Tag(),
            get_tagged_data<Tag>(collection),
            fold_over_collection(
                structural_collection<detail::tag_list<Rest...>, Storage>(
                    collection.storage),
                f,
                z));
    }
};

} // namespace detail

template<class Collection, class Function, class Initial>
auto
fold_over_collection(Collection collection, Function f, Initial z)
{
    return detail::collection_folder<Collection>::apply(collection, f, z);
}

} // namespace alia

#endif
