#ifndef ALIA_UI_INTERFACE_HPP
#define ALIA_UI_INTERFACE_HPP

#include <alia/common.hpp>
#include <alia/accessors.hpp>
#include <alia/layout_interface.hpp>
#include <alia/event_routing.hpp>

namespace alia {

struct ui_context;

struct widget_data;
typedef widget_data* widget_id;
static widget_id const auto_id = 0;

// Often, widgets with internal storage will want to give the application the
// option of providing their own storage for that data. (This is useful if the
// application wants to persist that storage or needs to manipulate it directly
// in reponse to other user actions.)
// In those cases, the widget can accept an optional_storage<T> argument.
// The application can then call storage(accessor) to provide an accessor for
// the widget's storage, or it can pass in none to opt out of providing
// storage.
template<class T>
struct optional_storage : noncopyable
{
    optional_storage() {}
    optional_storage(none_type _) : storage(0) {}
    // 0 if no storage provided
    accessor<T> const* storage;
};
template<class Accessor>
struct accessor_storage : optional_storage<typename Accessor::value_type>
{
    accessor_storage(Accessor const& accessor)
      : accessor_(accessor)
    {
        this->storage = &accessor_;
    }
 private:
    Accessor accessor_;
};
template<class Accessor>
accessor_storage<Accessor> storage(Accessor const& accessor)
{ return accessor_storage<Accessor>(accessor); }

// resolve_storage(optional_storage, fallback) returns an accessor to the
// optional storage iff it's valid and to the fallback storage otherwise.
template<class T>
accessor_mux<indirect_accessor<T>,inout_accessor<T> >
resolve_storage(optional_storage<T> const& s, T* fallback)
{
    return select_accessor(s.storage != 0, ref(*s.storage), inout(fallback));
}

// STYLING

struct ui_controller : noncopyable
{
    virtual ~ui_controller() {}

    virtual void do_ui(ui_context& ctx) = 0;
};

// widget state flags
struct widget_state_flag_tag {};
typedef flag_set<widget_state_flag_tag> widget_state;
// primary state
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x01, WIDGET_NORMAL)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x02, WIDGET_DISABLED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x03, WIDGET_HOT)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x04, WIDGET_DEPRESSED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x05, WIDGET_SELECTED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x0f, WIDGET_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x10, WIDGET_FOCUSED)

struct style_search_path;
struct dispatch_table;
struct primary_style_properties;

struct style_state
{
    style_search_path const* path;
    dispatch_table const* theme;
    primary_style_properties const* properties;
    id_interface const* id;
};

struct layout_style_info;

struct scoped_substyle : noncopyable
{
    scoped_substyle() : ctx_(0) {}
    scoped_substyle(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, substyle_name, state); }
    ~scoped_substyle() { end(); }

    void begin(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL);
    void end();

 private:
    ui_context* ctx_;
    style_state old_state_;
    layout_style_info const* old_style_info_;
};

// CULLING

struct culling_block
{
    culling_block() {}
    culling_block(ui_context& ctx) { begin(ctx); }
    ~culling_block() { end(); }
    void begin(ui_context& ctx);
    void end();
    bool is_relevant() const { return srr_.is_relevant(); }
 private:
    scoped_routing_region srr_;
};

#define alia_if_relevant_(ctx) \
    { \
        ::alia::culling_block alia__culling_block(ctx); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__culling_block.is_relevant()); \
            if (alia__culling_block.is_relevant()) \
            {

#define alia_if_relevant alia_if_relevant_(ctx)

// UI CACHING

struct layout_node;

struct cached_ui_block
{
    cached_ui_block() : ctx_(0) {}
    cached_ui_block(ui_context& ctx, id_interface const& id)
    { begin(ctx, id); }
    ~cached_ui_block() { end(); }
    void begin(ui_context& ctx, id_interface const& id);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    scoped_routing_region routing_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

#define alia_cached_ui_block_(ctx, id) \
    { \
        ::alia::cached_ui_block alia__cached_ui_block(ctx, id); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__cached_ui_block.is_relevant()); \
            if (alia__cached_ui_block.is_relevant()) \
            {

#define alia_cached_ui_block(id) alia_cached_ui_block_(ctx, id)

// UI CONTEXT

struct ui_system;
struct surface;
struct ui_event;
struct ui_caching_node;

struct ui_context
{
    ui_system* system;
    data_traversal data;
    geometry_context geometry;
    layout_traversal layout;
    alia::surface* surface;
    ui_event* event;
    event_routing_traversal routing;
    style_state style;
    ui_caching_node* active_cacher;
    bool pass_aborted;
};

static inline data_traversal& get_data_traversal(ui_context& ctx)
{ return ctx.data; }
static inline layout_traversal& get_layout_traversal(ui_context& ctx)
{ return ctx.layout; }

void end_pass(ui_context& ctx);

struct untyped_ui_value
{
    virtual ~untyped_ui_value() {}
};

template<class T>
struct typed_ui_value : untyped_ui_value
{
    T value;
};

}

#endif
