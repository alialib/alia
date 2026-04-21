#pragma once

#include <alia/abi/kernel/substrate.h>

// test-only substrate harness
//
// The core substrate API is C ABI, but the initialization/cleanup helpers and
// internal substrate struct layouts live in C++ internals. This harness
// provides opaque C-callable entry points to set up a usable substrate
// context for C tests.

ALIA_EXTERN_C_BEGIN

typedef struct alia_test_substrate_fixture alia_test_substrate_fixture;

alia_test_substrate_fixture*
alia_test_substrate_fixture_create(alia_general_allocator allocator);

void
alia_test_substrate_fixture_destroy(alia_test_substrate_fixture* fixture);

alia_substrate_traversal*
alia_test_substrate_fixture_traversal(alia_test_substrate_fixture* fixture);

alia_substrate_system*
alia_test_substrate_fixture_system(alia_test_substrate_fixture* fixture);

alia_substrate_block*
alia_test_substrate_fixture_root_block(alia_test_substrate_fixture* fixture);

// Reset traversal state (especially scratch arena allocation offset).
void
alia_test_substrate_fixture_reset_traversal(
    alia_test_substrate_fixture* fixture);

// Cleanup the root block and run all registered destructors for it.
// This also increments the substrate system generation counter.
void
alia_test_substrate_fixture_cleanup_root_block(
    alia_test_substrate_fixture* fixture);

ALIA_EXTERN_C_END
