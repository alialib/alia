#ifndef ALIA_DATA_MAP_HPP
#define ALIA_DATA_MAP_HPP

namespace alia {

// TODO: Make this more efficient, and maybe less templated.

template<class Value>
struct data_map_entry
{
    Value value;
    bool touched;
};

template<class Key, class Value>
struct data_map
{
    std::map<Key,data_map_entry<Value> > entries;
};

// TODO: This causes problems when end_pass is called. It should detect if
// the pass was prematurely ended and not delete untouched items, but that
// would require a context parameter.

template<class Key, class Value>
struct data_map_traversal
{
    data_map_traversal() { active_ = false; }
    data_map_traversal(data_map<Key,Value>& data)
    { begin(data); }
    ~data_map_traversal() { end(); }
    void begin(data_map<Key,Value>& data)
    {
        data_ = &data;
        for (std::map<Key,data_map_entry<Value> >::iterator
            i = data_->entries.begin(); i != data_->entries.end(); ++i)
        {
            i->second.touched = false;
        }
        active_ = true;
    }
    bool get(Value** value, Key const& key)
    {
        typename std::map<Key,data_map_entry<Value> >::iterator i =
            data_->entries.find(key);
        bool not_found = i == data_->entries.end();
        if (not_found)
        {
            i = data_->entries.insert(
                typename std::map<Key,data_map_entry<Value> >::value_type(
                key, data_map_entry<Value>())).first;
        }
        *value = &i->second.value;
        i->second.touched = true;
        return not_found;
    }
    void end()
    {
        if (active_)
        {
            for (std::map<Key,data_map_entry<Value> >::iterator
                i = data_->entries.begin(); i != data_->entries.end(); )
            {
                std::map<Key,data_map_entry<Value> >::iterator next = i;
                ++next;
                if (!i->second.touched)
                    data_->entries.erase(i);
                i = next;
            }
            active_ = false;
        }
    }
 private:
    data_map<Key,Value>* data_;
    bool active_;
};

// This is used to get data outside of a normal traversal.
template<class Key, class Value>
bool get(Value** value, data_map<Key,Value>& data, Key const& key)
{
    typename std::map<Key,data_map_entry<Value> >::iterator i =
        data.entries.find(key);
    bool not_found = i == data.entries.end();
    if (not_found)
    {
        i = data.entries.insert(
            typename std::map<Key,data_map_entry<Value> >::value_type(
            key, data_map_entry<Value>())).first;
    }
    *value = &i->second.value;
    i->second.touched = true;
    return not_found;
}

}

#endif
