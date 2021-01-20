#include <alia/html/dom.hpp>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <chrono>

namespace alia { namespace html {

void
create_as_element(element_object& object, char const* type)
{
    assert(object.asmdom_id == 0);
    std::cout << "asmdom::direct::createElement: " << type << std::endl;
    object.asmdom_id = asmdom::direct::createElement(type);
    std::cout << "-> " << object.asmdom_id << std::endl;
    object.type = element_object::NORMAL;
}

void
create_as_text_node(element_object& object, char const* value)
{
    assert(object.asmdom_id == 0);
    std::cout << "asmdom::direct::createTextNode: " << value << std::endl;
    object.asmdom_id = asmdom::direct::createTextNode(value);
    std::cout << "-> " << object.asmdom_id << std::endl;
    object.type = element_object::NORMAL;
}

void
create_as_existing(element_object& object, emscripten::val node)
{
    assert(object.asmdom_id == 0);
    object.asmdom_id = asmdom::direct::toElement(node);
    object.type = element_object::NORMAL;
}

void
create_as_body(element_object& object)
{
    assert(object.asmdom_id == 0);
    object.asmdom_id = asmdom::direct::toElement(
        emscripten::val::global("document")["body"]);
    object.type = element_object::BODY;
}

void
create_as_placeholder_root(element_object& object, emscripten::val placeholder)
{
    assert(object.asmdom_id == 0);
    object.type = element_object::PLACEHOLDER_ROOT;
    object.asmdom_id = asmdom::direct::toElement(placeholder);
    asmdom::direct::toElement(placeholder["parentNode"]);
}

void
create_as_placeholder_root(element_object& object, char const* placeholder_id)
{
    emscripten::val document = emscripten::val::global("document");
    emscripten::val placeholder = document.call<emscripten::val>(
        "getElementById", emscripten::val(placeholder_id));
    // Strip out the ID to creating duplicate IDs through template reuse.
    // It's not longer needed once we've find it.
    placeholder.call<void>("removeAttribute", emscripten::val("id"));
    create_as_placeholder_root(object, placeholder);
}

void
create_as_modal_root(element_object& object)
{
    object.type = element_object::MODAL_ROOT;
}

void
element_object::relocate(
    element_object& new_parent, element_object* after, element_object* before)
{
    assert(this->type == element_object::NORMAL);
    assert(this->asmdom_id != 0);
    assert(new_parent.type != element_object::UNINITIALIZED);
    switch (new_parent.type)
    {
        case element_object::NORMAL:
        case element_object::BODY:
            assert(new_parent.asmdom_id != 0);
            std::cout << "asmdom::direct::insertBefore: "
                      << new_parent.asmdom_id << ", " << this->asmdom_id
                      << ", " << (before ? before->asmdom_id : 0) << std::endl;
            asmdom::direct::insertBefore(
                new_parent.asmdom_id,
                this->asmdom_id,
                before ? before->asmdom_id : 0);
            break;
        case element_object::PLACEHOLDER_ROOT:
            assert(new_parent.asmdom_id != 0);
            asmdom::direct::insertBefore(
                asmdom::direct::parentNode(new_parent.asmdom_id),
                this->asmdom_id,
                before ? before->asmdom_id : new_parent.asmdom_id);
            break;
        case element_object::MODAL_ROOT: {
            emscripten::val document = emscripten::val::global("document");
            document["body"].call<emscripten::val>(
                "appendChild", asmdom::direct::getElement(this->asmdom_id));
            break;
        }
        case element_object::UNINITIALIZED:
            // Suppress warnings.
            break;
    }
}

void
element_object::remove()
{
    assert(this->asmdom_id != 0);
    std::cout << "asmdom::direct::remove: " << this->asmdom_id << std::endl;
    asmdom::direct::remove(this->asmdom_id);
}

element_object::~element_object()
{
    if (this->is_initialized())
    {
        this->remove();
        std::cout << "asmdom::direct::deleteElement: " << this->asmdom_id
                  << std::endl;
        if (this->type != element_object::BODY)
            asmdom::direct::deleteElement(this->asmdom_id);
    }
}

namespace detail {

void
callback_proxy(std::uintptr_t callback, emscripten::val event)
{
    (*reinterpret_cast<std::function<void(emscripten::val)>*>(callback))(
        event);
};

EMSCRIPTEN_BINDINGS(callback_proxy)
{
    emscripten::function(
        "callback_proxy", &callback_proxy, emscripten::allow_raw_pointers());
};

void
install_onpopstate_callback(
    std::function<void(emscripten::val)> const* function)
{
    EM_ASM(
        {
            window.onpopstate = function(event)
            {
                Module.callback_proxy($0, event);
            };
        },
        reinterpret_cast<std::uintptr_t>(function));
}

void
install_onhashchange_callback(
    std::function<void(emscripten::val)> const* function)
{
    EM_ASM(
        {
            window.onhashchange = function()
            {
                Module.callback_proxy($0, null);
            };
        },
        reinterpret_cast<std::uintptr_t>(function));
}

void
install_element_callback(
    context ctx,
    element_object& object,
    callback_data& data,
    char const* event_type)
{
    std::cout << "install callback" << std::endl;
    auto external_id = externalize(&data.identity);
    auto* system = &get<alia::system_tag>(ctx);
    auto callback = [=](emscripten::val v) {
        dom_event event(v);
        auto start = std::chrono::high_resolution_clock::now();
        dispatch_targeted_event(*system, event, external_id);
        auto elapsed_ms
            = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now() - start)
                  .count();
        std::cout << "event time: " << elapsed_ms << " µs" << std::endl;
        return true;
    };
    asmdom::direct::setCallback(object.asmdom_id, event_type, callback);
    std::cout << "worked!" << std::endl;
}

struct text_node_data
{
    tree_node<element_object> node;
    captured_id value_id;
};

void
text_node(html::context ctx, readable<std::string> text)
{
    text_node_data* data;
    if (get_cached_data(ctx, &data))
        create_as_text_node(data->node.object, "");
    if (is_refresh_event(ctx))
    {
        refresh_tree_node(get<tree_traversal_tag>(ctx), data->node);
        refresh_signal_view(
            data->value_id,
            text,
            [&](std::string const& new_value) {
                std::cout << "asmdom::direct::setNodeValue: "
                          << data->node.object.asmdom_id << ": " << new_value
                          << std::endl;
                asmdom::direct::setNodeValue(
                    data->node.object.asmdom_id, new_value.c_str());
            },
            [&]() {
                std::cout << "asmdom::direct::setNodeValue: "
                          << data->node.object.asmdom_id << ": (null)"
                          << std::endl;
                asmdom::direct::setNodeValue(data->node.object.asmdom_id, "");
            });
    }
}

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<std::string> value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            stored_id,
            value,
            [&](std::string const& new_value) {
                asmdom::direct::setAttribute(
                    object.asmdom_id, name, new_value.c_str());
            },
            [&]() {
                asmdom::direct::removeAttribute(object.asmdom_id, name);
            });
    });
}

