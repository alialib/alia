#ifndef ALIA_SIGNALS_HIGHER_ORDER_HPP
#define ALIA_SIGNALS_HIGHER_ORDER_HPP

#include <map>
#include <utility>
#include <vector>

#include <alia/flow/for_each.hpp>

namespace alia {

// `transform()` is the component-level version of `std::transform`. See the
// docs for more details.

// the sequence version...

template<class MappedItem>
struct mapped_sequence_data
{
    captured_id input_id;
    std::vector<MappedItem> mapped_items;
    std::vector<captured_id> item_ids;
    counter_type output_version = 0;
};

template<class MappedItem>
struct mapped_sequence_signal : signal<
                                    mapped_sequence_signal<MappedItem>,
                                    std::vector<MappedItem>,
                                    read_only_signal>
{
    mapped_sequence_signal(
        mapped_sequence_data<MappedItem>& data, bool all_items_have_values)
        : data_(&data), all_items_have_values_(all_items_have_values)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->output_version);
        return id_;
    }
    bool
    has_value() const
    {
        return all_items_have_values_;
    }
    std::vector<MappedItem> const&
    read() const
    {
        return data_->mapped_items;
    }

 private:
    mapped_sequence_data<MappedItem>* data_;
    bool all_items_have_values_;
    mutable simple_id<counter_type> id_;
};

template<
    class Context,
    class Container,
    class Function,
    std::enable_if_t<
        !is_map_like<typename Container::value_type>::value,
        int> = 0>
auto
transform(Context ctx, Container const& container, Function&& f)
{
    typedef typename decltype(
        f(ctx,
          std::declval<readable<
              typename Container::value_type::value_type>>()))::value_type
        mapped_value_type;

    mapped_sequence_data<mapped_value_type>* data;
    get_cached_data(ctx, &data);

    bool all_items_have_values = false;

    ALIA_IF(has_value(container))
    {
        size_t container_size = read_signal(container).size();

        if (!data->input_id.matches(container.value_id()))
        {
            data->mapped_items.resize(container_size);
            data->item_ids.resize(container_size);
            ++data->output_version;
            data->input_id.capture(container.value_id());
        }

        size_t valid_item_count = 0;
        auto captured_item = data->mapped_items.begin();
        auto captured_id = data->item_ids.begin();
        for_each(ctx, container, [&](context ctx, auto item) {
            auto mapped_item = f(ctx, item);
            if (signal_has_value(mapped_item))
            {
                if (!captured_id->matches(mapped_item.value_id()))
                {
                    *captured_item = read_signal(mapped_item);
                    captured_id->capture(mapped_item.value_id());
                    ++data->output_version;
                }
                ++valid_item_count;
            }
            ++captured_item;
            ++captured_id;
        });
        assert(captured_item == data->mapped_items.end());
        assert(captured_id == data->item_ids.end());

        all_items_have_values = (valid_item_count == container_size);
    }
    ALIA_END

    return mapped_sequence_signal<mapped_value_type>(
        *data, all_items_have_values);
}

// the map version...

template<class Key, class MappedItem>
struct mapped_map_data
{
    captured_id input_id;
    std::map<Key, MappedItem> mapped_items;
    std::vector<captured_id> item_ids;
    counter_type output_version = 0;
};

template<class Key, class MappedItem>
struct mapped_map_signal : signal<
                               mapped_map_signal<Key, MappedItem>,
                               std::map<Key, MappedItem>,
                               read_only_signal>
{
    mapped_map_signal(
        mapped_map_data<Key, MappedItem>& data, bool all_items_have_values)
        : data_(&data), all_items_have_values_(all_items_have_values)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->output_version);
        return id_;
    }
    bool
    has_value() const
    {
        return all_items_have_values_;
    }
    std::map<Key, MappedItem> const&
    read() const
    {
        return data_->mapped_items;
    }

 private:
    mapped_map_data<Key, MappedItem>* data_;
    bool all_items_have_values_;
    mutable simple_id<counter_type> id_;
};

template<
    class Context,
    class Container,
    class Function,
    std::enable_if_t<
        is_map_like<typename Container::value_type>::value,
        int> = 0>
auto
transform(Context ctx, Container const& container, Function&& f)
{
    typedef typename Container::value_type::key_type key_type;
    typedef typename decltype(
        f(ctx,
          std::declval<readable<key_type>>(),
          std::declval<readable<
              typename Container::value_type::mapped_type>>()))::value_type
        mapped_value_type;

    mapped_map_data<key_type, mapped_value_type>* data;
    get_cached_data(ctx, &data);

    bool all_items_have_values = false;

    ALIA_IF(has_value(container))
    {
        size_t container_size = read_signal(container).size();

        if (!data->input_id.matches(container.value_id()))
        {
            // There's probably a less heavy-handed approach to this, but
            // this works for now.
            data->mapped_items.clear();
            data->item_ids.clear();
            data->item_ids.resize(container_size);
            ++data->output_version;
            data->input_id.capture(container.value_id());
        }

        size_t valid_item_count = 0;
        auto captured_id = data->item_ids.begin();
        for_each(ctx, container, [&](context ctx, auto key, auto value) {
            auto mapped_item = f(ctx, key, value);
            if (signal_has_value(mapped_item))
            {
                if (!captured_id->matches(mapped_item.value_id()))
                {
                    data->mapped_items[read_signal(key)]
                        = read_signal(mapped_item);
                    captured_id->capture(mapped_item.value_id());
                    ++data->output_version;
                }
                ++valid_item_count;
            }
            ++captured_id;
        });
        assert(captured_id == data->item_ids.end());

        all_items_have_values = (valid_item_count == container_size);
    }
    ALIA_END

    return mapped_map_signal<key_type, mapped_value_type>(
        *data, all_items_have_values);
}

} // namespace alia

#endif
