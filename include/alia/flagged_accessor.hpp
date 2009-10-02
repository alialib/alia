#ifndef ALIA_BIT_FLAGGED_ACCESSOR_HPP
#define ALIA_BIT_FLAGGED_ACCESSOR_HPP

#include <alia/accessor.hpp>

namespace alia {

template<class T, class BitFlags>
struct bit_flagged_accessor : accessor<T>
{
    bit_flagged_accessor(T* v, BitFlags* flags, BitFlags mask)
     : v_(v), flags_(flags), mask_(mask) {}
    bool is_valid() const
    {
        return (*flags_ & mask_) != 0;
    }
    T get() const
    {
        return *v_;
    }
    void set(T const& value) const
    {
        *v_ = value;
        *flags_ |= mask;
    }
 private:
    T* v_;
    BitFlags* flags_;
    BitFlags mask_;
};

template<class T, class BitFlags>
bit_flagged_accessor<T>
make_bit_flagged_accessor(T* value, BitFlags* flags, BitFlags mask)
{
    return bit_flagged_accessor<T>(value, flags, mask);
}

}

#endif
