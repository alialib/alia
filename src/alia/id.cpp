#include <alia/id.hpp>
#include <typeinfo>

namespace alia {

inline bool
types_match(id_interface const& a, id_interface const& b)
{
    return typeid(a).name() == typeid(b).name() || typeid(a) == typeid(b);
}

bool
operator<(id_interface const& a, id_interface const& b)
{
    return typeid(a).before(typeid(b)) || (types_match(a, b) && a.less_than(b));
}

void
clone_into(id_interface*& storage, id_interface const* id)
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

void
clone_into(std::unique_ptr<id_interface>& storage, id_interface const* id)
{
    if (!id)
    {
        storage.reset();
    }
    else if (storage && types_match(*storage, *id))
    {
        id->deep_copy(&*storage);
    }
    else
    {
        storage.reset(id->clone());
    }
}

bool
operator==(captured_id const& a, captured_id const& b)
{
    return a.is_initialized() == b.is_initialized()
           && (!a.is_initialized() || a.get() == b.get());
}
bool
operator!=(captured_id const& a, captured_id const& b)
{
    return !(a == b);
}
bool
operator<(captured_id const& a, captured_id const& b)
{
    return b.is_initialized() && (!a.is_initialized() || a.get() < b.get());
}

} // namespace alia
