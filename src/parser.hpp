#ifndef MACTEN_PASER_HPP
#define MACTEN_PASER_HPP

#include <sstream>

#include "macten_all_tokens.hpp"
#include "macten_tokens.hpp"
#include "token_stream.hpp"

class MactenParser final : public cpp20scanner::BaseParser<MactenTokenScanner, MactenToken>
{
  private:
    using Token          = MactenToken;
    using AllToken       = MactenAllToken;
    using TokenStream    = macten::TokenStream<Token>;
    using AllTokenStream = macten::TokenStream<MactenAllToken>;

  public:
    [[nodiscard]] explicit MactenParser(const std::string& file_path)
        : cpp20scanner::BaseParser<MactenTokenScanner, MactenToken>(file_path)
        , emitter()
    {
    }

    /**
     * Default Ctor prohibted.
     */
    constexpr MactenParser() = delete;

    /**
     * Main Macten parse loop.
     */
    [[nodiscard]] auto parse() noexcept -> bool
    {
        advance();

        while (!match(Token::EndOfFile))
        {
            declaration();
        }

        return !this->has_error;
    }

  private:
    /**
     * Keep consuming while the current token is a tab.
     */
    auto skip_whitespace() noexcept -> void
    {
        while (check(Token::Tab))
        {
            advance();
        }
    }

    /**
     * Parse the current argument and populate macro tokens pattern and argument names.
     *
     * For example, if we parse `$hello`, we will have the token pattern of {<dollar><identifer>}.
     * If the input does not contain a parameter ($), then we only populate the token pattern.
     */
    auto parse_arg(std::vector<Token>& macro_tokens, std::vector<std::string>& macro_args) -> void 
    {
        if (match(Token::Dollar))
        {
            macro_tokens.push_back(previous.type);

            const auto arg_name = consume_identifier("Expected argument name");
            macro_args.push_back(arg_name);
        }
        else
        {
            advance();
        }

        macro_tokens.push_back(previous.type);
    }

    /**
     * Return a vector of tokens expected by the argument list that
     * is currently being parsed.
     *
     * // Note: Head token should be LParen '('.
     */
    auto parse_args() noexcept -> std::pair<std::vector<std::string>, std::vector<Token>>
    {
        std::vector<Token>       macro_tokens{};
        std::vector<std::string> macro_args{};

        // Move into the body of the '('.
        int scope{1};

        while (!this->scanner.is_at_end())
        {
            if (current.is(Token::LParen))
                scope++;
            else if (current.is(Token::RParen))
            {
                scope--;

                if (scope <= 0)
                {
                    advance();
                    break;
                }
            }

            parse_arg(macro_tokens, macro_args);
        }

        return {std::move(macro_args), std::move(macro_tokens)};
    }

    /**
     * Parse identifier.
     */
    auto consume_identifier(std::string_view message) -> std::string
    {
        std::stringstream ss {};
        consume(Token::Identifier, message);
        return previous.lexeme;
    }

