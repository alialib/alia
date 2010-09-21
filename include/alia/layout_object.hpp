#ifndef ALIA_LAYOUT_OBJECT_HPP
#define ALIA_LAYOUT_OBJECT_HPP

#include <alia/layout_interface.hpp>
#include <alia/event.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class layout_object : boost::noncopyable
{
 public:
    layout_object() : active_(false) {}
    box2i const& get_region() const;
    bool is_relevant() const;
    bool is_dirty() const;
    // When the current event is a targeted event, this will detect if the
    // target is inside this layout object. It should only be called after the
    // contents of the layout object have been invoked.
    bool contains_target() const;
 protected:
    context& get_context() const { return *ctx_; }
    void begin(context& ctx, layout_object_data& data,
        layout const& layout_spec, layout_logic* logic);
    void end();
    bool logic_needed() { return logic_needed_; }
    bool is_active() { return active_; }
 private:
    context* ctx_;
    layout_object_data* data_;
    bool active_, logic_needed_, already_saw_target_;
    layout layout_spec_;
    layout_logic* old_logic_;
    layout_data* old_data_;
    layout_data** old_next_ptr_;
};

}

#endif
