#ifndef ALIA_ABI_KERNEL_SUBSTRATE_TYPES_H
#define ALIA_ABI_KERNEL_SUBSTRATE_TYPES_H

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_substrate_allocator
{
    void* (*alloc)(void* user_data, size_t size, size_t alignment);
    void (*free)(void* user_data, void* ptr);
    void* user_data;
} alia_substrate_allocator;

typedef struct alia_substrate_destructor_record
{
    alia_substrate_destructor_record* next;
    void (*destructor)(void*);
    void* ptr;
} alia_substrate_destructor_record;

struct alia_substrate_system;

typedef uint32_t alia_generation_counter;

// An `alia_substrate_block` represents the basic block of the substrate
// system. During a single traversal of the substrate system:
// 1. Either all nodes in the block are executed or all nodes are bypassed.
// 2. If executed, they are always executed in the same order.
//
// The block is a contiguous region of memory allocated from the substrate's
// allocator. It stores the data for nodes in the block as well as any
// destructors registered for those nodes.
//
typedef struct alia_substrate_block
{
    // the system that owns this block - This is needed to destruct the block
    // using only a pointer to the block itself.
    alia_substrate_system* system;
    uint8_t* storage;
    alia_substrate_destructor_record* destructors;
    // the generation ID for this block - assigned at allocation time
    alia_generation_counter generation;
} alia_substrate_block;

struct alia_substrate_block_spec
{
    size_t size;
    size_t alignment;
};

typedef struct alia_substrate_system
{
    alia_substrate_allocator allocator;
    alia_substrate_block root_block;
    alia_substrate_block_spec root_block_spec;
    // incremented whenever a block is freed (and thus might be reused)
    alia_generation_counter current_generation;
} alia_substrate_system;

typedef struct alia_substrate_block_discovery_state
{
    size_t offset = 0;
    size_t alignment = 0;
} alia_substrate_block_discovery_state;

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

typedef struct alia_substrate_block_traversal_state
{
    alia_substrate_block* data;

    // used for reconstructing the full path to nodes in this block
    alia_substrate_block_traversal_state* parent;
    size_t offset_in_parent;

    // used for assigning memory in non-discovery passes
    size_t current_offset;

    alia_substrate_block_traversal_mode mode;

    alia_substrate_block_discovery_state* discovery;
} alia_substrate_block_traversal_state;

// `alia_substrate_traversal` stores the state associated with a single
// traversal of an `alia_substrate_system`.
struct alia_substrate_traversal
{
    alia_bump_allocator scratch;
    alia_substrate_system* system;
    alia_substrate_block_traversal_state block;
};

ALIA_EXTERN_C_END

#endif
