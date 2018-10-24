#ifndef ALIA_COMMON_HPP
#define ALIA_COMMON_HPP

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

// This file defines some generic functionality that's commonly used throughout
// alia.

namespace alia {

typedef long long counter_type;

// Inspired by Boost, inheriting from noncopyable disables copying for a type.
// The namespace prevents unintended ADL if used by applications.
namespace impl {
namespace noncopyable_ {
struct noncopyable
{
    noncopyable()
    {
    }

 private:
    noncopyable(noncopyable const& other) = delete;
    noncopyable&
    operator=(noncopyable const& other)
        = delete;
};
} // namespace noncopyable_
} // namespace impl
typedef impl::noncopyable_::noncopyable noncopyable;

// general-purpose exception class for alia
struct exception : std::exception
{
    exception(std::string const& msg) : msg_(new std::string(msg))
    {
    }

    ~exception() throw()
    {
    }

    virtual char const*
    what() const throw()
    {
        return msg_->c_str();
    }

    // Add another level of context to the error messsage.
    void
    add_context(std::string const& str)
    {
        *msg_ += "\n" + str;
    }

 private:
    std::shared_ptr<std::string> msg_;
};

// ALIA_UNUSED is used to denote that a variable may be unused.
#ifdef __GNUC__
#define ALIA_UNUSED [[gnu::unused]]
#else
#define ALIA_UNUSED
#endif

// implementation of C++17's void_t that works on C++11 compilers
template<typename... Ts>
struct make_void
{
    typedef void type;
};
template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

} // namespace alia

#endif
