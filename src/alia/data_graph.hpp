#ifndef ALIA_DATA_GRAPH_HPP
#define ALIA_DATA_GRAPH_HPP

#include <alia/id.hpp>
#include <alia/common.hpp>
#include <cassert>
#include <alia/accessors.hpp>

// This file defines the data retrieval library used for associating mutable
// state and cached data with alia UIs. It is designed so that each widget
// instance is associated with a unique instance of data, even if there is
// no specific external identifier for that widget instance.
//
// More generally, if you replace "widget instance" with "subexpression
// evaluation" in the previous sentence, it can be used to associate data with
// particular points in the evaluation of any function. This can be useful in
// situations where you need to evaluate a particular function many times with
// slightly different inputs and you want to reuse the work that was done in
// earlier evaluations without a lot of manual bookkeeping.
//
// To understand what's going on here, imagine the evaluation of a function on
// a simple in-order, single-threaded processor. We can represent all possible
// execution flows using a single DAG where each node represents the execution
// of a particular instruction by the processor and edges represent the
// transition to the next instruction. Nodes with multiple edges leaving them
// represent the execution of branch instructions, while nodes with multiple
// edges coming in are points where multiple branches merge back into a single
// flow.
//
// Since the graph is a DAG, loops are represented by unrolling them.
// Similarly, function calls are represented by inlining the callee's graph
// into the caller's graph (with appropriate argument substitutions).
// Note that both of these features make the graph potentially infinite.
// Furthermore, if calls to function pointers are involved, parts of the graph
// may be entirely unknown.
//
// Thus, for an arbitrary function, we cannot construct its graph a priori.
// However, we CAN observe a particular evaluation of the function and
// construct its path through the graph. We can also observe multiple
// evaluations and construct the portion of the DAG that these executions
// cover. In other words, if we're only interested in portions of the graph
// that are reached by actual evaluations of the function, we can lazily
// construct them by simply observing those evaluations.
//
// And that is essentially what this library does. In order to use it, you
// must annotate the control flow in your function, and it uses these
// annotations to trace each evaluation's flow through the graph, constructing
// unexplored regions as they're encountered. The graph is used to store data
// that is made available to your function as it executes.
//
// One problem with all this is that sometimes a subexpression evaluation
// (widget instance) is associated with a particular piece of input data
// and the evaluation of that input data is not fixed within the graph
// (e.g., it's in a list of items where you can remove or shuffle items).
// In cases like this, we allow the application to attach an explicit ID to
// the subgraph representing the evaluation of that expression, and we ensure
// that that subgraph is always used where that ID is encountered.

namespace alia {

// It's worth noting here that the storage of the graph is slightly different
// from what's described above. In reality, the only nodes the library knows
// about are the annotated branch nodes and ones where you request data.
// Other nodes are irrelevant, and the library never knows about them.
// Furthermore, not all edges need to be stored explicitly.

// A data node is a node in the graph that represents the retrieval of data,
// and thus it stores the data associated with that retrieval.
// Data nodes are stored as linked lists, held by data_blocks.
//
// Note that a data node is capable of storing any type of data.
// data_node is a base class for all data nodes.
// typed_data_node<T> represents data nodes that store values of type T.
//
struct data_node : noncopyable
{
    data_node() : next(0) {}
    virtual ~data_node() {}
    data_node* next;
};
template<class T>
struct typed_data_node : data_node
{
    T value;
};

struct named_block_ref_node;

// A data block represents a block of execution. During a single evaluation,
// either all nodes in the block are executed or all nodes are bypassed, and,
// if executed, they are always executed in the same order.
// (Other nodes may be executed in between, depending on the evaluation.)
struct data_block : noncopyable
{
    // the list of nodes in this basic block
    data_node* nodes;

    // set if the block's cache is clear
    bool cache_clear;

    // list of named blocks (blocks with IDs) referenced from this data block
    // The references maintain (shared) ownership of the named blocks.
    // The order of the references indicates the order in which the block
    // references appeared in the last pass. When inputs are constant, this
    // order is also constant, and thus we can find the blocks with a very
    // small, constant cost.
    named_block_ref_node* named_blocks;

