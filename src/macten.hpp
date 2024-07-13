#ifndef MACTEN_HPP
#define MACTEN_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>

#include "macten_all_tokens.hpp"
#include "macten_tokens.hpp"
#include "prod_macro_def.hpp"
#include "prod_macro_writer.hpp"
#include "token_stream.hpp"
#include "utils.hpp"

#include "declarative_parameter.hpp"

namespace macten {


class MactenWriter;

/**
 * Declarative Macro.
 *
 * This template structure contains all of the vital information
 * needed in order to perform macro substitution.
 */
struct DeclarativeTemplate
{
  private:
    using Token = cpp20scanner::Token<MactenAllToken>;

  public:
    // Default Ctor.
    DeclarativeTemplate() = default;

    // Ctor.
    //
    // Constructs and returns a new declartive template with the given name and body.
    explicit DeclarativeTemplate(const std::string& name, const std::vector<std::string>& body,
                                 const std::vector<DeclarativeMacroParameter>& parameters)
        : m_name(name)
        , m_params(parameters)
    {
        for (const auto& s : body)
        {
            m_token_stream.push_back(macten::TokenStream<MactenAllToken>::from_string(s));
        }
    }

    // Apply declarative template macro, the expansion is written to a target token stream.
    [[nodiscard]] auto apply(macten::MactenWriter* env, const int index,
                             macten::TokenStream<MactenAllToken>& target,
                             std::map<std::string, std::string>   args = {}) const -> bool;

    /**
     * Returns an argument to argument name match. If the argument match fails, none is returned.
     */
    [[nodiscard]] auto map_args(
        std::size_t                                           index,
        macten::TokenStream<MactenAllToken>::TokenStreamView& arg_all_tokens_view) const noexcept
        -> std::optional<std::map<std::string, std::string>>
    {
        const auto& param = m_params.at(index);
        return param.map_args(arg_all_tokens_view);
    }

    /**
     * Check arity. Returns true if the arguments supplied count is the same as parameter count.
     */
    [[nodiscard]] auto check_arity(const std::map<std::string, std::string> args,
                                   const DeclarativeMacroParameter&         param) -> bool
    {
        return (param.pattern_mode == DeclarativeMacroParameter::PatternMode::Normal &&
                args.size() != param.argument_names.size());
    }

    /**
     * Try matching input against all patterns inside the environment and returns the corresponding
     * macro index.
     */
    [[nodiscard]] auto match(macten::TokenStream<MactenToken>::TokenStreamView view) const noexcept
        -> int
    {
        auto size = static_cast<int>(m_params.size());
        for (int index{0}; index < size; index++)
        {
            const auto& param = m_params[index];
            if (param.match(view))
            {
                return index;
            }
        }
        return -1;
    }

    /**
     * Members.
     */
    std::string                                      m_name;
    std::vector<DeclarativeMacroParameter>           m_params;
    std::vector<macten::TokenStream<MactenAllToken>> m_token_stream;
};

struct DeclarativeMacroDetail
{
    auto construct_template() const -> DeclarativeTemplate
    {
        return DeclarativeTemplate(m_name, m_body, m_params);
    }

    /**
     * Members;
     */
    std::string                            m_name;
    std::vector<std::string>               m_body;
    std::vector<DeclarativeMacroParameter> m_params;
};


/**
 * Parser.
 */
#include "parser.hpp"

// TODO: This class needs to be refactored.
// WARN: Do it before it becomes too big.
class MactenWriter
{
  private:
    using DeclarativeMacroRules = std::unordered_map<std::string, DeclarativeTemplate>;
    using ProceduralMacroRules  = std::unordered_set<std::string>;
    using TType                 = MactenAllToken;
    inline static const std::map<std::string, std::string> EmptyArgList{};

  public:
    explicit MactenWriter(std::string_view path, std::string_view output_name)
        : m_source_path{path}
        , m_output_name{output_name}
    {
    }

    /**
     * Generate declarative macro rules from source.
     */
    auto generate_declarative_rules() -> bool
    {
        MactenParser parser(m_source_path);
        const auto             res = parser.parse();
        if (res)
        {
            std::for_each(parser.m_macros.begin(), parser.m_macros.end(),
                          [this](const auto& macro_detail) {
                              m_declarative_macro_rules[macro_detail.m_name] =
                                  macro_detail.construct_template();
                          });
            std::for_each(parser.m_prod_macros.begin(), parser.m_prod_macros.end(),
                          [this](const auto& prod_macro_name) {
                              m_procedural_macro_rules.insert(prod_macro_name);
                          });
        }
        return res;
    }

    /**
     * Checks wheter the macro with the given name exists as a declarative macro.
     */
    auto has_declarative_macro(const std::string& name) -> bool
    {
        return m_declarative_macro_rules.contains(name);
    }


