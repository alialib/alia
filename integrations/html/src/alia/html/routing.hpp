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

// scnlib-style predicate for testing if a character is a path separator
template<typename CharT>
struct is_path_separator
{
    using char_type = CharT;

    // If this is set, slashes don't count as separators because we're allowing
    // full subpaths to be parsed.
    bool allow_subpaths = false;

    bool
    operator()(scn::span<const char_type> ch)
    {
        return !allow_subpaths
            && ch[0] == scn::detail::ascii_widen<char_type>('/');
    }

    constexpr bool
    is_multibyte() const
    {
        return false;
    }
};

}}} // namespace alia::html::detail

template<>
struct scn::scanner<alia::html::detail::path_component> : scn::common_parser
{
    template<typename ParseContext>
    error
    parse(ParseContext& pctx)
    {
        using char_type = typename ParseContext::char_type;

        std::array<char_type, 3> options{
            {scn::detail::ascii_widen<char_type>('/')}};
        auto e = parse_common(
            pctx,
            span<const char_type>{options.begin(), options.end()},
            span<bool>{&is_subpath, 3},
            null_type_cb<ParseContext>);
        if (!e)
        {
            return e;
        }

        return {};
    }

    template<typename Context>
    error
    scan(alia::html::detail::path_component& val, Context& ctx)
    {
        using char_type = typename Context::char_type;
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
        auto s = read_until_space_zero_copy(
            ctx.range(),
            alia::html::detail::is_path_separator<char_type>{is_subpath},
            false);
        if (!s)
            return s.error();
        val.text
            = basic_string_view<char_type>(s.value().data(), s.value().size());
        return {};
    }

    bool is_subpath = false;
};

namespace alia {
namespace html {

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
        auto scan_result = scn::scan(route, pattern, components[S]...);
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
        ALIA_IF (!already_matched && ALIA_FIELD(result, matched))
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
struct page_arity
    : std::conditional_t<
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
    refresh_handler(ctx, [&](auto ctx) {
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

} // namespace html

namespace actions {

// An action that sets the location hash.
inline auto
set_location_hash(html::context ctx)
{
    auto& sys = get<html::system_tag>(ctx);
    return callback([&sys](std::string new_route) {
        set_location_hash(sys, std::move(new_route));
    });
}

} // namespace actions

namespace html {

// Get the current location hash for the HTML context.
direct_const_signal<std::string>
get_location_hash(html::context ctx);

} // namespace html
} // namespace alia

#endif
