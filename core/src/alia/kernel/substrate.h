#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/ids.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/prelude.h>

#include <stdint.h>

struct alia_substrate_system;
struct alia_substrate_key_map;
struct alia_substrate_key_table;

struct alia_substrate_cleanup_record
{
    alia_substrate_cleanup_record* next;
    void (*cleanup)(
        alia_substrate_system*, void*, alia_substrate_cleanup_mode mode);
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
    // linked list of cleanup records for nodes in the block
    alia_substrate_cleanup_record* cleanup_records;
    // the generation ID for this block - assigned at allocation time
    alia_generation_counter generation;
    // TODO: We should be able to squeeze this into 32 bits: 24 bits for the
    // size, 8 bits for the alignment (in powers of 2).
    alia_struct_spec spec;
};

struct alia_substrate_key_entry
{
    alia_substrate_anchor anchor;
    alia_captured_id key;
    void* key_storage;
    uint32_t last_seen;
    uint8_t flags;
    // prediction list link
    alia_substrate_key_entry* next;
};

struct alia_substrate_key_table
{
    bool requires_explicit_delete;
    alia_substrate_key_entry* first;
    alia_substrate_key_map* map;
    alia_substrate_system* system;
    alia_substrate_key_table* registry_next;
    uint32_t last_seen;
};

struct alia_substrate_system
{
    alia_general_allocator allocator;
    alia_substrate_anchor root_anchor;
    alia_struct_spec root_block_spec;
    // incremented whenever a block is freed (and thus might be reused)
    alia_generation_counter current_generation;
    alia_substrate_key_table* key_table_registry;
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

struct alia_substrate_key_scope
{
    alia_substrate_key_table* table;
    alia_substrate_key_entry* predicted;
    alia_substrate_key_entry** prediction_tail;
};

// `alia_substrate_traversal` is the state associated with a single traversal
// of an `alia_substrate_system`.
struct alia_substrate_traversal
{
    alia_bump_allocator scratch;
    alia_substrate_system* system;
    alia_substrate_block_traversal_state block;
    uint32_t current_frame;
    // When true, keyed lookups update each table's prediction list.
    // This should be disabled on partial passes.
    bool allow_prediction_updates;
};

namespace alia {

void
substrate_system_init(
    alia_substrate_system& system, alia_general_allocator allocator);

void
substrate_system_destroy(alia_substrate_system& system);

// Reset the system, destroying the root block if it exists.
// This is mainly intended for test/harness usage.
void
substrate_system_reset(alia_substrate_system& system);

void
substrate_traversal_init(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch,
    uint32_t current_frame,
    bool allow_prediction_updates);

void
substrate_reset_anchor(
    alia_substrate_system* system, alia_substrate_anchor* anchor);

} // namespace alia
