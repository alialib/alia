#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <scn/scn.h>
#include <scn/tuple_return.h>

#include <alia.hpp>

#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/storage.hpp>
#include <alia/html/widgets.hpp>

using namespace alia;
using namespace html;

using std::string;

#define ALIA_PP_CONCAT(a, b) ALIA_PP_CONCAT1(a, b)
#define ALIA_PP_CONCAT1(a, b) ALIA_PP_CONCAT2(a, b)
#define ALIA_PP_CONCAT2(a, b) a##b

#define ALIA_PP_FE_2_0(F, a, b)
#define ALIA_PP_FE_2_1(F, a, b, x) F(a, b, x)
#define ALIA_PP_FE_2_2(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_1(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_3(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_2(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_4(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_3(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_5(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_4(F, a, b, __VA_ARGS__)

#define ALIA_PP_GET_MACRO(_0, _1, _2, _3, _4, _5, NAME, ...) NAME
#define ALIA_PP_FOR_EACH_2(F, a, b, ...)                                      \
    ALIA_PP_GET_MACRO(                                                        \
        _0,                                                                   \
        __VA_ARGS__,                                                          \
        ALIA_PP_FE_2_5,                                                       \
        ALIA_PP_FE_2_4,                                                       \
        ALIA_PP_FE_2_3,                                                       \
        ALIA_PP_FE_2_2,                                                       \
        ALIA_PP_FE_2_1,                                                       \
        ALIA_PP_FE_2_0)                                                       \
    (F, a, b, __VA_ARGS__)

#define ALIA_DEFINE_STRUCT_SIGNAL_FIELD(signal_type, struct_name, field_name) \
    auto ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)()         \
    {                                                                         \
        return (*this)->*&struct_name::field_name;                            \
    }                                                                         \
    __declspec(property(                                                      \
        get = ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)))    \
        alia::field_signal<                                                   \
            ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),      \
            decltype(struct_name::field_name)>                                \
            field_name;

#define ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(signal_type, struct_name, ...)       \
    ALIA_PP_FOR_EACH_2(                                                       \
        ALIA_DEFINE_STRUCT_SIGNAL_FIELD,                                      \
        signal_type,                                                          \
        struct_name,                                                          \
        __VA_ARGS__)

#define ALIA_DEFINE_STRUCT_SIGNAL(signal_type, struct_name, ...)              \
    struct ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name)        \
        : signal_type<struct_name>                                            \
    {                                                                         \
        using signal_ref::signal_ref;                                         \
        ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(                                     \
            signal_type, struct_name, __VA_ARGS__)                            \
    };

template<class Label, class Type, class Value>
void
labeled_form_input(html::context ctx, Label label, Type type, Value value)
{
    element(ctx, "div").attr("class", "mb-3").children([&] {
        element(ctx, "label").attr("class", "form-label").text(label);
        input(ctx, value).attr("type", type).attr("class", "form-control");
    });
}

#define ALIA_AGGREGATOR(f) [](auto&&... args) { return f{args...}; }

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

void
page_header(html::context ctx)
{
    element(ctx, "header")
        .attr("class", "navbar navbar-expand-md navbar-dark")
        .children([&] {
            div(ctx, "container-xl", [&] {
                link(ctx, "Some App", noop_action())
                    .attr(
                        "class", "navbar-brand navbar-brand-autodark mb-0 h1");
            });
        });
}

template<class Title>
void
page_title(html::context ctx, Title title)
{
    div(ctx, "page-header", [&] {
        div(ctx, "row align-items-center", [&] {
            div(ctx, "col-auto", [&] {
                element(ctx, "h2").attr("class", "page-title").text(title);
            });
        });
    });
}

struct document_object
{
    document_object() : val_(emscripten::val::global("document"))
    {
    }

    void
    set_title(std::string title)
    {
        val_.set("title", std::move(title));
    }
    std::string
    get_title()
    {
        return val_["title"].as<std::string>();
    }
    __declspec(property(get = get_title, put = set_title)) std::string title;

    void
    set_body(emscripten::val body)
    {
        val_.set("body", std::move(body));
    }
    emscripten::val
    get_body()
    {
        return val_["body"];
    }
    __declspec(property(get = get_body, put = set_body)) emscripten::val body;

 private:
    emscripten::val val_;
};

void
document_title(html::context ctx, readable<std::string> title)
{
    auto& id = get_data<captured_id>(ctx);
    refresh_signal_view(
        id,
        add_default(title, ""),
        [](auto new_title) {
            emscripten::val::global("document").set("title", new_title);
        },
        [] {});
}

auto
get_storage_state(
    html::context ctx,
    std::string const& storage_name,
    std::string const& key,
    readable<std::string> default_value)
{
    return add_write_action(
        get_state(
            ctx,
            add_default(
                lambda_reader(
                    [=] { return storage_object(storage_name).has_item(key); },
                    [=] {
                        return storage_object(storage_name).get_item(key);
                    }),
                default_value)),
        callback([=](std::string new_value) {
            storage_object(storage_name).set_item(key, new_value);
        }));
}

