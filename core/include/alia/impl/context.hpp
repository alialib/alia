#pragma once

#include <alia/abi/ui/context_ids.h>
#include <alia/abi/ui/events.h>

#include <type_traits>
#include <utility>

namespace alia {

// `context` is the C++ pass handle. It stores a pointer to a concrete
// context struct. `HasSubstrate` is false for an ephemeral view.
template<class Storage, bool HasSubstrate>
struct context
{
    // C context for this pass
    Storage* ptr = nullptr;

    context() = default;

    // Build a handle for `storage`.
    explicit context(Storage* storage) : ptr(storage)
    {
    }

    Storage*
    operator->() const
    {
        return ptr;
    }

    Storage&
    operator*() const
    {
        return *ptr;
    }

    // Convert to the C context pointer.
    operator Storage*() const
    {
        return ptr;
    }

    // Return this handle without substrate access. The C pointer stays live.
    context<Storage, false>
    without_substrate() const
    {
        static_assert(HasSubstrate, "context is already ephemeral");
        return context<Storage, false>(ptr);
    }

    // Drop substrate.
    operator context<Storage, false>() const
        requires HasSubstrate
    {
        return without_substrate();
    }

    // Narrow a UI handle onto its embedded kernel context.
    // A full handle may also become ephemeral.
    template<bool KernelHasSubstrate>
        requires std::is_same_v<Storage, alia_ui_context>
              && (HasSubstrate || !KernelHasSubstrate)
    operator context<alia_kernel_context, KernelHasSubstrate>() const
    {
        return context<alia_kernel_context, KernelHasSubstrate>(&ptr->kernel);
    }
};

// UI pass handle with substrate
using ui_context = context<alia_ui_context, true>;

// kernel pass handle with substrate
using kernel_context = context<alia_kernel_context, true>;

// Run `content` with collapsed context IDs.
template<class Content>
void
context_id_collapse(ui_context ctx, Content&& content)
{
    alia_context_id_collapse_begin(ctx);
    std::forward<Content>(content)();
    alia_context_id_collapse_end(ctx);
}

} // namespace alia
