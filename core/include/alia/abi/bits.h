#ifndef ALIA_ABI_BITS_H
#define ALIA_ABI_BITS_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_bitref
{
    uint32_t* storage;
    unsigned offset;
} alia_bitref;

static inline bool
alia_bitref_is_set(alia_bitref ref)
{
    return (*ref.storage & (1u << ref.offset)) != 0;
}

static inline void
alia_bitref_set(alia_bitref ref)
{
    *ref.storage |= (1u << ref.offset);
}

static inline void
alia_bitref_clear(alia_bitref ref)
{
    *ref.storage &= ~(1u << ref.offset);
}

static inline void
alia_bitref_toggle(alia_bitref ref)
{
    *ref.storage ^= (1u << ref.offset);
}

static inline uint32_t
alia_bitref_read_pair(alia_bitref ref)
{
    return (*ref.storage >> ref.offset) & 0x3;
}

static inline void
alia_bitref_write_pair(alia_bitref ref, uint32_t value)
{
    *ref.storage
        = (*ref.storage & ~(0x3 << ref.offset)) | (value << ref.offset);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_BITS_H
