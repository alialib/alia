#pragma once

#include <alia/abi/base/allocator.h>
#include <alia/abi/kernel/ids.h>

#include <concepts>
#include <functional>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <utility>

namespace alia {

// This file defines a thin C++ facade over `alia_id_view`.

using id_view = alia_id_view;

inline id_view
null_id()
{
    return alia_id_view_null();
}

inline id_view
unit_id()
{
    return alia_id_view_unit();
}

// value ID type tag for signals with a value that doesn't change over time
struct constant_value_tag
{
};

constexpr bool
operator==(constant_value_tag, constant_value_tag)
{
    return true;
}
constexpr bool
operator!=(constant_value_tag, constant_value_tag)
{
    return false;
}

// `make_id(v)` for various common types - This takes care of selecting a
// compatible `alia_id_view` constructor based on the type.
// Note that this is only defined for types that can be fully stored within
// the `alia_id_view`.

inline id_view
make_id(bool v)
{
    return alia_id_view_make_u32(v ? 1u : 0u);
}

// This covers all integral types (excluding boolean) and selects the
// appropriate ID constructor based on the size and signedness of the type.
template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
make_id(T v)
{
    if constexpr (sizeof(T) <= sizeof(int32_t))
    {
        if constexpr (std::signed_integral<T>)
            return alia_id_view_make_i32(static_cast<int32_t>(v));
        else
            return alia_id_view_make_u32(static_cast<uint32_t>(v));
    }
    else
    {
        if constexpr (std::signed_integral<T>)
            return alia_id_view_make_i64(static_cast<int64_t>(v));
        else
            return alia_id_view_make_u64(static_cast<uint64_t>(v));
    }
}

inline id_view
make_id(float v)
{
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), sizeof(float));
}

inline id_view
make_id(double v)
{
    static_assert(
        sizeof(double) <= ALIA_ID_INLINE_CAPACITY,
        "double must fit in alia_id_view inline payload");
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), sizeof(double));
}

// Construct an ID from a pointer. Use this only for immutable objects where
// address is equivalent to identity, such as string literals.
template<class T>
id_view
make_pointer_id(T const* ptr)
{
    return alia_id_view_make_pointer(static_cast<void const*>(ptr));
}

// `make_id_by_reference(v)` makes an ID for an existing object. The ID may
// borrow memory from `v`, so `v` must outlive any use of the returned
// `id_view`. This is designed to be overloaded for custom types.

inline id_view
make_id_by_reference(bool const& v)
{
    return make_id(v);
}