    /**
     * Checks wheter the macro with the given name exists as a procedural macro.
     */
    auto has_procedural_macro(const std::string& name) -> bool
    {
        return m_procedural_macro_rules.contains(name);
    }

    /**
     * Apply macro rules.
     */
    auto apply_macro_rules(macten::TokenStream<MactenAllToken>&                  target,
                           macten::TokenStream<MactenAllToken>::TokenStreamView& source_view)
        -> bool

    {
        std::vector<TType> prefix_buffer{};

        while (!source_view.peek().is(MactenAllToken::EndOfFile))
        {
            auto token = source_view.peek();

            while (source_view.match_sequence(TType::Identifier, TType::Underscore))
            {
                if (source_view.peek(2).is(TType::Identifier))
                {
                    token.lexeme += "_" + source_view.peek(2).lexeme;
                    source_view.advance(2);
                }
                else
                {
                    while (source_view.peek(1).is(TType::Underscore))
                    {
                        token.lexeme += "_";
                        source_view.advance(1);
                    }
                }
            }

            const bool macro_call_found = macten::utils::is_macro_call(source_view);

            if (macro_call_found)
            {
                std::stringstream indent_ss{};
                for (const auto& prefix : prefix_buffer)
                    indent_ss << prefix.get_symbol();
                const std::string indent = indent_ss.str();

                if (has_declarative_macro(token.lexeme)) 
                {
                    // Move onto the '['.
                    source_view.skip_until(TType::LSquare);
                    if (!source_view.consume(TType::LSquare))
                        return false;

                    const auto args = source_view.between(MactenAllToken::LSquare, MactenAllToken::RSquare);
                    source_view.advance(args.remaining_size());

                    if (!match_and_execute_macro(target, token.lexeme, args.construct()))
                    {
                        return false;
                    }
                }
                else if (has_procedural_macro(token.lexeme))
                {
                    if (!handle_procedural_macro_call(target, token.lexeme, source_view, indent)) 
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (token.type == TType::Newline)
                    prefix_buffer.clear();
                else
                    prefix_buffer.push_back(token.type);

                // Default. Just push the token, no need to care.
                target.push_back(token);
            }

            source_view.advance();
        }


        return true;
    }

    auto handle_procedural_macro_call(
                                      macten::TokenStream<MactenAllToken>& target,
                                      const std::string& macro_name, 
                                      macten::TokenStream<MactenAllToken>::TokenStreamView& source_view,
                                      const std::string& indent) -> bool
    {
        // Capture argument body [arg].
        source_view.skip_until(TType::LSquare);
        if (!source_view.consume(TType::LSquare)) return false;
        const auto args = source_view.between(MactenAllToken::LSquare, MactenAllToken::RSquare);
        source_view.advance(args.remaining_size());

        // Dump the arguments into a file.
        std::ofstream tmp_file{};
        tmp_file.open(".macten/tmp.in");
        tmp_file << args.construct();
        tmp_file.close();

        // Set up fork & exec.
        int pid, status;
        if (pid = fork(); pid != 0)
        {
            // Wait for process to finish running.
            waitpid(pid, &status, 0);
        }
        else
        {
            // Child process
           const char executable[] = "/home/linuxbrew/.linuxbrew/bin/python3";
           execl(executable, executable, ".macten/driver.py", macro_name.c_str(), ".macten/tmp.in",  NULL);
        }

        const auto result_stream = macten::TokenStream<MactenAllToken>::from_file_raw(".macten/tmp.in.out");
        auto view = result_stream.get_view();

        while (!view.is_at_end())
        {
            const auto token = view.pop();
            target.push_back(token);
            if (token.lexeme.ends_with('\n'))
            {
                target.add_string(indent);
            }
        }

        return true;
    }

    auto match_and_execute_macro(macten::TokenStream<MactenAllToken>& target,
                                 const std::string& macro_name, const std::string& args) -> bool
    {
        const DeclarativeTemplate macro_rule{this->m_declarative_macro_rules.at(macro_name)};

        const auto all_token_stream      = macten::TokenStream<MactenAllToken>::from_string(args);
        auto       all_token_stream_view = all_token_stream.get_view();

        do
        {
            const auto token_stream = macten::TokenStream<MactenToken>::from_string(all_token_stream_view.construct());
            auto token_stream_view = token_stream.get_view();

            // Find match.
            const int idx = macro_rule.match(token_stream_view);
            if (idx == -1)
            {
                return false;
            }

            const auto args_mapping = macro_rule.map_args(idx, all_token_stream_view);

            if (!args_mapping.has_value())
            {
                std::cerr << "Failed to create argument mapping for macro '" << macro_name << "'\n";
                return false;
            }

            const bool success = macro_rule.apply(this, idx, target, args_mapping.value());
            if (!success)
            {
                std::cerr << "Failed to apply macro: '" << macro_name << "'\n";
                return false;
            }

            while (all_token_stream_view.peek().any_of(MactenAllToken::Newline))
            {
                target.push_back(all_token_stream_view.pop());
            }
            all_token_stream_view.skip(MactenAllToken::Space, MactenAllToken::Newline, MactenAllToken::Tab);
        } while ((!all_token_stream_view.is_at_end()));
        return true;
    }

    /**
     * Skip macro definition. This removes the macten definitions from the source code.
     */
    auto skip_macro_definition(macten::TokenStream<MactenAllToken>::TokenStreamView& view) -> void
    {
        using TokenType = MactenAllToken;
        view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline, TokenType::Identifier);

        int16_t brace_scope{1};

        if (view.consume(TokenType::LBrace))
        {
            while (!view.is_at_end() && brace_scope > 0)
            {
                switch (view.peek().type)
                {
                    break;
                    case TokenType::LBrace:
                        ++brace_scope;
                        break;
                    case TokenType::RBrace:
                        --brace_scope;
                        break;
                    default:
                    {
                    }
                }
                view.advance();
            }

            view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);
        }
    }