    data_block();
    ~data_block();
};

// Clear all data from a data block.
void clear_data_block(data_block& block);

struct naming_map_node;

// data_graph stores the data graph associated with a function.
struct data_graph : noncopyable
{
    data_block root_block;

    naming_map_node* map_list;

    // This list stores unused references to named blocks. When named block
    // references disappear from a traversal, it's possible that they've done
    // so only because the traversal was interrupted by an exception.
    // Therefore, they're kept here temporarily to keep the named blocks alive
    // until a complete traversal can establish new references to the named
    // blocks. They're cleaned up when someone calls gc_named_data(graph)
    // following a complete traversal.
    named_block_ref_node* unused_named_block_refs;

    // for versioning state stored in the graph
    //local_identity identity;

    data_graph()
      : map_list(0)
    {}
};

struct naming_map;

// data_traversal stores the state associated with a single traversal of a
// data_graph.
struct data_traversal
{
    data_graph* graph;
    naming_map* active_map;
    data_block* active_block;
    named_block_ref_node* predicted_named_block;
    named_block_ref_node* used_named_blocks;
    named_block_ref_node** named_block_next_ptr;
    data_node** next_data_ptr;
    bool gc_enabled;
    bool cache_clearing_enabled;
    // If this is set, the traversal was aborted, so we shouldn't expect it
    // to complete.
    bool traversal_aborted;
};

// The utilities here operate on data_traversals. However, the data_graph
// library is intended to be used to enable the development of other libraries
// with immediate mode APIs, and while the utilities below are intended to be
// used directly by the application developer, they are intended to be used
// within a context defined by the larger IM library. Thus, the utilities
// are designed to accept a generic context parameter. The only requirement is
// that it defines the function get_data_traversal(ctx), which returns a
// reference to a data_traversal.

// If using this library directly, the data_traversal itself can server as the
// context.
static inline data_traversal& get_data_traversal(data_traversal& ctx)
{ return ctx; }

// A scoped_data_block activates the associated data_block at the beginning
// of its scope and deactivates it at the end. It's useful anytime there is a
// branch in the code and you need to activate the block associated with the
// taken branch while that branch is active.
// Note that the macros defined below make heavy use of this and reduce the
// need for applications to use it directly.
struct scoped_data_block : noncopyable
{
    scoped_data_block() : traversal_(0) {}

    template<class Context>
    scoped_data_block(Context& ctx, data_block& block)
    { begin(ctx, block); }

    ~scoped_data_block() { end(); }

    template<class Context>
    void begin(Context& ctx, data_block& block)
    { begin(get_data_traversal(ctx), block); }

    void begin(data_traversal& traversal, data_block& block);

    void end();

 private:
    data_traversal* traversal_;
    // old state
    data_block* old_active_block_;
    named_block_ref_node* old_predicted_named_block_;
    named_block_ref_node* old_used_named_blocks_;
    named_block_ref_node** old_named_block_next_ptr_;
    data_node** old_next_data_ptr_;
};

// A named_block is like a scoped_data_block, but instead of supplying a
// data_block directly, you provide an ID, and it finds the block associated
// with that ID and activates it.
//
// This is the mechanism for dealing with dynamically ordered data.
// named_blocks are free to move around within the graph as long as they
// maintain the same IDs.
//
// A naming_context provides a context for IDs. IDs used within one naming
// context can be reused within another without conflict.
//
// named_blocks are automatically garbage collected when the library detects
// that they've disappeared from the graph. The logic for this is fairly
// sophisticated, and it generally won't mistakingly collect named_blocks in
// inactive regions of the graph. However, it still may not always do what you
// want.
// In those cases, you can specify the manual_delete flag. This will prevent
// the library from collecting the block. It can be deleted manually by calling
// delete_named_data(ctx, id). If that never happens, it will be deleted when
// its context is destroyed.

// The flag is specified via its own structure to make it very obvious at the
// call site.
struct manual_delete
{
    explicit manual_delete(bool value)
      : value(value) {}
    bool value;
};

struct named_block : noncopyable
{
    named_block() {}

    template<class Context>
    named_block(Context& ctx, id_interface const& id,
        manual_delete manual = manual_delete(false))
    { begin(ctx, id, manual); }

    template<class Context>
    void begin(Context& ctx, id_interface const& id,
        manual_delete manual = manual_delete(false))
    { begin(get_data_traversal(ctx), get_naming_map(ctx), id, manual); }

