#ifndef ALIA_FLOW_DATA_GRAPH_HPP
#define ALIA_FLOW_DATA_GRAPH_HPP

#include <alia/common.hpp>
#include <alia/id.hpp>
#include <alia/signals/core.hpp>
#include <cassert>

// This file defines the data retrieval library used for associating mutable
// state and cached data with alia content graphs. It is designed so that each
// node emitted by an application is associated with a unique instance of data,
// even if there is no specific external identifier for that node.
//
// More generally, if you replace "node" with "subexpression evaluation" in the
// previous sentence, it can be used to associate data with particular points in
// the evaluation of any function. This can be useful in situations where you
// need to evaluate a particular function many times with slightly different
// inputs and you want to reuse the work that was done in earlier evaluations
// without a lot of manual bookkeeping.
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
// (content node) is associated with a particular piece of input data and the
// evaluation of that input data is not fixed within the graph (e.g., it's in a
// list of items where you can remove or shuffle items). In cases like this, we
// allow the application to attach an explicit name to the subgraph
// representing the evaluation of that expression, and we ensure that that
// subgraph is always used where that name is encountered.

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
    data_node() : next(0)
    {
    }
    virtual ~data_node()
    {
    }
    data_node* next;
};
template<class T>
struct typed_data_node : data_node
{
    T value;
};

struct named_block_ref_node;

// A data_block represents a block of execution. During a single evaluation,
// either all nodes in the block are executed or all nodes are bypassed, and, if
// executed, they are always executed in the same order. (It's conceptually
// similar to a 'basic block' except that other nodes may be executed in between
// nodes in a data_block.)
struct data_block : noncopyable
{
    // the list of nodes in this block
    data_node* nodes = nullptr;

    // a flag to track if the block's cache is clear
    bool cache_clear = true;

    // list of named blocks referenced from this data block - The references
    // maintain shared ownership of the named blocks. The order of the
    // references indicates the order in which the block references appeared in
    // the last pass. When the content graph is stable and this order is
    // constant, we can find the blocks with a very small, constant cost.
    named_block_ref_node* named_blocks = nullptr;

    ~data_block();
};

// Clear all data from a data block.
// Note that this recursively processes child blocks.
void
clear_data_block(data_block& block);

struct naming_map_node;

// data_graph stores the data graph associated with a function.
struct data_graph : noncopyable
{
    data_block root_block;

    naming_map_node* map_list = nullptr;

    // This list stores unused references to named blocks. When named block
    // references disappear from a traversal, it's possible that they've done
    // so only because the traversal was interrupted by an exception.
    // Therefore, they're kept here temporarily to keep the named blocks alive
    // until a complete traversal can establish new references to the named
    // blocks. They're cleaned up when someone calls gc_named_data(graph)
    // following a complete traversal.
    named_block_ref_node* unused_named_block_refs = nullptr;
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
};

// The utilities here operate on data_traversals. However, the data_graph
// library is intended to be used in scenarios where the data_traversal object
// is part of a larger context. Thus, any utilities here that are intended to be
// used directly by the application developer are designed to accept a generic
// context parameter. The only requirement on that paramater is that it defines
// the function get_data_traversal(ctx), which returns a reference to a
// data_traversal.

// If using this library directly, the data_traversal itself can serve as the
// context.
inline data_traversal&
get_data_traversal(data_traversal& ctx)
{
    return ctx;
}

// A scoped_data_block activates the associated data_block at the beginning
// of its scope and deactivates it at the end. It's useful anytime there is a
// branch in the code and you need to activate the block associated with the
// taken branch while that branch is active.
// Note that the macros defined below make heavy use of this and reduce the
// need for applications to use it directly.
struct scoped_data_block : noncopyable
{
    scoped_data_block() : traversal_(0)
    {
    }

    template<class Context>
    scoped_data_block(Context& ctx, data_block& block)
    {
        begin(ctx, block);
    }

    ~scoped_data_block()
    {
        end();
    }

    template<class Context>
    void
    begin(Context& ctx, data_block& block)
    {
        begin(get_data_traversal(ctx), block);
    }

    void
    begin(data_traversal& traversal, data_block& block);