void
do_element_attribute(
    context ctx,
    element_object& object,
    char const* name,
    readable<bool> value)
{
    auto& stored_id = get_cached_data<captured_id>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            stored_id,
            value,
            [&](bool new_value) {
                if (new_value)
                    asmdom::direct::setAttribute(object.asmdom_id, name, "");
                else
                    asmdom::direct::removeAttribute(object.asmdom_id, name);
            },
            [&]() {
                asmdom::direct::removeAttribute(object.asmdom_id, name);
            });
    });
}

struct element_class_token_data
{
    std::string existing_value;
    captured_id value_id;
};

void
clear_class_token(int asmdom_id, std::string const& token)
{
    EM_ASM(
        {
            var node = Module['nodes'][$0];
            node.classList.remove(Module['UTF8ToString']($1));
        },
        asmdom_id,
        token.c_str());
}

void
add_class_token(int asmdom_id, std::string const& token)
{
    EM_ASM(
        {
            var node = Module['nodes'][$0];
            node.classList.add(Module['UTF8ToString']($1));
        },
        asmdom_id,
        token.c_str());
}

void
do_element_class_token(
    context ctx, element_object& object, bool, readable<std::string> value)
{
    auto& data = get_cached_data<element_class_token_data>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            data.value_id,
            value,
            [&](std::string const& new_value) {
                if (!data.existing_value.empty())
                {
                    clear_class_token(object.asmdom_id, data.existing_value);
                }
                add_class_token(object.asmdom_id, new_value);
                data.existing_value = new_value;
            },
            [&]() {
                if (!data.existing_value.empty())
                {
                    clear_class_token(object.asmdom_id, data.existing_value);
                    data.existing_value.clear();
                }
            });
    });
}

void
do_element_class_token(
    context ctx, element_object& object, bool initializing, char const* value)
{
    on_refresh(ctx, [&](auto ctx) {
        if (initializing)
            add_class_token(object.asmdom_id, value);
    });
}

void
set_element_property(
    element_object& object, char const* name, emscripten::val const& value)
{
    std::cout << "asmdom::direct::setProperty: " << object.asmdom_id << "."
              << name << ": " << value.as<std::string>() << std::endl;
    asmdom::direct::setProperty(object.asmdom_id, name, value);
}

void
clear_element_property(element_object& object, char const* name)
{
    std::cout << "asmdom::direct::removeProperty: " << object.asmdom_id << "."
              << name << std::endl;
    asmdom::direct::removeProperty(object.asmdom_id, name);
}

} // namespace detail

scoped_element&
scoped_element::begin(context ctx, char const* type)
{
    ctx_.reset(ctx);
    initializing_ = get_cached_data(ctx, &node_);
    if (initializing_)
        create_as_element(node_->object, type);
    if (is_refresh_event(ctx))
        tree_scoping_.begin(get<tree_traversal_tag>(ctx), *node_);
    return *this;
}

}} // namespace alia::html
