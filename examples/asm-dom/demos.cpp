#define ALIA_IMPLEMENTATION
#define ALIA_LOWERCASE_MACROS
#include "alia.hpp"

#include "dom.hpp"

using namespace alia;

static std::map<std::string, std::function<void(std::string dom_id)>> the_demos;

struct demo : noncopyable
{
    demo(std::string name, std::function<void(std::string dom_id)> f)
    {
        the_demos[name] = f;
    }
};

#include "snippets/greeting_ui.cpp"

void
init_greeting_ui(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_greeting_ui(ctx, get_state(ctx, string()));
    });
}

static demo greeting_ui("greeting-ui", init_greeting_ui);

#include "snippets/addition_ui.cpp"

void
init_addition_ui(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_addition_ui(
            ctx,
            get_state(ctx, empty<double>()),
            get_state(ctx, empty<double>()));
    });
}

static demo addition_ui("addition-ui", init_addition_ui);

#include "snippets/addition_analysis.cpp"

void
init_addition_analysis(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_addition_analysis(
            ctx,
            get_state(ctx, empty<double>()),
            get_state(ctx, empty<double>()));
    });
}

static demo addition_analysis("addition-analysis", init_addition_analysis);

extern "C" {

bool
init_demo(char const* name)
{
    auto demo = the_demos.find(name);
    if (demo == the_demos.end())
        return false;
    demo->second(name);
    return true;
}
}
