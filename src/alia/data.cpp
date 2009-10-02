#include <alia/data.hpp>
#include <alia/context.hpp>
#include <alia/flags.hpp>
#include <map>
#include <vector>

namespace alia {

typedef std::map<id_ref,dynamic_block_node*> naming_map_type;
struct naming_map : boost::noncopyable
{
    naming_map() : next(0), prev(0) {}
    ~naming_map();
    naming_map_type map;
    context* ctx;
    naming_map* next;
    naming_map* prev;
};

struct dynamic_block_node : boost::noncopyable
{
    dynamic_block_node() : context(0), anchor(0) {}
    ~dynamic_block_node() { if (context) context->map.erase(id_ref(id.id)); }
    data_block block;
    owned_id id;
    naming_map* context;
    data_block* anchor;
    uint64 last_used;
    bool manual;
    dynamic_block_node* next;
    dynamic_block_node* prev;
};

naming_map::~naming_map()
{
    // delete/orphan all blocks associated with this map
    while (!map.empty())
    {
        naming_map_type::iterator i = map.begin();
        dynamic_block_node* node = i->second;
        node->context = 0;
        map.erase(i);
        if (!node->anchor)
            delete node;
        else
            node->manual = false;
    }
    // remove from context's map list
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    else
        ctx->map_list = next;
}

static void clear_block_list(dynamic_block_node* blocks)
{
    dynamic_block_node* i = blocks;
    while (i)
    {
        dynamic_block_node* next = i->next;
        if (i->manual)
        {
            assert(i->context);
            i->anchor = 0;
        }
        else
            delete i;
        i = next;
    }
}

data_block::data_block()
  : last_refresh(0), nodes(0), unused_blocks(0), used_blocks(0),
    first_used_block(0)
{
}
data_block::~data_block()
{
    delete nodes;
    clear_block_list(unused_blocks);
    clear_block_list(used_blocks);
}
data_block* get_data_block(context& ctx)
{
    return get_data<data_block>(ctx);
}

static void activate_data_block(context& ctx, data_block& block)
{
    ctx.pass_state.active_block = &block;
    ctx.pass_state.predicted_dynamic_block = block.first_used_block;
    ctx.pass_state.next_data_ptr = &block.nodes;

    // delete unused blocks and make all the used ones unused for this pass
    if (block.last_refresh != ctx.refresh_counter)
    {
        clear_block_list(block.unused_blocks);
        block.unused_blocks = block.used_blocks;
        block.used_blocks = 0;
        block.first_used_block = 0;
        block.last_refresh = ctx.refresh_counter;
    }
}

void scoped_data_block::begin(context& ctx, data_block& block)
{
    ctx_ = &ctx;
    old_block_ = ctx.pass_state.active_block;
    old_predicted_ = ctx.pass_state.predicted_dynamic_block;
    old_next_ptr_ = ctx.pass_state.next_data_ptr;
    activate_data_block(ctx, block);
    active_ = true;
}
void scoped_data_block::end()
{
    if (active_)
    {
        ctx_->pass_state.active_block = old_block_;
        ctx_->pass_state.predicted_dynamic_block = old_predicted_;
        ctx_->pass_state.next_data_ptr = old_next_ptr_;
        active_ = false;
    }
}

void naming_context::begin(context& ctx)
{
    ctx_ = &ctx;
    old_map_ = ctx.pass_state.active_map;
    naming_map* new_map;
    if (get_data<naming_map>(ctx, &new_map))
    {
        new_map->ctx = &ctx;
        new_map->next = ctx.map_list;
        if (ctx.map_list)
            ctx.map_list->prev = new_map;
        new_map->prev = 0;
        ctx.map_list = new_map;
    }
    ctx.pass_state.active_map = new_map;
    active_ = true;
}
void naming_context::end()
{
    if (active_)
    {
        ctx_->pass_state.active_map = old_map_;
        active_ = false;
    }
}

static void record_usage(context& ctx, dynamic_block_node* node,
    unsigned flags)
{
    node->last_used = ctx.refresh_counter;

    // remove from unused list of old anchor
    data_block* old_anchor = node->anchor;
    if (old_anchor)
    {
        ctx.pass_state.predicted_dynamic_block = node->prev;
        if (node->prev)
        {
            assert(old_anchor->unused_blocks != node);
            node->prev->next = node->next;
        }
        else
        {
            assert(old_anchor->unused_blocks == node);
            old_anchor->unused_blocks = node->next;
        }
        if (node->next)
            node->next->prev = node->prev;
    }

    // add to used list of new anchor
    data_block* new_anchor = ctx.pass_state.active_block;
    node->anchor = new_anchor;
    node->manual = (flags & MANUAL_DELETE) != 0;
    node->next = new_anchor->used_blocks;
    if (node->next)
        node->next->prev = node;
    node->prev = 0;
    new_anchor->used_blocks = node;
    if (!new_anchor->first_used_block)
        new_anchor->first_used_block = node;
}

static dynamic_block_node* find_dynamic_block(context& ctx,
    id_interface const& id, unsigned flags)
{
    dynamic_block_node* predicted = ctx.pass_state.predicted_dynamic_block;
    if (predicted && id_ref(predicted->id.id) == id_ref(&id) &&
        predicted->context == ctx.pass_state.active_map)
    {
        assert(predicted->anchor == ctx.pass_state.active_block);
        assert(predicted->context == ctx.pass_state.active_map);
        if (predicted->last_used != ctx.refresh_counter)
            record_usage(ctx, predicted, flags);
        return predicted;
    }

    naming_map_type::const_iterator i =
        ctx.pass_state.active_map->map.find(id_ref(&id));
    if (i != ctx.pass_state.active_map->map.end())
    {
        dynamic_block_node* node = i->second;
        assert(node);
        assert(node->context == ctx.pass_state.active_map);
        if (node->last_used != ctx.refresh_counter)
            record_usage(ctx, node, flags);
        return i->second;
    }

    dynamic_block_node* new_node = new dynamic_block_node;
    new_node->context = ctx.pass_state.active_map;
    new_node->id.id = id.clone();
    record_usage(ctx, new_node, flags);
    ctx.pass_state.active_map->map.insert(naming_map_type::value_type(
        id_ref(new_node->id.id), new_node));
    return new_node;
}

void named_block::begin_impl(context& ctx, id_interface const& id,
    unsigned flags)
{
    scoped_data_block_.begin(ctx, find_dynamic_block(ctx, id, flags)->block);
}
void named_block::end()
{
    scoped_data_block_.end();
}

void delete_named_data_impl(context& ctx, id_interface const& id)
{
    for (naming_map* i = ctx.map_list; i; i = i->next)
    {
        naming_map_type::const_iterator j = i->map.find(id_ref(&id));
        if (j != i->map.end())
        {
            dynamic_block_node* node = j->second;
            if (!node->anchor)
            {
                i->map.erase(j);
                delete node;
            }
            else
                node->manual = false;
        }
    }
}

struct switch_block_data
{
    switch_block_data() : blocks(0), size(0) {}
    ~switch_block_data()
    {
        for (unsigned i = 0; i < size; ++i)
            delete blocks[i];
        delete[] blocks;
    }
    data_block** blocks;
    unsigned size;
};
switch_block::switch_block(context& ctx)
{
    ctx_ = &ctx;
    get_data(ctx, &data_);
    old_block_ = ctx.pass_state.active_block;
    old_predicted_ = ctx.pass_state.predicted_dynamic_block;
    old_next_ptr_ = ctx.pass_state.next_data_ptr;
}
switch_block::~switch_block()
{
    ctx_->pass_state.active_block = old_block_;
    ctx_->pass_state.predicted_dynamic_block = old_predicted_;
    ctx_->pass_state.next_data_ptr = old_next_ptr_;
}
void switch_block::activate_case(unsigned i)
{
    if (i >= data_->size)
    {
        unsigned new_size = i + 1;
        data_block** new_blocks = new data_block*[new_size];
        for (unsigned j = 0; j < data_->size; ++j)
            new_blocks[j] = data_->blocks[j];
        for (unsigned j = data_->size; j < new_size; ++j)
            new_blocks[j] = 0;
        delete[] data_->blocks;
        data_->blocks = new_blocks;
        data_->size = new_size;
    }
    if (!data_->blocks[i])
        data_->blocks[i] = new data_block;
    activate_data_block(*ctx_, *data_->blocks[i]);
}

}
