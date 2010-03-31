#ifndef ALIA_DROP_DOWN_MENU_HPP
#define ALIA_DROP_DOWN_MENU_HPP

#include <alia/accessor.hpp>
#include <alia/panel.hpp>
#include <alia/flags.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class drop_down_menu : boost::noncopyable
{
 public:
    drop_down_menu() : active_(false) {}
    drop_down_menu(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~drop_down_menu() { end(); }

    void begin(context& ctx, layout const& layout_spec = default_layout);
    void end();

    bool do_list() const { return do_list_; }
    context& get_list_context() const { return *list_ctx_; }

 private:
    bool handle_key_press(key_event_info& info, int& selection);
    int clamp_selection(int selection);
    void set_selection(accessor<unsigned> const& accessor, int selection);
    friend class ddm_item;
    context* ctx_;
    struct data;
    data* data_;
    bool active_;
    region_id id_;
    panel panel_;
    bool do_list_;
    context* list_ctx_;
    scrollable_panel list_panel_;
};

class ddm_item : boost::noncopyable
{
 public:
    ddm_item() : active_(false) {}
    ddm_item(drop_down_menu& menu) { begin(menu); }
    ~ddm_item() { end(); }
    void begin(drop_down_menu& menu);
    void end();
    bool selected() const { return selected_; }
 private:
    bool active_, selected_;
    panel panel_;
};

}

#endif
