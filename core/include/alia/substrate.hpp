#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/substrate/types.h>

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

namespace alia {

// TODO: Revisit naming consistency here.

// Construct a substrate system.
void
construct_substrate_system(
    alia_substrate_system& system, alia_substrate_allocator allocator);

// Destruct a substrate system.
void
destruct_substrate_system(alia_substrate_system& system);

// Destruct a substrate block and invoke all registered destructors.
void
destruct_substrate_block(
    alia_substrate_system& system, alia_substrate_block& block);

void
substrate_begin_traversal(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch);

void
substrate_end_traversal(alia_substrate_traversal& traversal);

struct alia_substrate_usage_result
{
    void* ptr;
    alia_generation_counter generation;
    alia_substrate_block_traversal_mode mode;
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
alia_substrate_usage_result
substrate_use_memory(
    alia_substrate_traversal& traversal, size_t size, size_t alignment);

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
substrate_use_object(
    alia_substrate_traversal& traversal,
    size_t size,
    size_t alignment,
    void (*destructor)(void*));

alia_substrate_block*
substrate_use_block(alia_substrate_traversal& traversal);

using alia_substrate_block_scope = alia_substrate_block_traversal_state;

inline bool
needs_discovery(alia_substrate_block_spec& spec)
{
    return spec.alignment == 0;
}

void
substrate_begin_block(
    alia_substrate_traversal& traversal,
    alia_substrate_block_scope& scope,
    alia_substrate_block& block,
    alia_substrate_block_spec spec);

alia_substrate_block_spec
substrate_get_block_spec(alia_substrate_traversal& traversal);

void
substrate_end_block(
    alia_substrate_traversal& traversal, alia_substrate_block_scope& scope);

} // namespace alia
