#ifndef ALIA_CORE_BIT_OPS_HPP
#define ALIA_CORE_BIT_OPS_HPP

namespace alia {

// Ops on single bits.

inline unsigned
read_bit(unsigned bitset, unsigned index)
{
    return (bitset >> index) & 1u;
}

inline void
set_bit(unsigned& bitset, unsigned index)
{
    bitset |= (1u << index);
}

inline void
clear_bit(unsigned& bitset, unsigned index)
{
    bitset &= ~(1u << index);
}

inline void
toggle_bit(unsigned& bitset, unsigned index)
{
    bitset ^= (1u << index);
}

// Ops on pairs of bits (i.e., two-bit values).

inline unsigned
read_bit_pair(unsigned bitset, unsigned index)
{
    return (bitset >> index) & 3u;
}

inline void
write_bit_pair(unsigned& bitset, unsigned index, unsigned new_value)
{
    bitset = (bitset & ~(3u << index)) | (new_value << index);
}

} // namespace alia

#endif