    void
    end();

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
// want. In those cases, you can specify the manual_delete flag. This will
// prevent the library from collecting the block. It can be deleted manually by
// calling delete_named_data(ctx, id). If that never happens, it will be deleted
// when the data_graph that it belongs to is destroyed. (But this is likely to
// still be a memory leak, since the data_graph might live on as long as the
// application is running.)

// The manual deletion flag is specified via its own structure to make it very
// obvious at the call site.
struct manual_delete
{
    explicit manual_delete(bool value) : value(value)
    {
    }
    bool value;
};

struct named_block : noncopyable
{
    named_block()
    {
    }

    template<class Context>
    named_block(
        Context& ctx,
        id_interface const& id,
        manual_delete manual = manual_delete(false))
    {
        begin(ctx, id, manual);
    }

    template<class Context>
    void
    begin(
        Context& ctx,
        id_interface const& id,
        manual_delete manual = manual_delete(false))
    {
        begin(get_data_traversal(ctx), get_naming_map(ctx), id, manual);
    }

    void
    begin(
        data_traversal& traversal,
        naming_map& map,
        id_interface const& id,
        manual_delete manual);

    void
    end();

 private:
    scoped_data_block scoped_data_block_;
};

struct naming_context : noncopyable
{
    naming_context()
    {
    }

    template<class Context>
    naming_context(Context& ctx)
    {
        begin(ctx);
    }

    ~naming_context()
    {
        end();
    }

    template<class Context>
    void
    begin(Context& ctx)
    {
        begin(get_data_traversal(ctx));
    }

    void
    begin(data_traversal& traversal);

    void
    end()
    {
    }

    data_traversal&
    traversal()
    {
        return *traversal_;
    }
    naming_map&
    map()
    {
        return *map_;
    }

