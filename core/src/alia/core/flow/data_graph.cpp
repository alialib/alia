#include <alia/core/flow/data_graph.hpp>
#include <map>
#include <vector>

namespace alia {

struct named_block_node
{
    // the actual data block used for the content
    data_block content_block;

    // the ID of the block
    captured_id id;

    // pointers in the linked list of nodes defining their traversal order
    named_block_node* next = nullptr;
    named_block_node* prev = nullptr;

    // If this is set to true, the block must be manually deleted
    bool manual_delete = false;
};

// Remove a named_block_node from a doubly-linked list.
// This can be called on a node that's not currently in any lists, in which
// case it's a noop.
static void
remove_from_list(named_block_node*& head, named_block_node* node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    else if (head == node)
        head = node->next;
    node->next = nullptr;
    node->prev = nullptr;
}

static void
append_to_list(
    named_block_node*& head,
    named_block_node*& tail,
    named_block_node* new_node)
{
    assert(!new_node->next && !new_node->prev);
    if (tail)
    {
        tail->next = new_node;
        new_node->prev = tail;
    }
    else
        head = new_node;
    tail = new_node;
}

struct naming_map
{
    // a map from 'names' to the associated blocks
    typedef std::map<
        id_interface const*,
        named_block_node,
        id_interface_pointer_less_than_test>
        map_type;
    map_type blocks;

    // list of named blocks referenced from this data block, in the order that
    // they appear in the traversal - Note that not all blocks necessarily
    // appear in this list. If a node requires manual deletion, it can be
    // absent from this list but still living in the map.
    named_block_node* first = nullptr;
};

// naming_maps are always created via a naming_map_node, which takes care of
// associating them with the data graph.
struct naming_map_node : data_node
{
    naming_map map;

    // the graph that this node belongs to
    data_graph* graph = nullptr;

    // doubly linked list pointers
    naming_map_node* next = nullptr;
    naming_map_node* prev = nullptr;

    ~naming_map_node();

