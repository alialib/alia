#include <alia/id.hpp>
#include <typeinfo>

namespace alia {

static inline bool
types_match(id_interface const& a, id_interface const& b)
{
    return typeid(a).name() == typeid(b).name() || typeid(a) == typeid(b);
}

bool operator<(id_interface const& a, id_interface const& b)
{
    return typeid(a).before(typeid(b)) || (types_match(a, b) && a.less_than(b));
}

void clone_into(id_interface*& storage, id_interface const* id)
{
    if (!id)
    {
        delete storage;
        storage = 0;
    }
    else if (storage && types_match(*storage, *id))
    {
        id->deep_copy(storage);
    }
    else
    {
        delete storage;
        storage = id->clone();
    }
}

bool operator==(owned_id const& a, owned_id const& b)
{
    return a.is_initialized() == b.is_initialized() &&
        (!a.is_initialized() || a.get() == b.get());
}
bool operator!=(owned_id const& a, owned_id const& b)
{ return !(a == b); }
bool operator<(owned_id const& a, owned_id const& b)
{ return b.is_initialized() && (!a.is_initialized() || a.get() < b.get()); }
std::ostream& operator<<(std::ostream& o, owned_id const& id)
{ o << id.get(); return o; }

local_id generate_local_id()
{
    local_id id;
    id.tag.reset(new int);
    id.version = 0;
    return id;
}

std::ostream& operator<<(std::ostream& o, local_id const& id)
{ return o << "local_id(" << id.tag.get() << ":" << id.version << ")"; }

}