 private:
    data_traversal* traversal_;
    naming_map* map_;
};
inline data_traversal&
get_data_traversal(naming_context& ctx)
{
    return ctx.traversal();
}
inline naming_map&
get_naming_map(naming_context& ctx)
{
    return ctx.map();
}

// retrieve_naming_map gets a data_map from a data_traveral and registers it
// with the underlying graph. It can be used to retrieve additional naming maps
// from a data graph, in case you want to manage them yourself.
naming_map*
retrieve_naming_map(data_traversal& traversal);

// delete_named_block(ctx, id) deletes the data associated with a particular
// named block, as identified by the given ID.

void
delete_named_block(data_graph& graph, id_interface const& id);

template<class Context>
void
delete_named_block(Context& ctx, id_interface const& id)
{
    delete_named_block(*get_data_traversal(ctx).graph, id);
}

// This is a macro that, given a context, an uninitialized named_block, and an
// ID, combines the ID with another ID which is unique to that location in the
// code (but not the graph), and then initializes the named_block with the
// combined ID.
// This is not as generally useful as naming_context, but it can be used to
// identify the combinaion of a function and its argument.
#define ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, named_block, id)         \
    {                                                                          \
        static int _alia_dummy_static;                                         \
        named_block.begin(ctx, combine_ids(make_id(&_alia_dummy_static), id)); \
    }

// disable_gc(traversal) disables the garbage collector for a data traversal.
// It's used when you don't intend to visit the entire active part of the graph
// and thus don't want the garbage collector to collect the unvisited parts.
// It should be invoked before actually beginning a traversal.
// When using this, if you visit named blocks, you must visit all blocks in a
// data_block in the same order that they were last visited with the garbage
// collector enabled. However, you don't have to finish the entire sequence.
// If you violate this rule, you'll get a named_block_out_of_order exception.
struct named_block_out_of_order : exception
{
    named_block_out_of_order()
        : exception("named block order must remain constant with GC disabled")
    {
    }
};
void
disable_gc(data_traversal& traversal);

// scoped_cache_clearing_disabler will prevent the library from clearing the
// cache of inactive blocks within its scope.
struct scoped_cache_clearing_disabler
{
    scoped_cache_clearing_disabler() : traversal_(0)
    {
    }
    template<class Context>
    scoped_cache_clearing_disabler(Context& ctx)
    {
        begin(ctx);
    }
    ~scoped_cache_clearing_disabler()
    {
        end();
    }
    template<class Context>
    void
    begin(Context& ctx)
    {
        begin(get_data_traversal(ctx));
    }
    void
    begin(data_traversal& traversal);
    void
    end();

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
bool
get_data(Context& ctx, T** ptr)
{
    data_traversal& traversal = get_data_traversal(ctx);
    data_node* node = *traversal.next_data_ptr;
    if (node)
    {
        if (!dynamic_cast<typed_data_node<T>*>(node))
            assert(dynamic_cast<typed_data_node<T>*>(node));
        typed_data_node<T>* typed_node = static_cast<typed_data_node<T>*>(node);
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
    virtual ~cached_data()
    {
    }
};

template<class T>
struct typed_cached_data : cached_data
{
    T value;
};

struct cached_data_holder
{
    cached_data_holder() : data(0)
    {
    }
    ~cached_data_holder()
    {
        delete data;
    }
    cached_data* data;
};

template<class Context, class T>
bool
get_cached_data(Context& ctx, T** ptr)
{
    cached_data_holder* holder;
    get_data(ctx, &holder);
    if (holder->data)
    {
        assert(dynamic_cast<typed_cached_data<T>*>(holder->data));
        typed_cached_data<T>* data
            = static_cast<typed_cached_data<T>*>(holder->data);
        *ptr = &data->value;
        return false;
    }
    typed_cached_data<T>* data = new typed_cached_data<T>;
    holder->data = data;
    *ptr = &data->value;
    return true;
}

// Clear all cached data stored within a data block.
// Note that this recursively processes child blocks.
void
clear_cached_data(data_block& block);

// get_keyed_data(ctx, key, &signal) is a utility for retrieving cached data
// from a data graph.
// It stores not only the data but also a key that identifies the data.
// The key is presented at each retrieval, and if it changes, the associated
// data is invalidated and must be recomputed.

// The return value is true iff the data needs to be recomputed.

template<class Data>
struct keyed_data
{
    captured_id key;
    bool is_valid;
    Data value;
    keyed_data() : is_valid(false)
    {
    }
};

template<class Data>
bool
is_valid(keyed_data<Data> const& data)
{
    return data.is_valid;
}

template<class Data>
void
invalidate(keyed_data<Data>& data)
{
    data.is_valid = false;
    data.key.clear();
}

template<class Data>
void
mark_valid(keyed_data<Data>& data)
{
    data.is_valid = true;
}

template<class Data>
bool
refresh_keyed_data(keyed_data<Data>& data, id_interface const& key)
{
    if (!data.key.matches(key))
    {
        data.is_valid = false;
        data.key.capture(key);
        return true;
    }
    return false;
}

template<class Data>
void
set(keyed_data<Data>& data, Data const& value)
{
    data.value = value;
    mark_valid(data);
}

template<class Data>
Data const&
get(keyed_data<Data> const& data)
{
    assert(is_valid(data));
    return data.value;
}

template<class Data>
struct keyed_data_signal
    : signal<keyed_data_signal<Data>, Data, bidirectional_signal>
{
    keyed_data_signal()
    {
    }
    keyed_data_signal(keyed_data<Data>* data) : data_(data)
    {
    }
    bool
    is_readable() const
    {
        return data_->is_valid;
    }
    Data const&
    read() const
    {
        return data_->value;
    }
    id_interface const&
    value_id() const
    {
        return data_->key.is_initialized() ? data_->key.get() : no_id;
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(Data const& value) const
    {
        alia::set(*data_, value);
    }

 private:
    keyed_data<Data>* data_;
};

template<class Data>
keyed_data_signal<Data>
make_signal(keyed_data<Data>* data)
{
    return keyed_data_signal<Data>(data);
}

template<class Context, class Data>
bool
get_keyed_data(
    Context& ctx, id_interface const& key, keyed_data_signal<Data>* signal)
{
    keyed_data<Data>* ptr;
    get_cached_data(ctx, &ptr);
    refresh_keyed_data(*ptr, key);
    *signal = make_signal(ptr);
    return !is_valid(*ptr);
};

// This is another form of get_keyed_data where there's no signal to guard
// access to the retrieved data. Thus, it's up to the caller to track whether
// or not the data is properly initialized.

template<class Data>
struct raw_keyed_data
{
    captured_id key;
    Data data;
};

template<class Context, class Data>
bool
get_keyed_data(Context& ctx, id_interface const& key, Data** data)
{
    raw_keyed_data<Data>* ptr;
    bool is_new = false;
    if (get_cached_data(ctx, &ptr))
    {
        ptr->key.capture(key);
        is_new = true;
    }
    else if (!ptr->key.matches(key))
    {
        ptr->key.capture(key);
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
    scoped_data_traversal()
    {
    }

    scoped_data_traversal(data_graph& graph, data_traversal& traversal)
    {
        begin(graph, traversal);
    }

    ~scoped_data_traversal()
    {
        end();
    }

    void
    begin(data_graph& graph, data_traversal& traversal);

    void
    end();

 private:
    scoped_data_block root_block_;
    naming_context root_map_;
};

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
    void
    activate_case(Id id)
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
    data_block&
    block() const
    {
        return *block_;
    }
    data_traversal&
    traversal() const
    {
        return *traversal_;
    }
    void
    next();

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

// condition_is_true(x), where x is a signal whose value is testable in a
// boolean context, returns true iff x is readable and its value is true.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_true(Signal const& x)
{
    return signal_is_readable(x) && (read_signal(x) ? true : false);
}

// condition_is_false(x), where x is a signal whose value is testable in a
// boolean context, returns true iff x is readable and its value is false.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_false(Signal const& x)
{
    return signal_is_readable(x) && (read_signal(x) ? false : true);
}

// condition_is_readable(x), where x is a readable signal type, calls
// signal_is_readable.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_readable(Signal const& x)
{
    return signal_is_readable(x);
}

// read_condition(x), where x is a readable signal type, calls read_signal.
template<class Signal>
std::enable_if_t<
    is_readable_signal_type<Signal>::value,
    typename Signal::value_type>
read_condition(Signal const& x)
{
    return read_signal(x);
}

// ALIA_STRICT_CONDITIONALS disables the definitions that allow non-signals to
// be used in if/else/switch macros.
#ifndef ALIA_STRICT_CONDITIONALS

// condition_is_true(x) evaluates x in a boolean context.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_true(T x)
{
    return x ? true : false;
}

// condition_is_false(x) evaluates x in a boolean context and inverts it.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_false(T x)
{
    return x ? false : true;
}

// condition_is_readable(x), where x is NOT a signal type, always returns true.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_readable(T const& x)
{
    return true;
}

// read_condition(x), where x is NOT a signal type, simply returns x.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, T const&>
read_condition(T const& x)
{
    return x;
}

#endif

// if, else_if, else

#define ALIA_IF_(ctx, condition)                                               \
    {                                                                          \
        bool _alia_else_condition ALIA_UNUSED;                                 \
        {                                                                      \
            auto const& _alia_condition_value = (condition);                   \
            bool _alia_if_condition                                            \
                = ::alia::condition_is_true(_alia_condition_value);            \
            _alia_else_condition                                               \
                = ::alia::condition_is_false(_alia_condition_value);           \
            ::alia::if_block _alia_if_block(                                   \
                get_data_traversal(ctx), _alia_if_condition);                  \
            if (_alia_if_condition)                                            \
            {

#define ALIA_IF(condition) ALIA_IF_(ctx, condition)

#define ALIA_ELSE_IF_(ctx, condition)                                          \
    }                                                                          \
    }                                                                          \
    {                                                                          \
        auto const& _alia_condition_value = (condition);                       \
        bool _alia_else_if_condition                                           \
            = _alia_else_condition                                             \
              && ::alia::condition_is_true(_alia_condition_value);             \
        _alia_else_condition                                                   \
            = _alia_else_condition                                             \
              && ::alia::condition_is_false(_alia_condition_value);            \
        ::alia::if_block _alia_if_block(                                       \
            get_data_traversal(ctx), _alia_else_if_condition);                 \
        if (_alia_else_if_condition)                                           \
        {

#define ALIA_ELSE_IF(condition) ALIA_ELSE_IF_(ctx, condition)

#define ALIA_ELSE_(ctx)                                                        \
    }                                                                          \
    }                                                                          \
    {                                                                          \
        ::alia::if_block _alia_if_block(                                       \
            get_data_traversal(ctx), _alia_else_condition);                    \
        if (_alia_else_condition)                                              \
        {

#define ALIA_ELSE ALIA_ELSE_(ctx)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_if_(ctx, condition) ALIA_IF_(ctx, condition)
#define alia_if(condition) ALIA_IF(condition)
#define alia_else_if_(ctx, condition) ALIA_ELSE_IF_(ctx, condition)
#define alia_else_if(condition) ALIA_ELSE_IF(condition)
#define alia_else_(ctx) ALIA_ELSE(ctx)
#define alia_else ALIA_ELSE
#endif

// pass_dependent_if - This is used for tests that involve conditions that
// change from one pass to another. It does not clear out cached data within
// the block if it's skipped.

#define ALIA_PASS_DEPENDENT_IF_(ctx, condition)                                \
    {                                                                          \
        {                                                                      \
            bool _alia_condition = ::alia::condition_is_true(condition);       \
            ::alia::pass_dependent_if_block _alia_if_block(                    \
                get_data_traversal(ctx), _alia_condition);                     \
            if (_alia_condition)                                               \
            {

#define ALIA_PASS_DEPENDENT_IF(condition)                                      \
    ALIA_PASS_DEPENDENT_IF_(ctx, condition)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_pass_dependent_if_(ctx, condition)                                \
    ALIA_PASS_DEPENDENT_IF_(ctx, condition)
#define alia_pass_dependent_if(condition) ALIA_PASS_DEPENDENT_IF(condition)
#endif

// switch

#define ALIA_SWITCH_(ctx, x)                                                   \
    {                                                                          \
        ::alia::switch_block _alia_switch_block(ctx);                          \
        if (::alia::condition_is_readable(x))                                  \
        {                                                                      \
            switch (::alia::read_condition(x))                                 \
            {

#define ALIA_SWITCH(x) ALIA_SWITCH_(ctx, x)

#define ALIA_CONCATENATE_HELPER(a, b) a##b
#define ALIA_CONCATENATE(a, b) ALIA_CONCATENATE_HELPER(a, b)

#define ALIA_CASE(c)                                                           \
    case c:                                                                    \
        _alia_switch_block.activate_case(c);                                   \
        goto ALIA_CONCATENATE(_alia_dummy_label_, __LINE__);                   \
        ALIA_CONCATENATE(_alia_dummy_label_, __LINE__)

#define ALIA_DEFAULT                                                           \
    default:                                                                   \
        _alia_switch_block.activate_case("_alia_default_case");                \
        goto ALIA_CONCATENATE(_alia_dummy_label_, __LINE__);                   \
        ALIA_CONCATENATE(_alia_dummy_label_, __LINE__)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_switch_(ctx, x) ALIA_SWITCH_(ctx, x)
#define alia_switch(x) ALIA_SWITCH(x)
#define alia_case(c) ALIA_CASE(c)
#define alia_default ALIA_DEFAULT
#endif

// for

#define ALIA_FOR_(ctx, x)                                                      \
    {                                                                          \
        {                                                                      \
            ::alia::loop_block _alia_looper(get_data_traversal(ctx));          \
            for (x)                                                            \
            {                                                                  \
                ::alia::scoped_data_block _alia_scope;                         \
                _alia_scope.begin(                                             \
                    _alia_looper.traversal(), _alia_looper.block());           \
                _alia_looper.next();

#define ALIA_FOR(x) ALIA_FOR_(ctx, x)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_for_(ctx, x) ALIA_FOR_(ctx, x)
#define alia_for(x) ALIA_FOR(x)
#endif

// while

#define ALIA_WHILE_(ctx, x)                                                    \
    {                                                                          \
        {                                                                      \
            ::alia::loop_block _alia_looper(get_data_traversal(ctx));          \
            while (x)                                                          \
            {                                                                  \
                ::alia::scoped_data_block _alia_scope;                         \
                _alia_scope.begin(                                             \
                    _alia_looper.traversal(), _alia_looper.block());           \
                _alia_looper.next();

#define ALIA_WHILE(x) ALIA_WHILE_(ctx, x)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_while_(ctx, x) ALIA_WHILE_(ctx, x)
#define alia_while(x) ALIA_WHILE(x)
#endif

// end

#define ALIA_END                                                               \
    }                                                                          \
    }                                                                          \
    }

#ifdef ALIA_LOWERCASE_MACROS
#define alia_end ALIA_END
#endif

} // namespace alia

#endif
