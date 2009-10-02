#ifndef ALIA_DROP_DOWN_LIST_HPP
#define ALIA_DROP_DOWN_LIST_HPP

#include <alia/accessor.hpp>
#include <alia/panel.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class drop_down_list : boost::noncopyable
{
 public:
    drop_down_list() : active_(false) {}
    // TODO: Ideally, you shouldn't have to specify how many items there are.
    drop_down_list(context& ctx, accessor<unsigned> const& selection,
        unsigned n_items, unsigned flags = 0,
        layout const& layout_spec = default_layout)
    { begin(ctx, selection, n_items, flags, layout_spec); }
    ~drop_down_list()
    { end(); }

    void begin(context& ctx, accessor<unsigned> const& selection,
        unsigned n_items, unsigned flags = 0,
        layout const& layout_spec = default_layout);
    void end();

    bool do_list() const { return do_list_; }
    context& get_list_context() const { return *list_ctx_; }

    unsigned get_selection() const { return selection_; }
    bool changed() const { return changed_; }

 private:
    bool handle_key_press(key_event_info& info, int& selection);
    int clamp_selection(int selection);
    void set_selection(accessor<unsigned> const& accessor, int selection);
    friend class ddl_item;
    context* ctx_;
    struct data;
    data* data_;
    unsigned n_items_, flags_;
    int selection_;
    bool active_, changed_;
    region_id id_;
    panel panel_;
    bool do_list_, make_selection_visible_;
    context* list_ctx_;
    scrollable_panel list_panel_;
    int index_;
};

class ddl_item : boost::noncopyable
{
 public:
    ddl_item() : active_(false) {}
    ddl_item(drop_down_list& list)
    { begin(list); }
    ~ddl_item() { end(); }
    void begin(drop_down_list& list);
    void end();
    bool is_selected() const { return selected_; }
 private:
    drop_down_list* list_;
    bool active_, selected_;
    panel panel_;
};

bool do_drop_down_button(context& ctx, unsigned flags = 0,
    layout const& layout_spec = default_layout, region_id id = auto_id);

}

#endif
