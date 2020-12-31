#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <scn/scn.h>
#include <scn/tuple_return.h>

#include <alia.hpp>

#include <alia/html/bootstrap.hpp>
#include <alia/html/document.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/history.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/storage.hpp>
#include <alia/html/widgets.hpp>

using namespace alia;
using namespace html;
using namespace bootstrap;

using std::string;

template<class Label, class Type, class Value>
void
labeled_form_input(html::context ctx, Label label, Type type, Value value)
{
    element(ctx, "div").attr("class", "mb-3").children([&] {
        element(ctx, "label").attr("class", "form-label").text(label);
        input(ctx, value).attr("type", type).attr("class", "form-control");
    });
}

template<class Context, class Dialog>
void
centered_dialog(Context ctx, Dialog dialog)
{
    div(ctx,
        "h-100 flex-fill d-flex flex-column justify-content-center "
        "align-items-center",
        [&] {
            div(ctx, "container-tight", [&] {
                dialog(ctx);
                div(ctx, "my-5 py-5");
            });
        });
}

template<class LinkContents, class Menu>
void
dropdown_link(html::context ctx, LinkContents link_contents, Menu menu)
{
    div(ctx, "dropdown", [&] {
        element(ctx, "a")
            .attr("class", "btn-lg")
            .attr("type", "button")
            .attr("data-toggle", "dropdown")
            .children(link_contents);
        div(ctx, "dropdown-menu dropdown-menu-left").children(menu);
    });
}

template<class ButtonContents, class Menu>
void
dropdown_button(html::context ctx, ButtonContents button_contents, Menu menu)
{
    div(ctx, "dropdown", [&] {
        element(ctx, "button")
            .attr("class", "btn-lg")
            .attr("type", "button")
            .attr("data-toggle", "dropdown")
            .children(button_contents);
        div(ctx, "dropdown-menu dropdown-menu-left").children(menu);
    });
}

template<class Context, class Ui>
void
full_page_ui(Context ctx, Ui ui)
{
    div(ctx, "page", [&] {
        // page_header(ctx);
        alia_try
        {
            div(ctx, "content", [&] { div(ctx, "container-xl", ui); });
        }
        alia_catch(...)
        {
            div(ctx, "empty", [&] {
                element(ctx, "p")
                    .attr("class", "empty-title h3")
                    .text("Oops... Something went wrong.");
            });
        }
        alia_end
    });
}

void
clear_canvas(int asmdom_id)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.clearRect(0, 0, canvas.width, canvas.height);
        },
        asmdom_id);
}

void
set_fill_style(int asmdom_id, char const* style)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.fillStyle = Module['UTF8ToString']($1);
        },
        asmdom_id,
        style);
}

void
fill_rect(int asmdom_id, double x, double y, double width, double height)
{
    EM_ASM(
        {
            var ctx = Module['nodes'][$0].getContext('2d');
            ctx.fillRect($1, $2, $3, $4);
        },
        asmdom_id,
        x,
        y,
        width,
        height);
}

void
demo_ui(html::context ctx)
{
    auto& elm = element(ctx, "canvas").attr("id", "canvas")
        /*.on_init([&](auto& elm) { draw(); })*/;

    on_refresh(ctx, [&](auto ctx) {
        auto canvas = elm.asmdom_id();
        clear_canvas(canvas);

        auto x = get_raw_animation_tick_count(ctx) / 40.f;

        set_fill_style(canvas, "rgb(200, 0, 0)");
        fill_rect(canvas, x, 10, 50, 50);

        set_fill_style(canvas, "rgba(0, 0, 200, 0.5)");
        fill_rect(canvas, 30, 30, 50, 50);
    });
}

void
do_switch_test(html::context ctx)
{
    auto n = enforce_validity(ctx, get_state(ctx, empty<int>()));

    // clang-format off
    p(ctx, "Enter a number:");
    input(ctx, n);
    alia_switch(n)
    {
        alia_case(0):
            p(ctx, "foo");
            break;
        alia_case(1):
            p(ctx, "bar");
        alia_case(2):
        alia_case(3):
            p(ctx, "baz");
            break;
        alia_default:
            p(ctx, "zub");
    }
    alia_end
    // clang-format on
}

void
main_page(html::context ctx)
{
    // do_switch_test(ctx);

    tree_node<element_object>* modal_root;
    if (get_cached_data(ctx, &modal_root))
        create_as_modal_root(modal_root->object);

    auto some_string = get_state(ctx, "Test");
    input(ctx, some_string);

    auto my_modal = modal(ctx, [&] {
        standard_modal_header(ctx, "My Modal");
        modal_body(ctx, [&] {
            element(ctx, "p").text(some_string);

            auto some_int = get_state(ctx, 4);
            input(ctx, some_int);

            alia_if(some_int > 1)
            {
                element(ctx, "p").text("Some text...");
            }
            alia_end
            alia_if(some_int > 10)
            {
                element(ctx, "p").text("Some more text...");
            }
            alia_end
        });
        modal_footer(ctx, [&] {
            bootstrap::button(ctx, "Close", callback([&] { close_modal(); }))
                .class_("btn-secondary");
            element(ctx, "button")
                .attr("type", "button")
                .class_("btn", "btn-primary")
                .text("Save changes");
        });
    });

    bootstrap::button(ctx, callback([&] { my_modal.activate(); }))
        .class_("btn-secondary")
        .children([&] {
            text_node(ctx, "Activate");
            element(ctx, "span")
                .attr("class", "badge badge-primary")
                .text("4");
        });
}

void
root_ui(html::context ctx)
{
    document_title(ctx, value("Some App"));

    full_page_ui(ctx, main_page);
}

int
main()
{
    static alia::system the_sys;
    static html::system the_dom;
    initialize(the_dom, the_sys, "alia-placeholder", root_ui);
};
