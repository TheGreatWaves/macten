#ifndef MACTEN_PASER_HPP
#define MACTEN_PASER_HPP

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

#include "macten_all_tokens.hpp"
#include "macten_tokens.hpp"
#include "prod_macro_def.hpp"
#include "prod_macro_writer.hpp"
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

    [[nodiscard]] auto generate_procedural() noexcept -> bool
    {
        advance();

        bool has_procedural {false};

        while (!check(Token::EndOfFile))
        {
            if (match(Token::ProceduralDefinition))
            {
                if (!has_procedural)
                {
                    std::filesystem::create_directory(".macten");

                    if (!std::filesystem::exists(".macten/macten.py"))
                        generate_parser_utils();
                }
                has_procedural = true;

                procedural_definition(true);
            }
            else
                advance();
        }

        if (has_procedural)
        {
            generate_driver();
        }

        return this->has_error;
    }

    auto generate_driver() -> void
    {
        std::vector<std::pair<std::string, std::string>> macro_files;
        std::transform(m_prod_macros.begin(), 
                       m_prod_macros.end(), 
                       std::back_inserter(macro_files), 
                       [](const std::string& macro){ return std::make_pair(macro+"_parser", macro+"_handler"); });
        macten::CodeEmitter emitter{};
        emitter.comment("AUTO GENERATED CODE, DO NOT EDIT");
        emitter.section("Imports");
        emitter.writeln("import macten");
        emitter.writeln("import sys");
        emitter.writeln("from pathlib import Path");
        emitter.writeln("from contextlib import redirect_stdout");
        for (const auto& [parser, handler] : macro_files)
        {
            emitter.writeln("import " + parser);
            emitter.writeln("import " + handler);
        }
        emitter.section("Setup");
        emitter.writeln("macten.init()");
        for (const auto& [parser, handler] : macro_files)
        {
            emitter.writeln(parser+".add_rules()");
            emitter.writeln(handler+".add_handler()");
        }
        emitter.section("Execution");
        emitter.writeln("rule=sys.argv[1]");
        emitter.writeln("file=sys.argv[2]");
        emitter.writeln("source=Path(file).read_text()");
        emitter.writeln("input=macten.ListStream.from_string(source)");
        emitter.writeln("ast=None");
        const auto scope = emitter.begin_indent("with open('.macten/tmp.in.out','w') as f:");
        {
            const auto scope = emitter.begin_indent("with redirect_stdout(f):");
            {
                const auto scope = emitter.begin_indent("while input and not input.empty():");
                emitter.writeln("input,ast=macten.ctx.get_rule(rule).parse(input,ast)");
                {
                    const auto scope = emitter.begin_indent("if ast is None:");
                    emitter.writeln("print(f\"Failed to parse '{file}' using '{rule}' parser rules\")");
                    emitter.writeln("break");
                }
                emitter.writeln("macten.handler.get(rule)(ast)");
            }
        }
        std::ofstream driver_file{};
        driver_file.open(".macten/driver.py");
        driver_file << emitter.dump();
        driver_file.close();
    }

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

    auto procedural_definition(bool build = false) -> void
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

        if (build)
        {
            build_procedural_macro_files(profile);
        }
    }

    auto build_procedural_macro_files(const macten::ProceduralMacroProfile& profile) -> void
    {
        const auto& macro_name = profile.name;

        std::ofstream parser_file;
        parser_file.open(".macten/"+macro_name+"_parser.py");
        parser_file << profile.dump();
        parser_file.close();

        // Don't create a new handler if there is an existing one.
        const auto handler_file_path = ".macten/"+macro_name+"_handler.py";
        if (!std::filesystem::exists(handler_file_path))
        {
            macten::CodeEmitter emitter{};

            emitter.comment("USER IMPLEMENTATION - " + macro_name + " HANDLER");
            emitter.section("Imports");
            emitter.writeln("import macten");
            emitter.section("Register Handler");
            {
                const auto scope = emitter.begin_indent("def add_handler():");
                emitter.writeln("macten.handler.add(\"" + macro_name + "\", handle)");
            }
            emitter.section("Handler Function");
            {
                const auto scope = emitter.begin_indent("def handle(ast):");
                emitter.comment("TODO: Implementation of \"" + macro_name + "\" handler");
                emitter.writeln("macten.NodeUtils.print(ast)");
            }

            std::ofstream handler_file{};
            handler_file.open(handler_file_path);
            handler_file << emitter.dump();
            handler_file.close();
        }
    }

    auto generate_parser_utils() -> void
    {
        std::filesystem::copy("prod_macro_utils.py", ".macten/macten.py");
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
            const auto macro_name = consume_identifier("Expected macro name");
            m_prod_macros.push_back(macro_name);
            consume(Token::LBrace, "Expected '{' after procedural macro name");
            const auto _ = scanner.scan_body(MactenToken::LBrace, MactenToken::RBrace);
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
