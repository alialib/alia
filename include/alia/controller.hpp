#ifndef ALIA_CONTROLLER_HPP
#define ALIA_CONTROLLER_HPP

namespace alia {

struct context;

class controller
{
 public:
    virtual void do_ui(context& ctx) = 0;
    virtual ~controller() {}
};

class controller_function : controller
{
 public:
    controller_function(void(*fn)(context& ctx)) : fn_(fn) {}
    void do_ui(context& ctx) { fn_(ctx); }
 private:
    void(*fn_)(context& ctx);
};

static inline controller_function* make_controller(void(*fn)(context& ctx))
{ return new controller_function(fn); }

}

#endif