    /**
     * Parse declarative macros.
     */
    auto declarative_definition() -> void
    {
        const auto macro_name = consume_identifier("Expected macro name");
        consume(Token::LBrace, "Expected macro body, missing '{'");

        std::vector<std::string>               branch_bodies;
        std::vector<DeclarativeMacroParameter> branch_parameters;

        while (!match(Token::RBrace))
        {
            const auto parameter_signature = this->scanner.scan_body(Token::LParen, Token::RParen);
            const auto parameter_signature_stream = TokenStream::from_string(parameter_signature.lexeme);
            const auto parameter_signature_view = parameter_signature_stream.get_view();
            advance();

            branch_parameters.emplace_back(parameter_signature_view);

            consume(Token::RParen, "Expected arguments, missing ')'");

            std::vector<std::string> macro_args{};
            std::vector<Token>       macro_tokens{};

            consume(Token::Equal, "Expected '='");
            consume(Token::GreaterThan, "Expected '>'");

            if (check(Token::LBrace))
            {
                scanner.skip_whitespace();
                const auto macro_body_token = this->scanner.scan_body(Token::LBrace, Token::RBrace);

                const auto token_stream      = AllTokenStream::from_string(macro_body_token.lexeme);
                auto       token_stream_view = token_stream.get_view();

                AllTokenStream token_stream_result;

                while (!token_stream_view.is_at_end())
                {
                    const auto token = token_stream_view.pop();

                    if (token.is(AllToken::Newline))
                    {
                        // Optional skip twice to take into account the indentation of the macro
                        // body. WARN: At the moment, this would break if the prefix spacing is
                        // inconsistent. WARN: If you are using an editor which converts tabs into
                        // multiple spaces,
                        //       the result of the macro body will be incorrect.
                        // TODO: Work out a better scheme and fix this. One possibility is to book
                        // mark the
                        //       indentation characters used before the first line of the body and
                        //       of the macro. this simply skip the book marked amount when scanning
                        //       subsequent lines. This solution is not exactly idea either because
                        //       there might be cases where the user might want to write a simple
                        //       single line macro. Perhaps a logging system should be implemented
                        //       and emit a warning. This would work great because currently the
                        //       project is lacking logging and error reporting capabilities.
                        static_cast<void>(
                            token_stream_view.consume(AllToken::Tab, AllToken::Space));
                        static_cast<void>(
                            token_stream_view.consume(AllToken::Tab, AllToken::Space));
                    }

                    token_stream_result.push_back(token);
                }

                if (token_stream_result.peek_back().is(AllToken::Newline))
                {
                    token_stream_result.pop_back();
                }

                branch_bodies.emplace_back(token_stream_result.construct());

                advance();
                advance();
            }
        }

        m_macros.emplace_back(std::move(macro_name), std::move(branch_bodies),
                              std::move(branch_parameters));
    }

    auto procedural_definition() -> void
    {
        // New procedural macro profile.
        macten::ProceduralMacroProfile profile{};

        // Retrieve macro name.
        const auto macro_name = consume_identifier("Expected macro name");
        profile.set_name(macro_name);
        m_prod_macros.push_back(macro_name);

        // Start parsing procedural macro body.
        consume(Token::LBrace, "Expected macro body, missing '{'");

        while (!match(Token::EndOfFile) && !match(Token::RBrace))
        {
            // Retrieve macro name.
            const auto rule_label = consume_identifier("Expected rule label of type identifier");

            auto& rule        = profile.create_rule(rule_label);
            profile.last_rule = rule_label;

            // Parse proc macro rules.
            do
            {
                consume(Token::LBrace, "Expected '{' after rule label name");

                int8_t                   scope{1};
                std::vector<std::string> entry{};
                while (!match(Token::EndOfFile))
                {
                    switch (current.type)
                    {
                        break;
                        case Token::RBrace:
                        {
                            scope--;
                        }
                        break;
                        case Token::LBrace:
                        {
                            scope++;
                        }
                        break;
                        default:
                        {
                            // do nothing...
                        }
                    }
                    advance();

                    // Break out if out of body scope.
                    if (scope == 0)
                        break;

                    // Add the rule
                    const auto rule_value_token = previous;
                    entry.push_back(rule_value_token.lexeme);

                    if (rule_value_token.lexeme == rule_label)
                        rule.second = true;
                }
                rule.first.push_back(entry);
            } while (match(Token::Pipe));
        }

        profile.dump();
    }
    /**
     * Parse declartions.
     */
    auto declaration() noexcept -> void
    {
        if (match(Token::DeclarativeDefinition))
        {
            declarative_definition();
        }
        else if (match(Token::ProceduralDefinition))
        {
            procedural_definition();
        }
        else
        {
            advance(true);
        }
    }

  public:
    std::vector<DeclarativeMacroDetail> m_macros;
    std::vector<std::string>            m_prod_macros;
    macten::CodeEmitter                 emitter;
};

#endif /* MACTEN_PASER_HPP */