auto
get_session_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value)
{
    return get_storage_state(ctx, "sessionStorage", key, default_value);
}

auto
get_local_state(
    html::context ctx,
    std::string const& key,
    readable<std::string> default_value)
{
    return get_storage_state(ctx, "localStorage", key, default_value);
}

template<class Context, class Ui>
void
full_page_ui(Context ctx, Ui ui)
{
    div(ctx, "page", [&] {
        page_header(ctx);
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
    text(ctx, "Enter a number:");
    input(ctx, n);
    alia_switch(n)
    {
        alia_case(0):
            text(ctx, "foo");
            break;
        alia_case(1):
            text(ctx, "bar");
        alia_case(2):
        alia_case(3):
            text(ctx, "baz");
            break;
        alia_default:
            text(ctx, "zub");
    }
    alia_end
    // clang-format on
}

void
modal(html::context ctx)
{
    div(ctx, "modal-dialog modal-dialog-centered")
        .attr("role", "document")
        .children([&] {
            div(ctx, "modal-content", [&] {
                div(ctx, "modal-header", [&] {
                    element(ctx, "h5")
                        .class_("modal-title")
                        .text("New report");
                    element(ctx, "button")
                        .attr("type", "button")
                        .class_("close")
                        .attr("data-dismiss", "modal")
                        .attr("aria-label", "Close")
                        .children([&] {
                            element(ctx, "span")
                                .attr("aria-hidden", "true")
                                .text(u8"\u00D7");
                        });
                });
                div(ctx, "modal-body", [&] {
                    element(ctx, "p").text("Some text...");
                });
                div(ctx, "modal-footer", [&] {
                    button(ctx, "Close", callback([&] {
                               emscripten_run_script(
                                   "jQuery(\"#alia-modal\").modal('hide');");
                           }));
                    element(ctx, "button")
                        .attr("type", "button")
                        .class_("btn", "btn-primary")
                        .text("Save changes");
                });
            });
        });
}

template<class Content>
void
invoke_tree(
    html::context ctx, tree_node<element_object>& root, Content&& content)
{
    scoped_tree_root<element_object> scoped_root;
    if (is_refresh_event(ctx))
        scoped_root.begin(get<html::tree_traversal_tag>(ctx), root);
    (std::forward<Content>(content))();
}

struct modal_data
{
    element_object root;
};

// void
// activate_modal(html::context ctx, modal_data& data)
// {
//     // Create a top-level DOM element to hold the modal.
//     emscripten::val document = emscripten::val::global("document");
//     emscripten::val modal_root = document.call<emscripten::val>(
//         "createElement", emscripten::val("div"));
//     document["body"].call<emscripten::val>("appendChild", modal_root);
//     auto asmdom_id = asmdom::direct::toElement(modal_root);
//     asmdom::direct::setAttribute(asmdom_id, "class", "modal fade");
//     asmdom::direct::setAttribute(asmdom_id, "id", "alia-modal");
//     asmdom::direct::setAttribute(asmdom_id, "tabindex", "-1");
//     asmdom::direct::setAttribute(asmdom_id, "role", "dialog");
//     data.root.object.asmdom_id = asmdom_id;

//     // Install a callback to track when the modal is hidden.
//     asmdom::direct::setCallback(
//         dom_system.modal_root.object.asmdom_id,
//         "hide.bs.modal",
//         [&dom_system](emscripten::val) {
//             dom_system.active_modal = nullptr;
//             return true;
//         });
// }

void
main_page(html::context ctx)
{
    do_switch_test(ctx);

    tree_node<element_object>* modal_root;
    if (get_cached_data(ctx, &modal_root))
        create_as_modal_root(modal_root->object);

    int modal_asmdom_id;

    // invoke_pure_component(ctx, [&](auto ctx) {
    invoke_tree(ctx, *modal_root, [&] {
        auto& top_level_modal = element(ctx, "div")
                                    .class_("modal", "fade")
                                    .attr("id", "alia-modal")
                                    .attr("tabindex", "-1")
                                    .attr("role", "dialog")
                                    .children([&] { modal(ctx); });
        modal_asmdom_id = top_level_modal.asmdom_id();
    });
    //});

    // on_init(ctx, callback([&] {
    //             emscripten_run_script(
    //                 "jQuery(\"#alia-modal\").modal('show');");
    //         }));
    // button(ctx, "Activate", callback([&] {
    //            EM_ASM(
    //                { jQuery("#alia-modal").modal("show");");
    //        }));

    button(ctx, "Activate", callback([&] {
               EM_ASM(
                   { jQuery(Module['nodes'][$0]).modal('show'); },
                   modal_asmdom_id);
           }));
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
