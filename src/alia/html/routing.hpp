#ifndef ALIA_HTML_ROUTING_HPP
#define ALIA_HTML_ROUTING_HPP

#include <alia/html/context.hpp>
#include <alia/html/history.hpp>
#include <alia/html/system.hpp>

#include <scn/scn.h>

namespace alia { namespace html { namespace detail {

// A small/experimental routing framework for alia/HTML.
// See the docs for usage and examples.

struct path_component
{
    std::string_view text;
};

}}} // namespace alia::html::detail

template<typename Char>
struct scn::scanner<Char, alia::html::detail::path_component>
{
    template<typename ParseContext>
    error
    parse(ParseContext& pctx)
    {
        using char_type = typename ParseContext::char_type;
        pctx.arg_begin();
        if (SCN_UNLIKELY(!pctx))
        {
            return error(
                error::invalid_format_string, "Unexpected format string end");
        }
        for (auto ch = pctx.next(); pctx && !pctx.check_arg_end();
             pctx.advance(), ch = pctx.next())
        {
            if (ch == detail::ascii_widen<char_type>('/'))
                is_subpath = true;
            else
                break;
        }
        if (!pctx.check_arg_end())
        {
            return error(
                error::invalid_format_string, "Expected argument end");
        }
        pctx.arg_end();
        return {};
    }

    template<typename Context>
    error
    scan(alia::html::detail::path_component& val, Context& ctx)
    {
        using char_type = typename Context::char_type;
        auto is_separator = [&](char_type ch) {
            return !is_subpath
                   && ch == scn::detail::ascii_widen<char_type>('/');
        };
        if (!Context::range_type::is_contiguous)
        {
            return error(
                error::invalid_operation,
                "Cannot read a path component from a non-contiguous range");
        }
        // Allow empty subpaths.
        if (is_subpath && ctx.range().begin() == ctx.range().end())
        {
            val.text = std::string_view();
            return {};
        }
        auto s = read_until_space_zero_copy(ctx.range(), is_separator, false);
        if (!s)
            return s.error();
        val.text
            = basic_string_view<char_type>(s.value().data(), s.value().size());
        return {};
    }

    bool is_subpath = false;
};

namespace alia { namespace html {

namespace detail {

template<unsigned N>
constexpr unsigned
parameter_count(const char (&pattern)[N], unsigned i = 0, unsigned count = 0)
{
    return i == N - 1
               ? count
               : parameter_count(
                   pattern, i + 1, pattern[i] == '{' ? count + 1 : count);
}

template<unsigned N>
struct route_parse_result
{
    bool matched = false;
    std::array<std::string, N> arguments;
};

template<std::size_t N, typename = std::make_index_sequence<N>>
struct route_parser
{
};

template<std::size_t N, std::size_t... S>
struct route_parser<N, std::index_sequence<S...>>
{
    char const* pattern;

    route_parse_result<N>
    operator()(std::string const& route)
    {
        path_component components[N];
        auto scan_result
            = scn::scan(scn::make_view(route), pattern, components[S]...);
        route_parse_result<N> result;
        result.matched = scan_result && scan_result.range().empty();
        for (std::size_t i = 0; i != N; ++i)
            result.arguments[i] = std::string(components[i].text);
        return result;
    }
};

template<>
struct route_parser<0>
{
    char const* pattern;

    route_parse_result<0>
    operator()(std::string const& route)
    {
        route_parse_result<0> result;
        result.matched = route == pattern;
        return result;
    }
};

template<std::size_t N, typename = std::make_index_sequence<N>>
struct page_invoker
{
};

template<std::size_t N, std::size_t... S>
struct page_invoker<N, std::index_sequence<S...>>
{
    template<class Context, class Page, class Result>
    static void
    invoke(
        Context ctx, Page&& page, Result const& result, bool& already_matched)
    {
        ALIA_IF(!already_matched && ALIA_FIELD(result, matched))
        {
            auto args = ALIA_FIELD(result, arguments);
            std::forward<Page>(page)(args[S]...);
            already_matched = true;
        }
        ALIA_END
    }
};

template<std::size_t>
using route_arg_n = readable<std::string>;

template<class Page, std::size_t N, typename = std::make_index_sequence<N>>
struct page_has_n_arity
{
};

template<class Page, std::size_t N, std::size_t... S>
struct page_has_n_arity<Page, N, std::index_sequence<S...>>
    : std::is_invocable<Page, route_arg_n<S>...>
{
};

template<class Page, std::size_t N = 0>
struct page_arity : std::conditional_t<
                        page_has_n_arity<Page, N>::value,
                        std::integral_constant<std::size_t, N>,
                        page_arity<Page, N + 1>>
{
};

} // namespace detail

struct router_data
{
    captured_id path_id;
    alia::state_storage<std::string> path;
};

template<class Context>
struct router_handle
{
    Context ctx;
    alia::state_storage<std::string>& path;
    bool already_matched;

    template<class Page>
    router_handle&
    route(char const* pattern, Page&& page)
    {
        std::size_t constexpr N = detail::page_arity<Page>::value;
        detail::route_parser<N> parser{pattern};
        auto parse_result = alia::apply(ctx, parser, make_state_signal(path));
        detail::page_invoker<N>::invoke(
            ctx, std::forward<Page>(page), parse_result, already_matched);
        return *this;
    }
};

template<class Context>
router_handle<Context>
router(Context ctx, readable<std::string> path)
{
    auto& data = get_cached_data<router_data>(ctx);
    on_refresh(ctx, [&](auto ctx) {
        refresh_signal_view(
            data.path_id,
            path,
            [&](std::string const& new_value) { data.path.set(new_value); },
            [&]() { data.path.clear(); });
    });
    return router_handle<Context>{ctx, data.path, false};
}

template<class Context>
router_handle<Context>
router(Context ctx)
{
    return router(
        ctx,
        apply(
            ctx,
            [](auto path) {
                return (!path.empty() && path[0] == '#') ? path.substr(1)
                                                         : std::string();
            },
            get_location_hash(ctx)));
}

namespace actions { namespace {

// An action that sets the location hash.
auto
set_location_hash(html::context ctx)
{
    auto& sys = get<html::system_tag>(ctx);
    return callback([&sys](std::string new_route) {
        set_location_hash(sys, std::move(new_route));
    });
}

}} // namespace actions::

// Get the current location hash for the HTML context.
direct_const_signal<std::string>
get_location_hash(html::context ctx);

// internal_link() implements an 'internal' link to a different page in an SPA.

namespace detail {

element_handle
internal_link(
    context ctx, readable<std::string> text, readable<std::string> path);

element_handle
internal_link(context ctx, readable<std::string> path);

} // namespace detail

// with a label
template<class Text, class Path>
element_handle
internal_link(context ctx, Text text, Path path)
{
    return detail::internal_link(ctx, signalize(text), signalize(path));
}

// without a label - You provide the content.
template<class Path>
element_handle
internal_link(context ctx, Path path)
{
    return detail::internal_link(ctx, signalize(path));
}

}} // namespace alia::html

#endif
