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

template<typename Tag>
inline size_t&
get_memoized_size()
{
    static size_t value = 0;
    return value;
}

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define UNIQUE_ID(base) CONCAT(base, __COUNTER__)

// condition_is_true(x) evaluates x in a boolean context.
template<class T>
bool
condition_is_true(T x)
{
    return x ? true : false;
}

template<class T>
bool
condition_is_false(T x)
{
    return x ? false : true;
}

// MSVC likes to warn about shadowed local variables and function parameters,
// but there's really no way to implement these macros without potentially
// shadowing things, so we need to disable those warning within the macros.
// Similarly for Clang warning about unused variables.
#if defined(__clang__)
#define ALIA_DISABLE_MACRO_WARNINGS                                           \
    _Pragma("clang diagnostic push")                                          \
        _Pragma("clang diagnostic ignored \"-Wunknown-warning-option\"")      \
            _Pragma("clang diagnostic ignored \"-Wunused-but-set-variable\"")
#define ALIA_REENABLE_MACRO_WARNINGS _Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
#define ALIA_DISABLE_MACRO_WARNINGS                                           \
    __pragma(warning(push)) __pragma(warning(disable : 4456))                 \
        __pragma(warning(disable : 4457))
#define ALIA_REENABLE_MACRO_WARNINGS __pragma(warning(pop))
#else
#define ALIA_DISABLE_MACRO_WARNINGS
#define ALIA_REENABLE_MACRO_WARNINGS
#endif

struct IfBlock
{
    bool is_true, is_false;
    Block* parent_block;
    bool first_pass;

    template<class Condition>
    IfBlock(Condition condition)
    {
        is_true = condition_is_true(condition);
        is_false = condition_is_false(condition);
        parent_block = get_active_block();
        first_pass = true;
    }

    void
    push_block()
    {
        activate_block(use_block());
    }

    void
    restore()
    {
        activate_block(*parent_block);
    }

    void
    advance()
    {
        restore();
        first_pass = false;
    }

    bool
    done()
    {
        return !first_pass;
    }
};

int the_pass_number = 0;

struct InnerIf
{
    size_t& memo_size_;
    InnerIf(size_t& memo_size) : memo_size_(memo_size)
    {
        the_pass_number = memo_size_ == 0 ? 0 : 1;
    }

    void
    advance()
    {
        if (the_pass_number == 0)
            memo_size_ = 1;
        ++the_pass_number;
    }
};

#define alia_if(ctx, condition)                                               \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    for (auto _alia_if_block = IfBlock(condition); !_alia_if_block.done();    \
         _alia_if_block.advance())                                            \
        if (_alia_if_block.is_true)                                           \
            for (auto _alia_inner_if                                          \
                 = InnerIf(get_memoized_size<struct UNIQUE_ID(Memo)>());      \
                 the_pass_number < 2;                                         \
                 _alia_inner_if.advance())                                    \
    ALIA_REENABLE_MACRO_WARNINGS

#define alia_else(ctx)                                                        \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    else if (_alia_if_block                                                   \
                 .is_false) for (auto _alia_inner_if                          \
                                 = InnerIf(                                   \
                                     get_memoized_size<struct UNIQUE_ID(      \
                                         Memo)>());                           \
                                 the_pass_number < 2;                         \
                                 _alia_inner_if.advance())                    \
        ALIA_REENABLE_MACRO_WARNINGS

#define alia_end

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
