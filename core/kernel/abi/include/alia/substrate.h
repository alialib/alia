#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_substrate_ctx alia_substrate_ctx;
typedef struct alia_pass_scope alia_pass_scope;
typedef struct alia_substrate_block alia_substrate_block;

typedef struct
{
    uint64_t hash;
    const void* bytes;
    uint32_t size;
    uint32_t kind;
} alia_key;

typedef enum
{
    // caller-driven sizing pass; substrate returns fake/placeholder storage
    ALIA_PASS_DISCOVERY = 1,
    // structure may differ from prior; allocate blocks as needed; run GC
    ALIA_PASS_REFRESH = 2,
    // structure is stable; no GC; no structural changes expected
    ALIA_PASS_STABLE = 3
} alia_pass_mode;

typedef void* (*alia_alloc_fn)(void* ud, size_t size, size_t align);
typedef void (*alia_dealloc_fn)(void* ud, void* ptr);

alia_substrate_ctx*
alia_substrate_create(
    alia_alloc_fn alloc, alia_dealloc_fn dealloc, void* user_data);

void
alia_substrate_destroy(alia_substrate_ctx*);

void
alia_pass_begin(
    alia_substrate_ctx*, alia_pass_mode mode, alia_pass_scope** out);

void
alia_pass_end(alia_substrate_ctx*, alia_pass_scope*);

alia_substrate_block*
alia_substrate_use_block(alia_substrate_ctx*);

typedef size_t alia_block_plan;

void
alia_block_begin(
    alia_substrate_ctx*,
    alia_plan_ptr* plan_slot, // address of your static alia_block_plan*
    alia_block_frame** out);

void
alia_block_end(alia_substrate_ctx*, alia_block_frame*);

void*
alia_block_alloc(alia_substrate_ctx*, size_t size, size_t align);

#ifdef __cplusplus
} // extern "C"
#endif
