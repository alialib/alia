#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/prelude.h>

struct alia_substrate_system;

struct alia_substrate_destructor_record
{
    alia_substrate_destructor_record* next;
    void (*destructor)(alia_substrate_system*, void*);
    void* ptr;
};

// An `alia_substrate_block` represents the basic block of the substrate
// system. During a single traversal of the substrate system:
// 1. Either all nodes in the block are executed or all nodes are bypassed.
// 2. If executed, they are always executed in the same order.
//
// The block is a contiguous region of memory allocated from the substrate's
// allocator. It stores the data for nodes in the block as well as any
// destructors registered for those nodes.
//
struct alia_substrate_block
{
    // linked list of destructors for nodes in the block
    alia_substrate_destructor_record* destructors;
    // the generation ID for this block - assigned at allocation time
    // TODO: Make this 32 bits.
    alia_generation_counter generation;
    // TODO: We should be able to squeeze this into 32 bits: 24 bits for the
    // size, 8 bits for the alignment (in powers of 2).
    alia_struct_spec spec;
};

struct alia_substrate_system
{
    alia_general_allocator allocator;
    alia_substrate_anchor root_anchor;
    alia_struct_spec root_block_spec;
    // incremented whenever a block is freed (and thus might be reused)
    alia_generation_counter current_generation;
};

struct alia_substrate_block_traversal_state
{
    alia_substrate_block* block;

    alia_substrate_block_traversal_mode mode;

    // the spec for the block
    // In discovery mode, this is calculated as the traversal progresses.
    alia_struct_spec spec;

    // used for assigning memory in non-discovery passes
    size_t current_offset;

    // used for reconstructing the full path to nodes in this block
    alia_substrate_block_traversal_state* parent;
    size_t offset_in_parent;
};

// `alia_substrate_traversal` is the state associated with a single traversal
// of an `alia_substrate_system`.
struct alia_substrate_traversal
{
    alia_bump_allocator scratch;
    alia_substrate_system* system;
    alia_substrate_block_traversal_state block;
};

namespace alia {

void
substrate_system_init(
    alia_substrate_system& system, alia_general_allocator allocator);

void
substrate_system_cleanup(alia_substrate_system& system);

void
substrate_block_cleanup(
    alia_substrate_system& system, alia_substrate_block* block);

void
substrate_traversal_init(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch);

} // namespace alia
