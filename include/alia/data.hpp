#ifndef ALIA_DATA_HPP
#define ALIA_DATA_HPP

#include <alia/forward.hpp>
#include <alia/id.hpp>
#include <alia/typedefs.hpp>
#include <alia/flags.hpp>
#include <boost/noncopyable.hpp>
#include <cassert>

namespace alia {

struct data_node : boost::noncopyable
{
    data_node() : next(0) {}
    virtual ~data_node() { delete next; }
    data_node* next;
};

template<class T>
struct typed_data_node : data_node
{
    T value;
};

struct dynamic_block_node;

struct data_block
{
    data_block();
    ~data_block();
    uint64 last_refresh;
    data_node* nodes;
    dynamic_block_node* unused_blocks;
    dynamic_block_node* used_blocks;
    dynamic_block_node* first_used_block;
};
data_block* get_data_block(context& ctx);

class scoped_data_block : boost::noncopyable
{
 public:
    scoped_data_block() : active_(false) {}
    scoped_data_block(context& ctx, data_block& block) { begin(ctx, block); }
    ~scoped_data_block() { end(); }
    void begin(context& ctx, data_block& block);
    void end();
 private:
    context* ctx_;
    data_block* old_block_;
    dynamic_block_node* old_predicted_;
    data_node** old_next_ptr_;
    bool active_;
};

struct naming_map;

class naming_context : boost::noncopyable
{
 public:
    naming_context() : active_(false) {}
    naming_context(context& ctx) { begin(ctx); }
    ~naming_context() { end(); }
    void begin(context& ctx);
    void end();
 private:
    context* ctx_;
    naming_map* old_map_;
    bool active_;
};

class named_block : boost::noncopyable
{
 public:
    template<class Id>
    named_block(context& ctx, Id const& id, flag_set flags = NO_FLAGS)
    { begin_impl(ctx, typed_id<Id>(id), flags); }
    template<class Id>
    void begin(context& ctx, Id const& id, flag_set flags = NO_FLAGS)
    { begin_impl(ctx, typed_id<Id>(id), flags); }
    void end();
 private:
    void begin_impl(context& ctx, id_interface const& id,
        flag_set flags = NO_FLAGS);
    scoped_data_block scoped_data_block_;
};

void delete_named_data_impl(context& ctx, id_interface const& id);
template<class Id>
void delete_named_data(context& ctx, Id const& id)
{ delete_named_data_impl(ctx, typed_id<Id>(id)); }

struct switch_block_data;

class switch_block : boost::noncopyable
{
 public:
    switch_block(context& ctx);
    ~switch_block();
    void activate_case(unsigned i);
 private:
    context* ctx_;
    switch_block_data* data_;
    data_block* old_block_;
    dynamic_block_node* old_predicted_;
    data_node** old_next_ptr_;
};

}

#endif
