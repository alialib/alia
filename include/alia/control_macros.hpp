#ifndef ALIA_CONTROL_MACROS_HPP
#define ALIA_CONTROL_MACROS_HPP

// if, else_if, else

#define alia_if_(ctx, c) \
    { \
        bool alia__else_condition; \
        { \
            ::alia::context& alia__ctx = (ctx); \
            ::alia::data_block* alia__data_block = \
                ::alia::get_data_block(alia__ctx); \
            bool const alia__condition = (c) ? true : false; \
            alia__else_condition = !alia__condition; \
            if (alia__condition) \
            { \
                ::alia::scoped_data_block alia__scoped_block(alia__ctx, \
                    *alia__data_block);

#define alia_if(c) alia_if_(ctx, c)

#define alia_else_if_(ctx, c) \
            } \
        } \
        { \
            ::alia::context& alia__ctx = (ctx); \
            ::alia::data_block* alia__data_block = \
                ::alia::get_data_block(alia__ctx); \
            bool alia__condition; \
            if (alia__else_condition) \
            { \
                alia__condition = (c) ? true : false; \
                alia__else_condition = !alia__condition; \
            } \
            else \
                alia__condition = false; \
            if (alia__condition) \
            { \
                ::alia::scoped_data_block alia__scoped_block(alia__ctx, \
                    *alia__data_block);

#define alia_else_if(c) alia_else_if_(ctx, c)

#define alia_else_(ctx) \
            } \
        } \
        { \
            ::alia::context& alia__ctx = (ctx); \
            ::alia::data_block* alia__data_block = \
                ::alia::get_data_block(alia__ctx); \
            if (alia__else_condition) \
            { \
                ::alia::scoped_data_block alia__scoped_block(alia__ctx, \
                    *alia__data_block);

#define alia_else alia_else_(ctx)

// switch

#define alia_switch_(ctx, x) \
    {{{ \
        ::alia::switch_block alia__switch_block(ctx); \
        switch(x)

#define alia_switch(x) alia_switch_(ctx, x)

#define alia_case(c) \
            alia__switch_block.activate_case(c)

// for

#define alia_for_(ctx, x) \
    {{ \
        ::alia::context& alia__ctx = (ctx); \
        ::alia::data_block* alia__data_block = \
            ::alia::get_data_block(alia__ctx); \
        for (x) \
        { \
            ::alia::scoped_data_block alia__scoped_block(alia__ctx, \
                *alia__data_block); \
            alia__data_block = \
                ::alia::get_data<::alia::data_block>(alia__ctx);

#define alia_for(x) alia_for_(ctx, x)

// while

#define alia_while_(ctx, x) \
    {{ \
        ::alia::context& alia__ctx = (ctx); \
        ::alia::data_block* alia__data_block = \
            ::alia::get_data_block(alia__ctx); \
        while (x) \
        { \
            ::alia::scoped_data_block alia__scoped_block(alia__ctx, \
                *alia__data_block); \
            alia__data_block = \
                ::alia::get_data<::alia::data_block>(alia__ctx);

#define alia_while(x) alia_while_(ctx, x)

// end

#define alia_end \
    }}}

#endif
