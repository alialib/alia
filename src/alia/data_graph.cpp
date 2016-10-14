#include <alia/data_graph.hpp>
#include <map>
#include <vector>

#ifndef _WIN32
// ignore warnings for GCC
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

namespace alia {

// Using a reference type as the key for a map causes problems, so we need to
// create a structure that will serve as an ID reference.
struct id_reference
{
    id_reference() {}
    explicit id_reference(id_interface const& id)
      : id(&id)
    {}
    id_interface const* id;
};
static bool operator==(id_reference a, id_reference b)
{ return *a.id == *b.id; }
static bool operator!=(id_reference a, id_reference b)
{ return *a.id != *b.id; }
static bool operator<(id_reference a, id_reference b)
{ return *a.id < *b.id; }

struct named_block_node;

struct naming_map
{
    typedef std::map<id_reference,named_block_node*> map_type;
    map_type blocks;
};

struct named_block_node : noncopyable
{
    named_block_node()
      : reference_count(0), active_count(0), manual_delete(false), map(0)
    {}

    // the actual data block
    data_block block;

    // the ID of the block
    owned_id id;

    // count of references to this block by data_blocks
    int reference_count;

    // count of active data blocks that are currently referencing this block
    int active_count;

    // If this is set to true, the block is also owned by its map and will
    // persist until it's manually deleted (or the map is destroyed).
    bool manual_delete;

    // the naming map that this block belongs to
    // Since the block may not be owned by the map that contains it, it needs
    // to know where that map is so it can remove itself if it's no longer
    // needed.
    naming_map* map;
};

// naming_maps are always created via a naming_map_node, which takes care of
// associating them with the data_graph.
struct naming_map_node : noncopyable
{
    naming_map_node() : next(0), prev(0) {}
    ~naming_map_node();

    naming_map map;

    // the graph that this node belongs to
    data_graph* graph;

    // doubly linked list pointers
    naming_map_node* next;
    naming_map_node* prev;
};
naming_map_node::~naming_map_node()
{
    // Remove the association between any named_blocks left in the map and
    // the map itself.
    for (naming_map::map_type::const_iterator i = map.blocks.begin();
        i != map.blocks.end(); ++i)
    {
        named_block_node* node = i->second;
        if (node->reference_count == 0)
            delete node;
        else
            node->map = 0;
    }

    // Remove this node from graph's map list.
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    else
        graph->map_list = next;
}

// named_block_ref_nodes are stored as lists within data_blocks to hold
// references to the named_block_nodes that occur within that block.
// A named_block_ref_node provides ownership of the referenced node.
struct named_block_ref_node : noncopyable
{
    named_block_ref_node()
      : node(0), next(0)
    {}

    ~named_block_ref_node()
    {
        if (node)
        {
            --node->reference_count;
            if (!node->reference_count)
            {
                if (node->map)
                {
                    if (!node->manual_delete)
                    {
                        node->map->blocks.erase(id_reference(node->id.get()));
                        delete node;
                    }
                    else
                        clear_cached_data(node->block);
                }
                else
                    delete node;
            }
        }
    }

    // referenced node
    named_block_node* node;

    // is this reference contributing to the active count in the node?
    bool active;

