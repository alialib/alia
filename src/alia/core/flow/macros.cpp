#include <alia/core/flow/macros.hpp>

namespace alia {

if_block::if_block(data_traversal& traversal, bool condition)
{
    data_block_node* node;
    get_data_node(traversal, &node, [&] { return new data_block_node; });
    if (condition)
    {
        scoped_data_block_.begin(traversal, node->block);
    }
    else if (traversal.cache_clearing_enabled)
    {
        clear_data_block_cache(node->block);
    }
}

loop_block::loop_block(data_traversal& traversal)
{
    traversal_ = &traversal;
    next();
}
loop_block::~loop_block()
{
    // block_ stores the data block we were expecting to use for the next
    // iteration (and, indirectly, all subsequent iterations), but since the
    // destructor is being invoked, there won't be a next iteration, which
    // means we should clear out that block.
    if (!exception_detector_.detect())
        clear_data_block(*block_);
}
void
loop_block::next()
{
    data_block_node* node;
    get_data_node(*traversal_, &node, [&] { return new data_block_node; });
    block_ = &node->block;
}

event_dependent_if_block::event_dependent_if_block(
    data_traversal& traversal, bool condition)
{
    data_block_node* node;
    get_data_node(traversal, &node, [&] { return new data_block_node; });
    if (condition)
    {
        scoped_data_block_.begin(traversal, node->block);
    }
}

} // namespace alia
