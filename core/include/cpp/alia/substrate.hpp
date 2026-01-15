#pragma once

#include <cassert>
#include <cstdint>

#include <alia/arena.h>

// This file defines the data retrieval library used for associating mutable
// state and cached data with alia content graphs. It is designed so that each
// node emitted by an application is associated with a unique instance of data,
// even if there is no specific external identifier for that node.
//
// More generally, if you replace "node" with "subexpression evaluation" in the
// previous sentence, it can be used to associate data with particular points
// in the evaluation of any function. This can be useful in situations where
// you need to evaluate a particular function many times with slightly
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
// (content node) is associated with a particular piece of input data and the
// evaluation of that input data is not fixed within the graph (e.g., it's in a
// list of items where you can remove or shuffle items). In cases like this, we
// allow the application to attach an explicit key to the subgraph
// representing the evaluation of that expression, and we ensure that that
// subgraph is always used where that name is encountered.

namespace alia {

// enum alia_key_kind
// {
//     ALIA_KEY_NONE = 0,
//     ALIA_KEY_U64 = 1,
//     ALIA_KEY_I64 = 2,
//     ALIA_KEY_PTR = 3, // address identity; beware lifetimes/reuse
//     ALIA_KEY_UUID = 4, // 128-bit (hi, lo)
//     ALIA_KEY_STR = 5, // UTF-8 bytes (case-sensitive)
//     ALIA_KEY_BYTES = 6, // arbitrary bytes
//     ALIA_KEY_TUPLE = 7, // canonical composite produced by builder
// };

// struct alia_key_view
// {
//     uint64_t hash64; // hash of the canonical bytes (see below)
//     alia_key_kind kind;
//     uint32_t size; // bytes in canonical payload
//     union
//     {
//         // Small canonical payload fits here (e.g., u64, i64, ptr, uuid,
//         short
//         // strings)
//         uint8_t small[16];
//         // For longer payloads (str/bytes/tuple), point at caller memory;
//         graph
//         // copies it.
//         struct
//         {
//             const void* ptr;
//         } ext;
//     } payload;
// };

struct substrate_allocator
{
    void* (*alloc)(void* user_data, size_t size, size_t alignment) = nullptr;
    void (*free)(void* user_data, void* ptr) = nullptr;
    void* user_data = nullptr;
};

struct substrate_destructor_record
{
    substrate_destructor_record* next = nullptr;
    void (*destructor)(void*) = nullptr;
    void* ptr = nullptr;
};

struct substrate_system;

using generation_counter = std::uint32_t;

// A substrate_block represents a block of execution. During a single traversal
// of the substrate system, either all nodes in the block are executed or all
// nodes are bypassed, and, if executed, they are always executed in the same
// order.
//
// The block is a contiguous region of memory that is allocated from the
// substrate's allocator. It stores the data for nodes in the block as well as
// any destructors registered for those nodes.
//
struct substrate_block
{
    // the system that owns this block - This is needed to destruct the block
    // using only a pointer to the block itself.
    substrate_system* system = nullptr;
    std::uint8_t* storage = nullptr;
    substrate_destructor_record* destructors = nullptr;
    // the generation ID for this block - assigned at allocation time
    generation_counter generation = 0;
};

struct substrate_system
{
    substrate_allocator allocator{};
    substrate_block root_block{};
    // incremented whenever a block is freed (and thus might be reused)
    generation_counter current_generation = 0;
};

struct substrate_block_discovery_state
{
    size_t offset = 0;
    size_t alignment = 0;
};

enum class substrate_block_traversal_mode
{
    // normal mode
    NORMAL = 0,
    // initialization mode - This is set for the first pass over a freshly
    // allocated block.
    INIT = 1,
    // discovery mode - This is used to discover the
    // shape of the graph. Specifically, when traversing a block, this is used
    // to measure the total size of data used by nodes in the block.
    DISCOVERY = 2,
};

struct substrate_block_traversal_state
{
    substrate_block* data = nullptr;

    // used for reconstructing the full path to nodes in this block
    substrate_block_traversal_state* parent = nullptr;
    size_t offset_in_parent = 0;

    // used for assigning memory in non-discovery passes
    size_t current_offset = 0;

    substrate_block_traversal_mode mode{};

    substrate_block_discovery_state* discovery = nullptr;
};

// `substrate_traversal` stores the state associated with a single traversal of
// a `substrate_system`.
struct substrate_traversal
{
    // TODO: Not sure if the traversal actually needs to store its own pointer
    // to this.
    // TODO: Also, it should use the C++ API.
    alia_arena_view* scratch = nullptr;
    substrate_system* system = nullptr;
    substrate_block_traversal_state block{};
};

// TODO: Revisit naming consistency here.

// Construct a substrate system.
void
construct_substrate_system(
    substrate_system& system, substrate_allocator allocator);

// Destruct a substrate system.
void
destruct_substrate_system(substrate_system& system);

// Destruct a substrate block and invoke all registered destructors.
void
destruct_substrate_block(substrate_system& system, substrate_block& block);

// TODO: Revisit all this...
// The utilities here operate on `substrate_traversal`s. However, the library
// is intended to be used in scenarios where the `substrate_traversal` object
// is part of a larger context. Thus, any utilities here that are intended to
// be used directly by the application developer are designed to accept a
// generic context parameter. The only requirement on that paramater is that it
// defines the function `get_substrate_traversal(ctx)`, which returns a
// reference to a `substrate_traversal`.

// If using the substrate functionality directly, the substrate_traversal
// itself can serve as the context.
inline substrate_traversal&
get_substrate_traversal(substrate_traversal& ctx)
{
    return ctx;
}

void
substrate_begin_traversal(
    substrate_traversal& traversal,
    substrate_system& system,
    alia_arena_view* scratch);

void
substrate_end_traversal(substrate_traversal& traversal);

struct substrate_usage_result
{
    void* ptr = nullptr;
    generation_counter generation = 0;
    substrate_block_traversal_mode mode
        = substrate_block_traversal_mode::NORMAL;
};

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
substrate_usage_result
substrate_use_memory(
    substrate_traversal& traversal, size_t size, size_t alignment);

// 'Use' an 'object' from the current substrate block.
//
// By using an object, you are both using memory to store that object and
// providing a destructor that will be called when the memory is freed.
//
// Note that currently, the destructor is called even when cleaning up after
// discovery. TODO: Add an flag to disable this.
//
substrate_usage_result
substrate_use_object(
    substrate_traversal& traversal,
    size_t size,
    size_t alignment,
    void (*destructor)(void*));

substrate_block*
substrate_use_block(substrate_traversal& traversal);

using substrate_block_scope = substrate_block_traversal_state;

struct substrate_block_spec
{
    size_t size = 0;
    size_t alignment = 0;
};

inline bool
needs_discovery(substrate_block_spec& spec)
{
    return spec.alignment == 0;
}

void
substrate_begin_block(
    substrate_traversal& traversal,
    substrate_block_scope& scope,
    substrate_block& block,
    substrate_block_spec spec);

substrate_block_spec
substrate_get_block_spec(substrate_traversal& traversal);

void
substrate_end_block(
    substrate_traversal& traversal, substrate_block_scope& scope);

} // namespace alia