    void
    clear_cache();
};

naming_map_node::~naming_map_node()
{
    // Remove this node from the graph's map list.
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    else
        graph->map_list = next;
}

void
naming_map_node::clear_cache()
{
    // Clear out all the caches in the individual data blocks.
    for (auto& i : this->map.blocks)
        clear_data_block_cache(i.second.content_block);
}

static void
release_named_block_node(naming_map& map, named_block_node* node)
{
    if (!node->manual_delete)
    {
        map.blocks.erase(&*node->id);
    }
    else
    {
        clear_data_block_cache(node->content_block);
        node->prev = node->next = nullptr;
    }
}

static void
release_named_block_node_list(naming_map& map, named_block_node* head)
{
    // Unlike with data blocks, the relative destruction order doesn't matter
    // here, so just use a normal loop.
    named_block_node* node = head;
    while (node)
    {
        named_block_node* next = node->next;
        release_named_block_node(map, node);
        node = next;
    }
}

static void
clear_data_node_caches(data_node* node)
{
    // Clear node caches in reverse order to match general C++ semantics.
    // Note that the same concerns exist here as in delete_data_nodes().
    if (node)
    {
        clear_data_node_caches(node->alia_next_data_node_);
        node->clear_cache();
    }
}

void
clear_data_block_cache(data_block& block)
{
    if (!block.cache_clear)
    {
        clear_data_node_caches(block.nodes);
        block.cache_clear = true;
    }
}

static void
delete_data_nodes(data_node* node)
{
    // Delete nodes in reverse order to match general C++ semantics.
    //
    // Note that this is a potential stack overflow waiting to happen.
    // It might be worth storing a doubly linked list of nodes so that we can
    // directly traverse them in reverse order. Of course, it's also worth
    // considering alternate data structures that would reduce allocations and
    // fragmentation, so I'm leaving this for the future...
    //
    if (node)
    {
        delete_data_nodes(node->alia_next_data_node_);
        delete node;
    }
}

void
clear_data_block(data_block& block)
{
    // This isn't strictly necessary, but it gives nodes a chance to remove
    // themselves in a more explicit manner before they find themselves in a
    // state of destruction.
    clear_data_block_cache(block);

    delete_data_nodes(block.nodes);
    block.nodes = nullptr;
}

data_block::~data_block()
{
    clear_data_block(*this);
}

void
scoped_data_block::begin(data_traversal& traversal, data_block& block)
{
    traversal_ = &traversal;

    old_active_block_ = traversal.active_block;
    old_next_data_ptr_ = traversal.next_data_ptr;

    traversal.active_block = &block;
    traversal.next_data_ptr = &block.nodes;

    block.cache_clear = false;
}
void
scoped_data_block::end()
{
    if (traversal_)
    {
        traversal_->active_block = old_active_block_;
        traversal_->next_data_ptr = old_next_data_ptr_;
        traversal_ = nullptr;
    }
}

naming_map*
retrieve_naming_map(data_traversal& traversal)
{
    naming_map_node* map_node;
    if (get_data_node(
            traversal, &map_node, [&] { return new naming_map_node; }))
    {
        data_graph& graph = *traversal.graph;
        map_node->graph = &graph;
        map_node->next = graph.map_list;
        if (graph.map_list)
            graph.map_list->prev = map_node;
        map_node->prev = nullptr;
        graph.map_list = map_node;
    }
    return &map_node->map;
}

void
naming_context::begin(data_traversal& traversal)
{
    data_traversal_ = &traversal;

    map_ = retrieve_naming_map(traversal);

    inner_traversal_.predicted = map_->first;
}

void
naming_context::end()
{
    // If GC is enabled, record which named blocks were used and clear out
    // the unused ones.
    if (data_traversal_->gc_enabled)
    {
        auto& traversal = inner_traversal_;
        // If we're just unwinding the stack due to an exception, we don't
        // want to GC unseen nodes, but we also need to keep them as "expected"
        // nodes for next time, so we just append them to the list of seen
        // nodes.
        if (exception_detector_.detect())
        {
            // We only have to worry about the case where a divergence has been
            // detected. If none has been detected yet, then we haven't
            // actually touched any pointers, so we're all set.
            if (traversal.divergence_detected)
            {
                if (traversal.last_seen)
                {
                    traversal.last_seen->next = traversal.predicted;
                    if (traversal.predicted)
                        traversal.predicted->prev = traversal.last_seen;
                }
                else
                {
                    traversal.seen_blocks = traversal.predicted;
                    if (traversal.predicted)
                        traversal.predicted->prev = nullptr;
                }
            }
            map_->first = traversal.seen_blocks;
        }
        else
        {
            if (traversal.divergence_detected)
            {
                // The lists are already separate, so just release all
                // remaining nodes from the 'predicted' list.
                release_named_block_node_list(*map_, traversal.predicted);
                map_->first = traversal.seen_blocks;
            }
            else
            {
                // Everything that we saw was in the order it was supposed to
                // be, but we still have to check that we've seen everything
                // we predicted we'd see.
                if (traversal.predicted)
                {
                    release_named_block_node_list(*map_, traversal.predicted);
                    if (traversal.last_seen)
                        traversal.last_seen->next = nullptr;
                    else
                        map_->first = nullptr;
                }
            }
        }
    }
}

static named_block_node*
find_named_block(
    data_traversal& data_traversal,
    naming_context_traversal_state& traversal,
    naming_map& map,
    id_interface const& id,
    manual_delete manual)
{
    if (!traversal.divergence_detected)
    {
        // If the order continues to match what we expected, just continue
        // advancing the pointers.
        named_block_node* predicted = traversal.predicted;
        if (predicted && predicted->id.matches(id))
        {
            traversal.predicted = predicted->next;
            traversal.last_seen = predicted;
            return predicted;
        }

        // The predicted block wasn't the one that we actually saw, so a
        // divergence has just been detected...

        // The order of named blocks isn't allowed to change on non-GC
        // passes.
        if (!data_traversal.gc_enabled)
            throw named_block_out_of_order();

        // We have to split our single list of nodes into two lists: seen and
        // (as yet) unseen.
        if (traversal.last_seen)
        {
            traversal.seen_blocks = map.first;
            traversal.last_seen->next = nullptr;
        }
        if (predicted)
        {
            predicted->prev = nullptr;
        }

        // Record the divergence.
        traversal.divergence_detected = true;
    }

    // Otherwise, we've diverged from the predicted order, so look up
    // the node in the map.
    naming_map::map_type::iterator i = map.blocks.find(&id);

    // If it's not already in the map, create it and insert it.
    if (i == map.blocks.end())
    {
        named_block_node new_node;
        new_node.id.capture(id);
        new_node.manual_delete = manual.value;
        id_interface const* new_id = &*new_node.id;
        i = map.blocks.emplace(new_id, std::move(new_node)).first;
    }

    named_block_node* node = &i->second;

    // If the node is currently in a list, it must be in the predicted
    // list, so remove it from there.
    remove_from_list(traversal.predicted, node);
    // And add it to the list of blocks that have been seen.
    append_to_list(traversal.seen_blocks, traversal.last_seen, node);

    return node;
}

void
named_block::begin(
    naming_context& nc, id_interface const& id, manual_delete manual)
{
    scoped_data_block_.begin(
        *nc.data_traversal_,
        find_named_block(
            *nc.data_traversal_, nc.inner_traversal_, *nc.map_, id, manual)
            ->content_block);
}
void
named_block::end()
{
    scoped_data_block_.end();
}

void
delete_named_block(data_graph& graph, id_interface const& id)
{
    for (naming_map_node* i = graph.map_list; i; i = i->next)
    {
        naming_map::map_type::iterator j = i->map.blocks.find(&id);
        if (j != i->map.blocks.end())
        {
            named_block_node* node = &j->second;
            // If the block is still active, so we don't want to delete it. We
            // just want to clear the manual_delete flag.
            if (node->next || i->map.first == node)
                node->manual_delete = false;
            else
                i->map.blocks.erase(&id);
        }
    }
}

void
disable_gc(data_traversal& traversal)
{
    traversal.gc_enabled = false;
}

void
scoped_cache_clearing_disabler::begin(data_traversal& traversal)
{
    traversal_ = &traversal;
    old_cache_clearing_state_ = traversal.cache_clearing_enabled;
    traversal.cache_clearing_enabled = false;
}
void
scoped_cache_clearing_disabler::end()
{
    if (traversal_)
    {
        traversal_->cache_clearing_enabled = old_cache_clearing_state_;
        traversal_ = nullptr;
    }
}

void
scoped_data_traversal::begin(data_graph& graph, data_traversal& traversal)
{
    traversal.graph = &graph;
    traversal.gc_enabled = true;
    traversal.cache_clearing_enabled = true;
    root_block_.begin(traversal, graph.root_block);
    root_map_.begin(traversal);
}
void
scoped_data_traversal::end()
{
    root_block_.end();
    root_map_.end();
}

} // namespace alia