    // next node in linked list
    named_block_ref_node* next;
};

static void activate(named_block_ref_node& ref)
{
    if (!ref.active)
    {
        ++ref.node->active_count;
        ref.active = true;
    }
}
static void deactivate(named_block_ref_node& ref)
{
    if (ref.active)
    {
        --ref.node->active_count;
        if (!ref.node->active_count)
            clear_cached_data(ref.node->block);
        ref.active = false;
    }
}

static void delete_named_block_ref_list(named_block_ref_node* head)
{
    while (head)
    {
        named_block_ref_node* next = head->next;
        delete head;
        head = next;
    }
}

// Clear all cached data stored in the subgraph referenced from the given node
// list.
static void clear_cached_data(data_node* nodes)
{
    for (data_node* i = nodes; i; i = i->next)
    {
        // If this node is cached data, clear it.
        typed_data_node<cached_data_holder>* caching_node =
            dynamic_cast<typed_data_node<cached_data_holder>*>(i);
        if (caching_node)
        {
            cached_data_holder& holder = caching_node->value;
            delete holder.data;
            holder.data = 0;
        }

        // If this node is a data block, clear cached data from it.
        typed_data_node<data_block>* block =
            dynamic_cast<typed_data_node<data_block>*>(i);
        if (block)
            clear_cached_data(block->value);
    }
}

void clear_cached_data(data_block& block)
{
    if (!block.cache_clear)
    {
        clear_cached_data(block.nodes);
        for (named_block_ref_node* i = block.named_blocks; i; i = i->next)
            deactivate(*i);
        block.cache_clear = true;
    }
}

data_block::data_block()
  : nodes(0), cache_clear(true), named_blocks(0)
{
}
data_block::~data_block()
{
    clear_data_block(*this);
}

void clear_data_block(data_block& block)
{
    data_node* node = block.nodes;
    while (node)
    {
        data_node* next = node->next;
        delete node;
        node = next;
    }
    block.nodes = 0;

    delete_named_block_ref_list(block.named_blocks);
    block.named_blocks = 0;

    block.cache_clear = true;
}

void scoped_data_block::begin(data_traversal& traversal, data_block& block)
{
    traversal_ = &traversal;

    old_active_block_ = traversal.active_block;
    old_predicted_named_block_ = traversal.predicted_named_block;
    old_used_named_blocks_ = traversal.used_named_blocks;
    old_named_block_next_ptr_ = traversal.named_block_next_ptr;
    old_next_data_ptr_ = traversal.next_data_ptr;

    traversal.active_block = &block;
    traversal.predicted_named_block = block.named_blocks;
    traversal.used_named_blocks = 0;
    traversal.named_block_next_ptr = &traversal.used_named_blocks;
    traversal.next_data_ptr = &block.nodes;

    block.cache_clear = false;
}
void scoped_data_block::end()
{
    if (traversal_)
    {
        // If GC is enabled, record which named blocks were used and clear out
        // the unused ones.
        if (traversal_->gc_enabled && !traversal_->traversal_aborted)
        {
            traversal_->active_block->named_blocks =
                traversal_->used_named_blocks;
            delete_named_block_ref_list(traversal_->predicted_named_block);
        }

        traversal_->active_block = old_active_block_;
        traversal_->predicted_named_block = old_predicted_named_block_;
        traversal_->used_named_blocks = old_used_named_blocks_;
        traversal_->named_block_next_ptr = old_named_block_next_ptr_;
        traversal_->next_data_ptr = old_next_data_ptr_;

        traversal_ = 0;
    }
}

naming_map* retrieve_naming_map(data_traversal& traversal)
{
    naming_map_node* map_node;
    if (get_data(traversal, &map_node))
    {
        data_graph& graph = *traversal.graph;
        map_node->graph = &graph;
        map_node->next = graph.map_list;
        if (graph.map_list)
            graph.map_list->prev = map_node;
        map_node->prev = 0;
        graph.map_list = map_node;
    }
    return &map_node->map;
}

void naming_context::begin(data_traversal& traversal)
{
    traversal_ = &traversal;
    map_ = retrieve_naming_map(traversal);
}

static void record_usage(data_traversal& traversal, named_block_ref_node* ref)
{
    *traversal.named_block_next_ptr = ref;
    traversal.named_block_next_ptr = &ref->next;
    ref->next = 0;
    activate(*ref);
}

static named_block_node* find_named_block(
    data_traversal& traversal, naming_map& map, id_interface const& id,
    manual_delete manual)
{
    // If the sequence of data requests is the same as last pass (which it
    // generally is), then the block we're looking for is the predicted one.
    named_block_ref_node* predicted = traversal.predicted_named_block;
    if (predicted && predicted->node->id.get() == id &&
        predicted->node->map == &map)
    {
        traversal.predicted_named_block = predicted->next;
        if (traversal.gc_enabled)
            record_usage(traversal, predicted);
        return predicted->node;
    }

    if (!traversal.gc_enabled)
        throw named_block_out_of_order();

    // Otherwise, look it up in the map.
    naming_map::map_type::const_iterator i = map.blocks.find(id_reference(id));

    // If it's not already in the map, create it and insert it.
    if (i == map.blocks.end())
    {
        named_block_node* new_node = new named_block_node;
        new_node->id.store(id);
        new_node->map = &map;
        new_node->manual_delete = manual.value;
        i = map.blocks.insert(naming_map::map_type::value_type(
            id_reference(new_node->id.get()), new_node)).first;
    }

    named_block_node* node = i->second;
    assert(node && node->map == &map);

    // Create a new reference node to record the node's usage within this
    // data_block.
    // TODO: Allocate these from a pool.
    named_block_ref_node* ref = new named_block_ref_node;
    ref->node = node;
    ref->active = false;
    ++node->reference_count;
    record_usage(traversal, ref);

    return node;
}

void named_block::begin(data_traversal& traversal, naming_map& map,
    id_interface const& id, manual_delete manual)
{
    scoped_data_block_.begin(traversal,
        find_named_block(traversal, map, id, manual)->block);
}
void named_block::end()
{
    scoped_data_block_.end();
}

void delete_named_block(data_graph& graph, id_interface const& id)
{
    for (naming_map_node* i = graph.map_list; i; i = i->next)
    {
        naming_map::map_type::const_iterator j =
            i->map.blocks.find(id_reference(id));
        if (j != i->map.blocks.end())
        {
            named_block_node* node = j->second;
            // If the reference count is nonzero, the block is still active,
            // so we don't want to delete it. We just want to clear the
            // manual_delete flag.
            if (node->reference_count != 0)
            {
                node->manual_delete = false;
            }
            else
            {
                i->map.blocks.erase(id_reference(id));
                node->map = 0;
                delete node;
            }
        }
    }
}

void scoped_gc_disabler::begin(data_traversal& traversal)
{
    traversal_ = &traversal;
    old_gc_state_ = traversal.gc_enabled;
    traversal.gc_enabled = false;
}
void scoped_gc_disabler::end()
{
    if (traversal_)
    {
        traversal_->gc_enabled = old_gc_state_;
        traversal_ = 0;
    }
}

void scoped_cache_clearing_disabler::begin(data_traversal& traversal)
{
    traversal_ = &traversal;
    old_cache_clearing_state_ = traversal.cache_clearing_enabled;
    traversal.cache_clearing_enabled = false;
}
void scoped_cache_clearing_disabler::end()
{
    if (traversal_)
    {
        traversal_->cache_clearing_enabled = old_cache_clearing_state_;
        traversal_ = 0;
    }
}

void scoped_data_traversal::begin(data_graph& graph, data_traversal& traversal)
{
    traversal.graph = &graph;
    traversal.traversal_aborted = false;
    traversal.gc_enabled = true;
    traversal.cache_clearing_enabled = true;
    root_block_.begin(traversal, graph.root_block);
    root_map_.begin(traversal);
}
void scoped_data_traversal::end()
{
    root_block_.end();
    root_map_.end();
}

if_block::if_block(data_traversal& traversal, bool condition)
{
    data_block* block;
    get_data(traversal, &block);
    if (condition)
    {
        scoped_data_block_.begin(traversal, *block);
    }
    else
    {
        if (traversal.cache_clearing_enabled)
            clear_cached_data(*block);
    }
}

pass_dependent_if_block::pass_dependent_if_block(
    data_traversal& traversal, bool condition)
{
    data_block* block;
    get_data(traversal, &block);
    if (condition)
    {
        scoped_data_block_.begin(traversal, *block);
    }
}

loop_block::loop_block(data_traversal& traversal)
{
    traversal_ = &traversal;
    get_data(traversal, &block_);
}
loop_block::~loop_block()
{
    if (traversal_->cache_clearing_enabled)
        clear_cached_data(*block_);
}
void loop_block::next()
{
    get_data(*traversal_, &block_);
}

}