    /**
     * Tidy macro call site. This allows for convenient assumptions during the expansion phase.
     */
    auto tidy_macro_call_site(macten::TokenStream<MactenAllToken>::TokenStreamView& view,
                              macten::TokenStream<MactenAllToken>&                  target) -> void
    {
        using TokenType = MactenAllToken;
        view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);

        int16_t brace_scope{1};

        while (!view.is_at_end() && brace_scope > 0)
        {
            const auto token = view.pop();

            if (token.is(TokenType::Space))
            {
                while (view.peek().is(TokenType::Space))
                {
                    view.advance();
                }

                if (!view.peek().is(TokenType::Comma))
                {
                    target.push_back(token);
                }
                continue;
            }
            else if (token.is(TokenType::Comma))
            {
                view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);
                target.push_back(token);
                continue;
            }
            else if (token.is(TokenType::LSquare))
            {
                brace_scope++;
            }
            else if (token.is(TokenType::RSquare))
            {
                brace_scope--;
            }
            target.push_back(token);
        }
    }

    /**
     * Remove macro definitions from the final generated code.
     */
    auto preprocess(macten::TokenStream<MactenAllToken>& source)
        -> macten::TokenStream<MactenAllToken>
    {
        using TokenType = MactenAllToken;

        macten::TokenStream<MactenAllToken> processed_tokens{};
        auto                                source_view = source.get_view();

        while (!source_view.is_at_end())
        {
            const auto token = source_view.pop();

            if (token.any_of(TokenType::ProceduralDefinition, TokenType::DeclarativeDefinition))
            {
                skip_macro_definition(source_view);
                continue;
            }
            else if (token.is(TokenType::Identifier) &&
                     m_declarative_macro_rules.contains(token.lexeme) &&
                     source_view.match_sequence(TokenType::Exclamation, TokenType::LSquare))
            {
                processed_tokens.push_back(token);
                processed_tokens.push_back(source_view.peek(0));
                processed_tokens.push_back(source_view.peek(1));
                source_view.advance(2);
                tidy_macro_call_site(source_view, processed_tokens);
                continue;
            }

            processed_tokens.push_back(token);
        }

        return processed_tokens;
    }

    [[nodiscard]] auto generate() -> bool
    {
        MactenParser parser(m_source_path);
        return parser.generate_procedural();
    }

    /**
     * Tokenize. Substitute. Rebuild.
     * NOTE: At the moment, this requires two passes. This can be addressed later.
     */
    auto process() -> bool
    {
        // Generate parse rules.
        // This is the first pass.
        generate_declarative_rules();

        // Tokenize the file.
        macten::TokenStream<MactenAllToken> result_tokens;
        auto source_tokens = macten::TokenStream<MactenAllToken>::from_file(m_source_path);

        // Preprocess and remove definitions.
        source_tokens = preprocess(source_tokens);

        // Note: It's important that we get the view AFTER preprocess.
        auto       source_tokens_view = source_tokens.get_view();
        const auto res                = apply_macro_rules(result_tokens, source_tokens_view);

        // Write to output file
        std::ofstream output_file;
        output_file.open(m_output_name);
        output_file << result_tokens.construct();
        output_file.close();

        return res;
    }

    /**
     * Match each token with appropriate rules.
     */
    auto apply_rules() -> bool
    {
        return true;
    }

  private:
    const std::string     m_source_path;
    const std::string     m_output_name;
    DeclarativeMacroRules m_declarative_macro_rules;
    ProceduralMacroRules  m_procedural_macro_rules;
};

} // namespace macten

#endif /* MACTEN_H */
