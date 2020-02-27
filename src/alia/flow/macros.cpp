#include <alia/flow/macros.hpp>

namespace alia {

if_block::if_block(data_traversal& traversal, bool condition)
{
    data_block& block = get_data<data_block>(traversal);
    if (condition)
    {
        scoped_data_block_.begin(traversal, block);
    }
    else if (traversal.cache_clearing_enabled)
    {
        clear_cached_data(block);
    }
}

loop_block::loop_block(data_traversal& traversal)
{
    traversal_ = &traversal;
    get_data(traversal, &block_);
}
loop_block::~loop_block()
{
    // The current block is the one we were expecting to use for the next
    // iteration, but since the destructor is being invoked, there won't be a
    // next iteration, which means we should clear out that block.
    if (!std::uncaught_exception())
        clear_data_block(*block_);
}
void
loop_block::next()
{
    get_data(*traversal_, &block_);
}

} // namespace alia
