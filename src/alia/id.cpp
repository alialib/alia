#include <alia/id.hpp>
#include <typeinfo>

namespace alia {

bool operator==(id_ref const& a, id_ref const& b)
{
    return !a.id && !b.id ||
        a.id && b.id && typeid(*a.id) == typeid(*b.id) && a.id->equals(*b.id);
}
bool operator!=(id_ref const& a, id_ref const& b)
{
    return !(a == b);
}
bool operator<(id_ref const& a, id_ref const& b)
{
    return b.id && (!a.id || typeid(*a.id).before(typeid(*b.id)) ||
        typeid(*a.id) == typeid(*b.id) && a.id->less_than(*b.id));
}

}
