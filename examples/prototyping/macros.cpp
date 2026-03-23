#include <iostream>

struct Block
{
    size_t index;
    size_t memoized_size;
};

Block blocks[10];
size_t block_index = 0;

Block* active_block = nullptr;

void
reset_blocks()
{
    block_index = 0;
    active_block = nullptr;
}

Block&
use_block()
{
    return blocks[block_index++];
}

Block*
get_active_block()
{
    return active_block;
}

void
activate_block(Block& b)
{
    std::cout << "activate_block " << b.index << std::endl;
    active_block = &b;
}

int
main()
{
    for (int i = 0; i < 10; ++i)
    {
        reset_blocks();
        activate_block(use_block());
        alia_if (ctx, i % 2 == 0)
        {
            std::cout << "true " << the_pass_number << std::endl;
        }
        alia_else(ctx)
        {
            std::cout << "false " << the_pass_number << std::endl;
        }
        alia_end
    }
    return 0;
}