    void begin(data_traversal& traversal, naming_map& map,
        id_interface const& id, manual_delete manual);

    void end();

 private:
    scoped_data_block scoped_data_block_;
};

struct naming_context : noncopyable
{
    naming_context() {}

    template<class Context>
    naming_context(Context& ctx)
    { begin(ctx); }

    ~naming_context() { end(); }

    template<class Context>
    void begin(Context& ctx)
    { begin(get_data_traversal(ctx)); }

    void begin(data_traversal& traversal);

    void end() {}

    data_traversal& traversal() { return *traversal_; }
    naming_map& map() { return *map_; }

 private:
    data_traversal* traversal_;
    naming_map* map_;
};
static inline data_traversal& get_data_traversal(naming_context& ctx)
{ return ctx.traversal(); }
static inline naming_map& get_naming_map(naming_context& ctx)
{ return ctx.map(); }

// retrieve_naming_map gets a data_map from a data_traveral and registers it
// with the underlying graph. It can be used to retrieve additional naming maps
// from a data graph, in case you want to manage them yourself.
naming_map* retrieve_naming_map(data_traversal& traversal);

// delete_named_block(ctx, id) deletes the data associated with a particular
// named block, as identified by the given ID.

void delete_named_block(data_graph& graph, id_interface const& id);

template<class Context>
void delete_named_block(Context& ctx, id_interface const& id)
{ delete_named_block(*get_data_traversal(ctx).graph, id); }

// This is a macro that, given a context, an uninitialized named_block, and an
// ID, combines the ID with another ID which is unique to that location in the
// code (but not the graph), and then initializes the named_block with the
// combined ID.
// This is not as generally useful as naming_context, but it can be used to
// identify the combinaion of a function and its argument.
#define ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, named_block, id) \
    { \
        static int alia__dummy_static; \
        named_block.begin(ctx, combine_ids(make_id(&alia__dummy_static), id));\
    }

// scoped_gc_disabler disables the garbage collector within a scope of a
// traversal. It's used when you don't intend to visit the entire active part
// of the graph and thus don't want the garbage collector to collect the
// unvisited parts.
// When using this, if you visit named blocks, you must visit all blocks in a
// data_block in the same order that they were last visited with the garbage
// collector enabled. However, you don't have to finish the entire sequence.
// If you violate this rule, you'll get a named_block_out_of_order exception.
struct named_block_out_of_order : exception
{
    named_block_out_of_order()
      : exception("named block order must remain constant with GC disabled")
    {}
};
struct scoped_gc_disabler
{
    scoped_gc_disabler() : traversal_(0) {}
    template<class Context>
    scoped_gc_disabler(Context& ctx)
    { begin(ctx); }
    ~scoped_gc_disabler() { end(); }
    template<class Context>
    void begin(Context& ctx)
    { begin(get_data_traversal(ctx)); }
    void begin(data_traversal& traversal);
    void end();
 private:
    data_traversal* traversal_;
    bool old_gc_state_;
};

// Similar to scoped_gc_disabler, this will prevent the library from clearing
// the cache of blocks that are inactive.
struct scoped_cache_clearing_disabler
{
    scoped_cache_clearing_disabler() : traversal_(0) {}
    template<class Context>
    scoped_cache_clearing_disabler(Context& ctx)
    { begin(ctx); }
    ~scoped_cache_clearing_disabler() { end(); }
    template<class Context>
    void begin(Context& ctx)
    { begin(get_data_traversal(ctx)); }
    void begin(data_traversal& traversal);
    void end();
 private:
    data_traversal* traversal_;
    bool old_cache_clearing_state_;
};

// get_data(traversal, &ptr) represents a data node in the data graph.
// The call retrieves data from the graph at the current point in the
// traversal, assigns its address to *ptr, and advances the traversal to the
// next node.
// The return value is true if the data at the node was just constructed and
// false if it already existed.
//
// Note that get_data should normally not be used directly by the application.

template<class Context, class T>
bool get_data(Context& ctx, T** ptr)
{
    data_traversal& traversal = get_data_traversal(ctx);
    data_node* node = *traversal.next_data_ptr;
    if (node)
    {
        if (!dynamic_cast<typed_data_node<T>*>(node))
        assert(dynamic_cast<typed_data_node<T>*>(node));
        typed_data_node<T>* typed_node =
            static_cast<typed_data_node<T>*>(node);
        traversal.next_data_ptr = &node->next;
        *ptr = &typed_node->value;
        return false;
    }
    else
    {
        typed_data_node<T>* new_node = new typed_data_node<T>;
        *traversal.next_data_ptr = new_node;
        traversal.next_data_ptr = &new_node->next;
        *ptr = &new_node->value;
        return true;
    }
}

// get_cached_data(ctx, &ptr) is identical to get_data(ctx, &ptr), but the
// data stored in the node is understood to be a cached value of data that's
// generated by the application. The system assumes that the data can be
// regenerated if it's lost.

struct cached_data
{
    virtual ~cached_data() {}
};

template<class T>
struct typed_cached_data : cached_data
{
    T value;
};

struct cached_data_holder
{
    cached_data_holder() : data(0) {}
    ~cached_data_holder() { delete data; }
    cached_data* data;
};

template<class Context, class T>
bool get_cached_data(Context& ctx, T** ptr)
{
    cached_data_holder* holder;
    get_data(ctx, &holder);
    if (holder->data)
    {
        assert(dynamic_cast<typed_cached_data<T>*>(holder->data));
        typed_cached_data<T>* data =
            static_cast<typed_cached_data<T>*>(holder->data);
        *ptr = &data->value;
        return false;
    }
    typed_cached_data<T>* data = new typed_cached_data<T>;
    holder->data = data;
    *ptr = &data->value;
    return true;
}

// get_state is the standard interface for retrieving state from a data graph.
// Instead of a simple pointer, it returns an accessor to the state, which will
// allow future versions of this code to track changes in the data graph.
// It comes in multiple forms...
//
// get_state(ctx, default_value) returns the accessor to the state. If the
// state hasn't been initialized yet, it's initialized with default_value.
// default_value is optional, and if omitted, the state will be initialized to
// a default constructed value.
//
// get_state(ctx, &accessor) writes the accessor for the state to *accessor.
// The return value is true iff the underlying state requires initialization.

template<class Context, class T>
bool get_state(Context& ctx, state_accessor<T>* accessor)
{
    state<T>* ptr;
    bool is_new = get_data(get_data_traversal(ctx), &ptr);
    *accessor = make_accessor(*ptr);
    return is_new;
}

template<class T, class Context>
std::enable_if_t<
    !std::is_base_of<untyped_accessor_base,T>::value,
    state_accessor<T> >
get_state(Context& ctx, T const& default_value = T())
{
    state<T>* ptr;
    if (get_data(ctx, &ptr))
        ptr->set(default_value);
    return make_accessor(*ptr);
}

// Another form of get_state where the initial_value is passed by accessor...
// This is now the preferred form.

// get_state(ctx, initial_value) returns an accessor to some persistent local
// state whose initial value is determined by the accessor :initial_value. The
// returned accessor will not be gettable until :initial_value is gettable.
template<class Context, class State>
auto
get_state(Context& ctx, accessor<State> const& initial_value)
{
    auto state = get_state(ctx, optional<State>());
    if (is_gettable(state) && !get(state) && is_gettable(initial_value))
    {
        set(state, some(get(initial_value)));
    }
    return unwrap_optional(state);
}

// get_keyed_data(ctx, key, &accessor) is a utility for retrieving cached data
// from a data graph.
// It stores not only the data but also a key that identifies the data.
// The key is presented at each retrieval, and if it changes, the associated
// data is invalidated and must be recomputed.

// The return value is true iff the data needs to be recomputed.

template<class Data>
struct keyed_data
{
    owned_id key;
    bool is_valid;
    Data value;
    keyed_data() : is_valid(false) {}
};

template<class Data>
bool is_valid(keyed_data<Data> const& data)
{ return data.is_valid; }

template<class Data>
void invalidate(keyed_data<Data>& data)
{
    data.is_valid  = false;
    data.key.clear();
}

template<class Data>
void mark_valid(keyed_data<Data>& data)
{ data.is_valid = true; }

template<class Data>
bool refresh_keyed_data(keyed_data<Data>& data, id_interface const& key)
{
    if (!data.key.matches(key))
    {
        data.is_valid = false;
        data.key.store(key);
        return true;
    }
    return false;
}

template<class Data>
void set(keyed_data<Data>& data, Data const& value)
{
    data.value = value;
    mark_valid(data);
}

template<class Data>
Data const& get(keyed_data<Data> const& data)
{
    assert(is_valid(data));
    return data.value;
}

template<class Data>
struct keyed_data_accessor : accessor<Data>
{
    keyed_data_accessor() {}
    keyed_data_accessor(keyed_data<Data>* data) : data_(data) {}
    bool is_gettable() const { return data_->is_valid; }
    Data const& get() const { return data_->value; }
    alia__shared_ptr<Data> get_ptr() const
    { return alia__shared_ptr<Data>(new Data(data_->value)); }
    id_interface const& id() const
    { return data_->key.is_initialized() ? data_->key.get() : no_id; }
    bool is_settable() const { return true; }
    void set(Data const& value) const
    { alia::set(*data_, value); }
 private:
    keyed_data<Data>* data_;
};

template<class Data>
keyed_data_accessor<Data>
make_accessor(keyed_data<Data>* data)
{ return keyed_data_accessor<Data>(data); }

template<class Context, class Data>
bool get_keyed_data(Context& ctx, id_interface const& key,
    keyed_data_accessor<Data>* accessor)
{
    keyed_data<Data>* ptr;
    get_cached_data(ctx, &ptr);
    refresh_keyed_data(*ptr, key);
    *accessor = make_accessor(ptr);
    return !is_valid(*ptr);
};

// This is another form of get_keyed_data where there's no accessor to guard
// access to the retrieved data. Thus, it's up to the caller to track whether
// or not the data is properly initialized.

template<class Data>
struct raw_keyed_data
{
    owned_id key;
    Data data;
};

template<class Context, class Data>
bool get_keyed_data(Context& ctx, id_interface const& key, Data** data)
{
    raw_keyed_data<Data>* ptr;
    bool is_new = false;
    if (get_cached_data(ctx, &ptr))
    {
        ptr->key.store(key);
        is_new = true;
    }
    else if (!ptr->key.matches(key))
    {
        ptr->key.store(key);
        ptr->data = Data();
        is_new = true;
    }
    *data = &ptr->data;
    return is_new;
};

// scoped_data_traversal can be used to manage a traversal of a graph.
// begin(graph, traversal) will initialize traversal to act as a traversal of
// graph.
struct scoped_data_traversal
{
    scoped_data_traversal() {}

