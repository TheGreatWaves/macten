#ifndef MACTEN_UTILS_HPP
#define MACTEN_UTILS_HPP

#include "macten_tokens.hpp"
#include "token_stream.hpp"

#include <map>
#include <optional>

/**
 * Join two tokens together.
 */
#define _JOIN(a, b) a##b

namespace macten
{
namespace utils
{
/**
 * Map input to arglist.
 * The arguments are expected to be comma separated.
 *
 * NOTE: Map is used instead of unordered_map because the expected number of
 *       arguments is low.
 */
inline auto map_raw_args_string_to_names(const std::vector<std::string>& names,
                                         const std::string&              raw_arglist)
    -> std::optional<std::map<std::string, std::string>>
{
    using TokenType      = MactenAllToken;
    using AllTokenStream = macten::TokenStream<MactenAllToken>;

    const auto ts   = AllTokenStream::from_string(raw_arglist);
    auto       view = ts.get_view();

    std::map<std::string, std::string> mapping{};

    const std::size_t expected_argcount = names.size();

    uint8_t argcount{0}, brace_scope{0}, square_scope{0}, paren_scope{0};

    AllTokenStream ts_buffer{};

    while (!view.is_at_end())
    {
        const auto token = view.pop();

        switch (token.type)
        {
#define SCOPE_CASE(Name, name)                                                                     \
    break; case _JOIN(TokenType::L, Name):                                                         \
    {                                                                                              \
        _JOIN(name, _scope++);                                                                     \
        ts_buffer.push_back(token);                                                                \
    }                                                                                              \
    break; case _JOIN(TokenType::R, Name):                                                         \
    {                                                                                              \
        _JOIN(name, _scope)--;                                                                     \
        ts_buffer.push_back(token);                                                                \
    }
            SCOPE_CASE(Paren, paren)
            SCOPE_CASE(Square, square)
            SCOPE_CASE(Brace, brace)
#undef SCOPE_CASE
            break; case TokenType::Comma:
            {
                if (brace_scope == 0 && square_scope == 0 && paren_scope == 0)
                {
                    if (argcount < expected_argcount)
                    {
                        mapping[names[argcount]] = ts_buffer.construct();
                        argcount++;
                    }
                    ts_buffer.clear();
                }
                else
                {
                    ts_buffer.push_back(token);
                }
            }
            break; default:
            {
                ts_buffer.push_back(token);
            }
        }
    }

    // Handle last argument. This also works for argless.
    if (argcount < expected_argcount)
    {
        mapping[names[argcount]] = ts_buffer.construct();
        argcount++;
    }

    // Arity error.
    if (argcount != expected_argcount)
        return {};

    return mapping;
}

/**
 * Checks whether the upcoming sequence in view matches a macro call: `<ident>![`.
 */
inline auto is_macro_call(typename macten::TokenStream<MactenAllToken>::TokenStreamView& view)
    -> bool
{
    return view.match_sequence(MactenAllToken::Identifier, MactenAllToken::Exclamation,
                               MactenAllToken::LSquare);
}
} // namespace utils
} // namespace macten

#undef _JOIN

#endif /* MACTEN_UTILS_H */
