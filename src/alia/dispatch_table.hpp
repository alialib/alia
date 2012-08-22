#ifndef ALIA_DISPATCH_TABLE_HPP
#define ALIA_DISPATCH_TABLE_HPP

#include <boost/ptr_container/ptr_map.hpp>

namespace alia {

struct type_info_comparison
{
    bool operator()(std::type_info const* a, std::type_info const* b) const
    { return a->before(*b) ? true : false; }
};

struct dispatch_interface
{
    virtual ~dispatch_interface() {}
};

typedef boost::ptr_map<
    std::type_info const*,dispatch_interface,type_info_comparison>
    dispatch_map;

struct dispatch_table
{
    dispatch_map mapping;
};

template<class Impl>
void add_implementation(dispatch_table& table, Impl* impl)
{
    table.mapping[&typeid(Impl)] = impl;
}

template<class Impl>
bool get_implementation(dispatch_table const& table, Impl** impl)
{
    dispatch_map::const_iterator i = table.mapping.find(&typeid(Impl));
    if (i != table.mapping.end())
    {
        *impl = static_cast<Impl*>(i->second);
        return true;
    }
    else
    {
        *impl = 0;
        return false;
    }
}

}

#endif