template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
make_id_by_reference(T const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(float const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(double const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(std::string const& v)
{
    return alia_id_view_make_bytes(v.data(), static_cast<uint32_t>(v.size()));
}

template<class T>
    requires std::is_trivially_copyable_v<T> && (!std::integral<T>)
          && (!std::floating_point<T>)
id_view
make_id_by_reference(T const& v)
{
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), static_cast<uint32_t>(sizeof(T)));
}

// `identifiable<T>` is true iff an ID can be formed from a live `T` via
// `make_id_by_reference`.
template<class T>
concept identifiable = requires(T const& v) {
    { make_id_by_reference(v) } -> std::convertible_to<id_view>;
};

// Combine two IDs into a pair. `storage` must outlive the returned view.
inline id_view
make_id_pair(alia_id_pair& storage, id_view left, id_view right)
{
    return alia_id_view_make_pair(&storage, left, right);
}

// `erased_id_storage<T>` holds any persistent state needed to erase a typed
// value ID of type `T` into an `id_view`. Leaf types need no storage. Pair
// types nest child storage and an `alia_id_pair` node.
template<class T>
struct erased_id_storage
{
};

template<class A, class B>
struct erased_id_storage<std::pair<A, B>>
{
    // nested storage for the left ID
    [[no_unique_address]] erased_id_storage<A> left;
    // nested storage for the right ID
    [[no_unique_address]] erased_id_storage<B> right;
    // pair node backing the erased view
    alia_id_pair pair{};
};

// `to_id_view(storage, id)` converts a typed value ID into a C-compatible
// `id_view`. This is the general form that supports types that require storage
// outside of the id_view.
//
// For types that don't require storage, `to_id_view(id)` is also provided as a
// convenience.

inline id_view
to_id_view(id_view id)
{
    return id;
}

inline id_view
to_id_view(constant_value_tag)
{
    return unit_id();
}

inline id_view
to_id_view(bool v)
{
    return make_id(v);
}

template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
to_id_view(T v)
{
    return make_id(v);
}

inline id_view
to_id_view(float v)
{
    return make_id(v);
}

inline id_view
to_id_view(double v)
{
    return make_id(v);
}

// A `char const*` value ID is erased as a pointer identity. (Using a `char
// const*` as the ID implies that it points to immutable text (e.g., a string
// literal).)
inline id_view
to_id_view(char const* p)
{
    return make_pointer_id(p);
}

template<class T>
    requires identifiable<T> && (!std::same_as<T, id_view>)
          && (!std::same_as<T, constant_value_tag>) && (!std::integral<T>)
          && (!std::floating_point<T>) && (!std::same_as<T, bool>)
          && (!std::is_pointer_v<std::remove_cvref_t<T>>)
id_view
to_id_view(T const& v)
{
    return make_id_by_reference(v);
}

template<class T>
id_view
to_id_view(erased_id_storage<T>&, T const& v)
{
    return to_id_view(v);
}

template<class A, class B>
id_view
to_id_view(
    erased_id_storage<std::pair<A, B>>& storage, std::pair<A, B> const& v)
{
    return make_id_pair(
        storage.pair,
        to_id_view(storage.left, v.first),
        to_id_view(storage.right, v.second));
}

// `typed_value_id<T>` is true iff `T` can be used as a concrete signal value
// ID. The name avoids colliding with the `value_id()` member function.
template<class T>
concept typed_value_id
    = std::copyable<T>
   && requires(erased_id_storage<T>& storage, T const& id) {
          { to_id_view(storage, id) } -> std::convertible_to<id_view>;
      };

// `captured_id<Id>` retains a typed value ID across frames. Value-like IDs
// are stored by copy. `id_view` IDs store an `alia_captured_id` plus an
// optional heap slab (null when the ID fits inline). Deallocation metadata
// for the slab lives in the slab header.

template<class Id>
struct captured_id
{
    bool
    empty() const
    {
        return !valid_;
    }

    void
    clear(alia_general_allocator* = nullptr)
    {
        valid_ = false;
        id_ = Id{};
    }

    void
    capture(Id const& id, alia_general_allocator* = nullptr)
    {
        id_ = id;
        valid_ = true;
    }

    bool
    matches(Id const& id) const
    {
        return valid_ && id_ == id;
    }

 private:
    bool valid_ = false;
    Id id_{};
};

template<>
struct captured_id<id_view>
{
    captured_id() = default;

    captured_id(captured_id const&) = delete;
    captured_id&
    operator=(captured_id const&) = delete;

    captured_id(captured_id&& other) noexcept
        : view_(other.view_), slab_(std::exchange(other.slab_, nullptr))
    {
        other.view_ = alia_captured_id_null();
    }

    captured_id&
    operator=(captured_id&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            view_ = other.view_;
            other.view_ = alia_captured_id_null();
            slab_ = std::exchange(other.slab_, nullptr);
        }
        return *this;
    }

    ~captured_id()
    {
        clear();
    }

    bool
    empty() const
    {
        return alia_captured_id_is_null(&view_);
    }

    void
    clear(alia_general_allocator* = nullptr)
    {
        if (slab_)
        {
            auto* hdr = static_cast<slab_header*>(slab_);
            void* captured = static_cast<char*>(slab_) + hdr->captured_offset;
            alia_captured_id_release(captured);
            hdr->alloc->free(
                hdr->alloc->user_data,
                slab_,
                hdr->alloc_size,
                hdr->alloc_align);
            slab_ = nullptr;
        }
        else if (!alia_captured_id_is_null(&view_))
        {
            alia_captured_id_release(&view_);
        }
        view_ = alia_captured_id_null();
    }

    void
    capture(id_view id, alia_general_allocator* alloc)
    {
        clear();
        if (alia_id_view_is_null(id))
            return;

        alia_struct_spec const captured_spec = alia_captured_id_spec(id);
        if (captured_spec.size <= sizeof(alia_captured_id))
        {
            // fully inline - Capture into `view_` without a heap slab.
            alia_captured_id_capture_into(id, &view_, sizeof(view_));
            return;
        }

        ALIA_ASSERT(alloc && alloc->alloc && alloc->free);
        size_t const captured_offset
            = alia_align_up(sizeof(slab_header), captured_spec.align);
        size_t const alloc_size = captured_offset + captured_spec.size;
        size_t const alloc_align
            = captured_spec.align > alignof(slab_header)
                ? captured_spec.align
                : alignof(slab_header);
        void* mem = alloc->alloc(alloc->user_data, alloc_size, alloc_align);
        auto* hdr = static_cast<slab_header*>(mem);
        hdr->alloc = alloc;
        hdr->alloc_size = alloc_size;
        hdr->alloc_align = alloc_align;
        hdr->captured_offset = captured_offset;
        void* captured = static_cast<char*>(mem) + captured_offset;
        alia_captured_id_capture_into(id, captured, captured_spec.size);
        view_ = *static_cast<alia_captured_id*>(captured);
        slab_ = mem;
    }

    bool
    matches(id_view id) const
    {
        if (alia_captured_id_is_null(&view_))
            return alia_id_view_is_null(id);
        return alia_captured_id_matches_view(&view_, id);
    }

    id_view
    as_view() const
    {
        return *alia_captured_id_as_view(&view_);
    }

 private:
    // heap allocation header preceding the captured ID payload
    struct slab_header
    {
        alia_general_allocator* alloc;
        size_t alloc_size;
        size_t alloc_align;
        size_t captured_offset;
    };

    // captured view - For heap captures, external payload pointers address
    // into `slab_`.
    alia_captured_id view_ = alia_captured_id_null();
    // heap block (`slab_header` + captured ID) - nullptr when unused
    void* slab_ = nullptr;
};

template<class A, class B>
struct captured_id<std::pair<A, B>>
{
    bool
    empty() const
    {
        return left_.empty() && right_.empty();
    }

    void
    clear(alia_general_allocator* alloc = nullptr)
    {
        left_.clear(alloc);
        right_.clear(alloc);
    }

    void
    capture(std::pair<A, B> const& id, alia_general_allocator* alloc = nullptr)
    {
        left_.capture(id.first, alloc);
        right_.capture(id.second, alloc);
    }

    bool
    matches(std::pair<A, B> const& id) const
    {
        return left_.matches(id.first) && right_.matches(id.second);
    }

 private:
    captured_id<A> left_;
    captured_id<B> right_;
};

} // namespace alia

// std::hash interface
namespace std {
template<>
struct hash<alia_id_view>
{
    size_t
    operator()(alia_id_view id) const noexcept
    {
        return static_cast<size_t>(alia_id_view_hash(id));
    }
};
} // namespace std