    scoped_data_traversal(data_graph& graph, data_traversal& traversal)
    { begin(graph, traversal); }

    ~scoped_data_traversal()
    { end(); }

    void begin(data_graph& graph, data_traversal& traversal);

    void end();

 private:
    scoped_data_block root_block_;
    naming_context root_map_;
};

// Clear all cached data stored in the subgraph referenced from the given
// data_block.
void clear_cached_data(data_block& block);

// The following are utilities that are used to implement the control flow
// macros. They shouldn't be used directly by applications.

struct if_block : noncopyable
{
    if_block(data_traversal& traversal, bool condition);
 private:
    scoped_data_block scoped_data_block_;
};

struct pass_dependent_if_block : noncopyable
{
    pass_dependent_if_block(data_traversal& traversal, bool condition);
 private:
    scoped_data_block scoped_data_block_;
};

struct switch_block : noncopyable
{
    template<class Context>
    switch_block(Context& ctx)
    {
        nc_.begin(ctx);
    }
    template<class Id>
    void activate_case(Id id)
    {
        active_case_.end();
        active_case_.begin(nc_, make_id(id), manual_delete(true));
    }
 private:
    naming_context nc_;
    named_block active_case_;
};

struct loop_block : noncopyable
{
    loop_block(data_traversal& traversal);
    ~loop_block();
    data_block& block() const { return *block_; }
    data_traversal& traversal() const { return *traversal_; }
    void next();
 private:
    data_traversal* traversal_;
    data_block* block_;
};

// The following are macros used to annotate control flow.
// They are used exactly like their C equivalents, but all require an alia_end
// after the end of their scope.
// Also note that all come in two forms. One form ends in an underscore and
// takes the context as its first argument. The other form has no trailing
// underscore and assumes that the context is a variable named 'ctx'.

// is_true(x) evaluates x in a boolean context.
template<class T>
std::enable_if_t<!std::is_base_of<untyped_accessor_base,T>::value,bool>
is_true(T x)
{ return x ? true : false; }

// is_true(x), where x is an accessor to a bool, returns true iff x is
// gettable and its value is true.
template<class Accessor>
std::enable_if_t<std::is_base_of<untyped_accessor_base,Accessor>::value,bool>
is_true(Accessor const& x)
{ return is_gettable(x) && is_true(get(x)); }

// is_false(x) evaluates x in a boolean context and inverts it.
template<class T>
std::enable_if_t<!std::is_base_of<untyped_accessor_base,T>::value,bool>
is_false(T x)
{ return x ? false : true; }

// is_false(x), where x is an accessor to a bool, returns false iff x is
// gettable and its value is false.
template<class Accessor>
std::enable_if_t<std::is_base_of<untyped_accessor_base,Accessor>::value,bool>
is_false(Accessor const& x)
{ return is_gettable(x) && is_false(get(x)); }

// if, else_if, else

#define alia_if_(ctx, condition) \
    { \
        bool alia__else_condition; \
        { \
            auto const& alia__condition_value = (condition); \
            bool alia__if_condition = alia::is_true(alia__condition_value); \
            alia__else_condition = alia::is_false(alia__condition_value); \
            ::alia::if_block alia__if_block(get_data_traversal(ctx), \
                alia__if_condition); \
            if (alia__if_condition) \
            {

#define alia_if(condition) alia_if_(ctx, condition)

#define alia_else_if_(ctx, condition) \
            } \
        } \
        { \
            auto const& alia__condition_value = (condition); \
            bool alia__else_if_condition = \
                alia__else_condition && \
                alia::is_true(alia__condition_value); \
            alia__else_condition = \
                alia__else_condition && \
                alia::is_false(alia__condition_value); \
            ::alia::if_block alia__if_block(get_data_traversal(ctx), \
                alia__else_if_condition); \
            if (alia__else_if_condition) \
            { \

#define alia_else_if(condition) alia_else_if_(ctx, condition)

#define alia_else_(ctx) \
            } \
        } \
        { \
            ::alia::if_block alia__if_block(get_data_traversal(ctx), \
                alia__else_condition); \
            if (alia__else_condition) \
            {

#define alia_else alia_else_(ctx)

// pass_dependent_if - This is used for tests that involve conditions that
// change from one pass to another. It does not clear out cached data wihin
// the block if it's skipped.

#define alia_pass_dependent_if_(ctx, condition) \
    { \
        { \
            bool alia__condition = alia::is_true(condition); \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__condition); \
            if (alia__condition) \
            {

#define alia_pass_dependent_if(condition) \
    alia_pass_dependent_if_(ctx, condition)

// switch

#define alia_switch_(ctx, x) \
    {{ \
        ::alia::switch_block alia__switch_block(ctx); \
        switch(x) {

#define alia_switch(x) alia_switch_(ctx, x)

#define ALIA_CONCATENATE_HELPER(a, b) a ## b
#define ALIA_CONCATENATE(a, b) ALIA_CONCATENATE_HELPER(a, b)

#define alia_case(c) \
            case c: \
                alia__switch_block.activate_case(c); \
                goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__); \
                ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

#define alia_default \
            default: \
                alia__switch_block.activate_case("default"); \
                goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__); \
                ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

// for

#define alia_for_(ctx, x) \
    {{ \
        ::alia::loop_block alia__looper(get_data_traversal(ctx)); \
        for (x) \
        { \
            ::alia::scoped_data_block alia__scope; \
            alia__scope.begin(alia__looper.traversal(), alia__looper.block());\
            alia__looper.next();

#define alia_for(x) alia_for_(ctx, x)

// while

#define alia_while_(ctx, x) \
    {{ \
        ::alia::loop_block alia__looper(get_data_traversal(ctx)); \
        while (x) \
        { \
            ::alia::scoped_data_block alia__scope; \
            alia__scope.begin(alia__looper.traversal(), alia__looper.block());\
            alia__looper.next();

#define alia_while(x) alia_while_(ctx, x)

// end

#define alia_end \
    }}}

}

#endif
