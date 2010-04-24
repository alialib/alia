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

template<class Key, class Value>
struct data_map_traversal
{
    data_map_traversal() { active_ = false; }
    data_map_traversal(data_map<Key,Value>& data)
    { begin(data); }
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
        std::pair<typename std::map<Key,data_map_entry<Value> >::iterator,bool>
            r = data_->entries.insert(
                typename std::map<Key,data_map_entry<Value> >::value_type(
                key, data_map_entry<Value>()));
        *value = &r.first->second.value;
        r.first->second.touched = true;
        return r.second;
    }
    void end()
    {
        if (active_)
        {
            for (std::map<Key,Value>::iterator i = data_->entries.begin();
                i != data_->entries.end(); )
            {
                std::map<Key,Value>::iterator next = i;
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
    std::pair<typename std::map<Key,data_map_entry<Value> >::iterator,bool>
        r = data.entries.insert(
            typename std::map<Key,data_map_entry<Value> >::value_type(
            key, data_map_entry<Value>()));
    *value = &r.first->second.value;
    return r.second;
}

}

#endif
