#ifndef ALIA_ABI_KERNEL_SUBSTRATE_H
#define ALIA_ABI_KERNEL_SUBSTRATE_H

#include <alia/abi/base/allocator.h>
#include <alia/abi/base/arena.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

#include <alia/abi/context.h>

// This file defines the data retrieval library used for associating mutable
// state and cached data with Alia components. It is designed so that each
// component emitted by an application is associated with a unique instance of
// data, even if there is no specific external identifier for that component.
//
// More generally, if you replace "component" with "subexpression evaluation"
// in the previous sentence, it can be used to associate data with particular
// points in the evaluation of any function. This can be useful in situations
// where you need to evaluate a particular function many times with slightly
// different inputs and you want to reuse the work that was done in earlier
// evaluations without a lot of manual bookkeeping.
//
// To understand what's going on here, imagine the evaluation of a function on
// a simple in-order, single-threaded processor. We can represent all possible
// execution flows using a single DAG where each node represents the execution
// of a particular instruction by the processor and edges represent the
// transition to the next instruction. Nodes with multiple edges leaving them
// represent the execution of branch instructions, while nodes with multiple
// edges coming in are points where multiple branches merge back into a single
// flow.
//
// Since the graph is a DAG, loops are represented by unrolling them.
// Similarly, function calls are represented by inlining the callee's graph
// into the caller's graph (with appropriate argument substitutions).
// Note that both of these features make the graph potentially infinite.
// Furthermore, if calls to function pointers are involved, parts of the graph
// may be entirely unknown.
//
// Thus, for an arbitrary function, we cannot construct its graph a priori.
// However, we CAN observe a particular evaluation of the function and
// construct its path through the graph. We can also observe multiple
// evaluations and construct the portion of the DAG that these executions
// cover. In other words, if we're only interested in portions of the graph
// that are reached by actual evaluations of the function, we can lazily
// construct them by simply observing those evaluations.
//
// And that is essentially what this library does. In order to use it, you
// must annotate the control flow in your function, and it uses these
// annotations to trace each evaluation's flow through the graph, constructing
// unexplored regions as they're encountered. The graph is used to store data
// that is made available to your function as it executes.
//
// One problem with all this is that sometimes a subexpression evaluation
// (i.e., a component) is associated with a particular piece of input data and
// the evaluation of that input data is not fixed within the graph (e.g., it's
// in a list of items where you can remove or shuffle items). In cases like
// this, we allow the application to attach an explicit key to the subgraph
// representing the evaluation of that expression, and we ensure that that
// subgraph is always used where that name is encountered.

ALIA_EXTERN_C_BEGIN

// TODO: Fix forward declarations.
typedef struct alia_substrate_system alia_substrate_system;
typedef struct alia_substrate_block alia_substrate_block;
typedef struct alia_substrate_traversal alia_substrate_traversal;
typedef struct alia_context alia_context;

enum alia_substrate_block_traversal_mode
{
    // normal mode
    ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL = 0,
    // initialization mode - This is set for the first pass over a freshly
    // allocated block.
    ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT = 1,
    // discovery mode - This is used to discover the shape of the graph.
    // Specifically, when traversing a block, this is used to measure the total
    // size of data used by nodes in the block.
    ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY = 2,
};

typedef struct alia_substrate_usage_result
{
    void* ptr;
    alia_generation_counter generation;
    enum alia_substrate_block_traversal_mode mode;
} alia_substrate_usage_result;

// 'Use' memory from the current substrate block.
//
// The `mode` field in the result indicates how the memory should be treated:
// - `NORMAL`: This is a normal usage. The pointer points to an existing node
//   that has been seen before.
// - `INIT`: This is a new usage. The pointer points to newly allocated memory
//   that should be initialized.
// - `DISCOVERY`: This is a discovery usage. The pointer points to newly
//   allocated memory that will be discarded once discovery is complete. You
//   only need to initialize the memory to the extent that the calling function
//   and/or the associated destructor require it.
//
alia_substrate_usage_result
alia_substrate_use_memory(alia_context* ctx, size_t size, size_t alignment);

// 'Use' an 'object' from the current substrate block.
//
// By using an object, you are both using memory to store that object and
// providing a destructor that will be called when the memory is freed.
//
// Note that currently, the destructor is called even when cleaning up after
// discovery. TODO: Maybe the caller can pass a null destructor to disable
// this.
//
alia_substrate_usage_result
alia_substrate_use_object(
    alia_context* ctx,
    size_t size,
    size_t alignment,
    void (*destructor)(alia_substrate_system*, void*));

// An anchor is a point of attachment for a substrate block. - Technically it's
// just a pointer to a block, but it tends to act as a slot where blocks
// (especially conditional blocks) can attach to the substrate.
typedef struct alia_substrate_anchor
{
    alia_substrate_block* block;
} alia_substrate_anchor;

// 'Use' an anchor from the current substrate block.
alia_substrate_anchor*
alia_substrate_use_anchor(alia_context* ctx);

// Reset an anchor, detaching any block that was attached to it.
void
alia_substrate_reset_anchor(alia_context* ctx, alia_substrate_anchor* anchor);

// Determine if a block needs to be discovered.
// `spec` is the memoized memory layout specification of the block.
inline bool
alia_substrate_block_needs_discovery(alia_struct_spec* spec)
{
    return spec->align == 0;
}

// Begin the scope of a child substrate block.
// `spec` is the memoized memory layout specification of the block.
// If `alia_substrate_block_needs_discovery` returns true on the spec, the
// traversal will enter discovery mode for the block.
void
alia_substrate_begin_block(
    alia_context* ctx, alia_substrate_anchor* anchor, alia_struct_spec* spec);

// End the scope of the current substrate block.
// Returns the layout specification of the block. - If the block was in
// discovery mode, this should be used to update the memoized spec value.
alia_struct_spec
alia_substrate_end_block(alia_context* ctx);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_KERNEL_SUBSTRATE_H
