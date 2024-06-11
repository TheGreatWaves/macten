#ifndef MACTEN_DECLARATIVE_PARAMETER_HPP
#define MACTEN_DECLARATIVE_PARAMETER_HPP

#include <map>
#include <optional>

#include "token_stream.hpp"

namespace macten
{

struct DeclarativeMacroParameter
{
  private:
    using Token    = MactenToken;
    using AllToken = MactenAllToken;

  public:
    enum class PatternMode
    {
        Empty,
        Normal,
        Plus,
        Asterisk,
    };

    /**
     * Default Constructor. Retursn an empty parameter list.
     */
    DeclarativeMacroParameter() = default;

    /**
     * Returns a parameter object with corresponds to the parameter input in the form of a stream
     * view.
     */
    DeclarativeMacroParameter(TS::View parameter_view)
        : pattern_mode(PatternMode::Normal)
        , pattern{}
        , variadic_pattern{}
    {
        if (parameter_view.is_at_end())
        {
            pattern_mode = PatternMode::Empty;
            return;
        }

        parse(parameter_view);
    }

    /**
     * Parse input and populate pattern/arg names from view.
     */
    auto parse(TS::View& parameter_view) -> void
    {
        while (!parameter_view.is_at_end())
        {
            const auto tok = parameter_view.pop();

            switch (tok.type)
            {
                break;
                case Token::Dollar:
                {
                    parse_arg_symbols(tok, parameter_view);
                }
                break;
                default:
                {
                    pattern.push_back(tok);
                }
            }
        }
    }

    /**
     * Parse symbols from view..
     */
    auto parse_arg_symbols(const TS::Token& tok, TS::View& parameter_view) -> void
    {
        switch (parameter_view.peek().type)
        {
            break;
            case Token::Identifier:
            {
                argument_names.push_back(parameter_view.pop().lexeme);
                pattern.push_back(tok);
            }
            break;
            case Token::LParen:
            {
                set_pattern_mode(PatternMode::Asterisk);

                parameter_view.advance();
                auto pview = parameter_view.between(Token::LParen, Token::RParen);

                parameter_view.advance(pview.remaining_size() + 1);

                while (!pview.is_at_end())
                {
                    const auto front_token = pview.pop();
                    variadic_pattern.push_back(front_token);

                    if (front_token.is(Token::Dollar))
                    {
                        // Get the variadic container name.
                        variadic_container_name = pview.pop().lexeme;
                    }
                }
            }
            break;
            default:
            {
                std::cerr << "Expected variable name after '$' symbol." << '\n';
            }
        }
    }

    /**
     * Set pattern mode.
     */
    auto set_pattern_mode(const PatternMode mode) noexcept -> void
    {
        this->pattern_mode = mode;
    }

    /**
     * Check pattern mode.
     */
    auto is_pattern_mode(const PatternMode mode) const noexcept -> bool
    {
        return this->pattern_mode == mode;
    }

    /**
     * Checks whether the input arguments matches the variadic template.
     */
    [[nodiscard]] auto match_variadic(TS::View input) const noexcept -> bool
    {
        if (variadic_pattern.empty() || input.is_exhausted())
        {
            return false;
        }

        while (!input.is_exhausted())
        {
            for (const auto& current_expected_token : variadic_pattern)
            {
                if (current_expected_token.is(Token::Dollar))
                {
                    if (input.pop().is(Token::LParen))
                    {
                        const auto body = input.between(Token::LParen, Token::RParen);
                        input.advance(body.remaining_size() + 1);
                    }
                    continue;
                }

                const auto token              = input.pop();
                const auto current_token_type = token.type;
                if (current_token_type != current_expected_token.type)
                {
                    return false;
                }

                // Lexeme (keyword) mismatch.
                if (current_expected_token.any_of(Token::Identifier, Token::Number) &&
                    !token.lexically_eq(current_expected_token))
                {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     *
     */
    [[nodiscard]] auto is_parameterless(TS::View input) const noexcept -> bool
    {
        return (input.peek().is(Token::EndOfFile) && pattern_mode == PatternMode::Empty);
    }

    /**
     * Checks whether the input token stream matches the pattern of the expected parameters or not.
     */
    [[nodiscard]] auto match(TS::View input) const noexcept -> bool
    {
        if (is_parameterless(input))
            return true;

        for (const auto& current_expected_token : pattern)
        {
            if (current_expected_token.type == Token::Dollar)
            {
                if (input.peek().is(Token::LParen))
                {
                    input.advance();
                    const auto body = input.between(Token::LParen, Token::RParen);
                    input.advance(body.remaining_size() + 1);
                }
                else
                {
                    input.advance();
                }
                continue;
            }

            const auto token              = input.pop();
            const auto current_token_type = token.type;

            if (current_token_type != current_expected_token.type)
                return false;

            // Lexeme (keyword) mismatch.
            if (current_expected_token.any_of(Token::Identifier, Token::Number) &&
                !token.lexically_eq(current_expected_token))
            {
                return false;
            }
        }

        if (is_pattern_mode(PatternMode::Normal))
        {
            // Valid if input is exhausted. There shouldn't be more to match.
            return input.is_exhausted();
        }

        return match_variadic(input);
    }

    /**
     * Match pattern with the input and retrieve argument names and their value.
     */
    [[nodiscard]] auto map_args(ATS::View& input) const noexcept
        -> std::optional<std::map<std::string, std::string>>
    {
        std::map<std::string, std::string> argmap{};
        std::size_t                        argcount{0};

        for (const auto& expected : pattern)
        {
            const auto& expected_tty = expected.type;
            input.skip(AllToken::Newline, AllToken::Tab, AllToken::Space);
            const auto token = input.pop();

            if (expected_tty == Token::Dollar)
            {
                // Arity error.
                if ((argcount + 1) > argument_names.size())
                {
                    std::cerr << "Arity error, declared parameter count mismatched.\n";
                    return {};
                }

                std::string        argval{};
                const std::string& argname = argument_names[argcount];

                // Handle grouping.
                if (token.is(AllToken::LParen))
                {
                    const auto expr = input.between(AllToken::LParen, AllToken::RParen);
                    input.advance(expr.remaining_size() + 1);
                    argval = expr.construct();
                }
                else
                {
                    argval = token.lexeme;
                }

                argmap[argname] = argval;

                argcount++;
            }
            else if (token.type.name() != expected_tty.name())
            {
                return {};
            }
        }

        if (pattern_mode != PatternMode::Normal)
        {
            argmap[variadic_container_name] = input.construct();

            while (!input.is_at_end())
                input.advance();
        }

        return argmap;
    }

    PatternMode                             pattern_mode{};
    std::vector<cpp20scanner::Token<Token>> pattern{};
    std::vector<std::string>                argument_names{};
    std::string                             variadic_container_name{};
    std::vector<cpp20scanner::Token<Token>> variadic_pattern{};
};

} // namespace macten

#endif /* MACTEN_DECLARATIVE_PARAMETER_HPP */
