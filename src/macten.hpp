#pragma once
#ifndef MACTEN_H
#define MACTEN_H

#define DEBUG

#include <algorithm>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "token_stream.hpp"
#include "utils.hpp"
#include "macten_tokens.hpp"
#include "macten_all_tokens.hpp"


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
 explicit DeclarativeTemplate(std::string name, const std::string& body, std::vector<std::string> args = {})
 : m_name(name)
 , m_arguments(args)
 , m_token_stream(macten::TokenStream<MactenAllToken>::from_string(body))
 {
 }


 [[nodiscard]] auto apply(
     MactenWriter* env,
     const std::string& indentation,
     macten::TokenStream<MactenAllToken>& target, 
     std::map<std::string, std::string> args = {}
  ) const -> bool;

 [[nodiscard]] auto map_args(const std::string& arg_body) const -> std::optional<std::map<std::string, std::string>>
 {
  return macten::utils::map_raw_args_string_to_names(this->m_arguments, arg_body);
 }

 /**
  * Members.
  */
 std::string                     m_name;
 std::vector<std::string>        m_arguments;
 macten::TokenStream<MactenAllToken> m_token_stream;
};

struct DeclarativeMacroDetail
{
 auto construct_template() const -> DeclarativeTemplate
 {
  return DeclarativeTemplate(m_name, m_body, m_args);
 }

 /**
  * Members;
  */
 std::string              m_name;
 std::string              m_body;
 std::vector<std::string> m_args;
};


class DeclarativeMacroParser : public cpp20scanner::BaseParser<MactenTokenScanner, MactenToken>
{
 private:
  using Token = MactenToken;
 public:
   [[nodiscard]] explicit DeclarativeMacroParser(const std::string& file_path)
       : cpp20scanner::BaseParser<MactenTokenScanner, MactenToken>(file_path)
   {
   }

   /**
    * Default Ctor prohibted.
    */
   constexpr DeclarativeMacroParser() = delete;

   [[nodiscard]] auto parse() noexcept -> bool
   {
    advance();

    while (!match(MactenToken::EndOfFile))
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
   * Parse declartions.
   */
  auto declaration() noexcept -> void 
  {
   if (match(Token::DeclarativeDefinition))
   {
    consume(MactenToken::Identifier, "Expected macro name, found: " + previous.lexeme + ".");
    const auto macro_name = previous.lexeme;

    consume(Token::LBrace, "Expected macro body, missing '{', found: '" + previous.lexeme + "'.");

    // Parse arguments.
    consume(Token::LParen, "Expected arguments, missing '(', found: '" + previous.lexeme + "'.");


    std::vector<std::string> macro_args;
    while(!match(Token::RParen))
    {
     // Retrieve the first argument.
     consume(Token::Identifier, "Expected argument name.");
     const auto arg = previous.lexeme;
     macro_args.push_back(arg);

     while (match(Token::Comma))
     {
      consume(Token::Identifier, "Expected argument name.");
      const auto arg = previous.lexeme;
      macro_args.push_back(arg);
     }
    }

    consume(Token::Equal, "Expected '='");
    consume(Token::GreaterThan, "Expected '>'");

    if (check(Token::LBrace))
    {
     scanner.skip_whitespace();
     auto macro_body_token = this->scanner.scan_body(Token::LBrace, Token::RBrace);

     auto token_stream = macten::TokenStream<MactenAllToken>::from_string(macro_body_token.lexeme);
     auto token_stream_view = token_stream.get_view();

     macten::TokenStream<MactenAllToken> token_stream_result;

     while (!token_stream_view.is_at_end())
     {
      const auto token = token_stream_view.pop();

      if (token.is(MactenAllToken::Newline))
      {
        // Optional skip twice to take into account the indentation of the macro body.
        // WARN: At the moment, this would break if the prefix spacing is inconsistent.
        // WARN: If you are using an editor which converts tabs into multiple spaces,
        //       the result of the macro body will be incorrect.
        // TODO: Work out a better scheme and fix this. One possibility is to book mark the 
        //       indentation characters used before the first line of the body and of the macro.
        //       this simply skip the book marked amount when scanning subsequent lines. This solution
        //       is not exactly idea either because there might be cases where the user might want to
        //       write a simple single line macro. Perhaps a logging system should be implemented and
        //       emit a warning. This would work great because currently the project is lacking logging
        //       and error reporting capabilities.
        static_cast<void>(token_stream_view.consume(MactenAllToken::Tab, MactenAllToken::Space));
        static_cast<void>(token_stream_view.consume(MactenAllToken::Tab, MactenAllToken::Space));
      }

      token_stream_result.push_back(token);
     }

     if (token_stream_result.peek_back().is(MactenAllToken::Newline)) 
     {
      token_stream_result.pop_back();
     }
    
     const auto macro_body = token_stream_result.construct();

     // std::cout << "\nMacro body:\n===================================================\n";
     // std::cout << macro_body;
     // std::cout << "\n===================================================\n";

     advance();
     consume(Token::RBrace, "Expected '}' the end of macro body, found: " + previous.lexeme);
     m_macros.emplace_back(macro_name, macro_body, macro_args);
     return;
    }
    report_error("Expected macro body, missing '{'");
   }
   else
   {
    advance();
   }
  }
 public: 
 std::vector<DeclarativeMacroDetail> m_macros;
};

// TODO: This class needs to be refactored.
// WARN: Do it before it becomes too big.
class MactenWriter
{
private:
 using DeclarativeMacroRules = std::unordered_map<std::string, DeclarativeTemplate>;
 using TokenType = MactenAllToken;
 inline static const std::map<std::string, std::string> EmptyArgList {};
public:

 explicit MactenWriter(std::string_view path, std::string_view output_name)
 : m_source_path{ path }
 , m_output_name{ output_name }
 {
 }

 /**
  * Generate declarative macro rules from source.
  */
 auto generate_declarative_rules() -> bool
 {
  DeclarativeMacroParser parser(m_source_path);
  const auto res = parser.parse();
  if (res)
  {
   std::for_each(parser.m_macros.begin(), parser.m_macros.end(), [this](const auto& macro_detail) {
     m_declarative_macro_rules[macro_detail.m_name] = macro_detail.construct_template();
   });
  }
  return res;
 }

 /**
  * Checks wheter the macro with the given name exists.
  */
 auto has_macro(const std::string& name) -> bool
 {
  return m_declarative_macro_rules.contains(name);
 }

 /**
  * Apply macro rules.
  */
 auto apply_macro_rules(macten::TokenStream<MactenAllToken>& target, macten::TokenStream<MactenAllToken>::TokenStreamView& source_view) -> bool
 {
  while (!source_view.is_at_end())
  {
   auto token = source_view.peek();

   while (source_view.match_sequence(TokenType::Identifier, TokenType::Underscore)) 
   {
    if (source_view.peek(2).is(TokenType::Identifier))
    {
     token.lexeme += "_" + source_view.peek(2).lexeme;
     source_view.advance(2);
    } 
    else
    {
     while (source_view.peek(1).is(TokenType::Underscore))
     {
      token.lexeme += "_";
      source_view.advance(1);
     }
    }
   } 

   const bool macro_call_found = macten::utils::is_macro_call(source_view);

   if (macro_call_found && has_macro(token.lexeme))
   {
    // Move onto the '['.
    source_view.skip_until(TokenType::LSquare);
    if (!source_view.consume(TokenType::LSquare)) return false;

    const auto args = source_view.between(MactenAllToken::LSquare, MactenAllToken::RSquare);
    source_view.advance(args.remaining_size());
    if (!match_and_execute_macro(target, token.lexeme, args.construct()))
    {
     return false;
    }
   }
   else
   {
    // Default. Just push the token, no need to care.
    target.push_back(token);
   }

   source_view.advance();
  }

  return true;
 }

 auto match_and_execute_macro(
  macten::TokenStream<MactenAllToken>& target, 
  const std::string& macro_name,
  const std::string& args
  ) -> bool
 {
    const DeclarativeTemplate macro_rule { this->m_declarative_macro_rules.at(macro_name) };
    const auto args_mapping = macro_rule.map_args(args);
    return macro_rule.apply(this, "", target, args_mapping.value());
 }

 /**
  * Skip macro definition. This removes the macten definitions from the source code.
  */
 auto skip_macro_definition(macten::TokenStream<MactenAllToken>::TokenStreamView& view, macten::TokenStream<MactenAllToken>& target) -> void
 {
  using TokenType = MactenAllToken;
  view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline, TokenType::Identifier);

  int16_t brace_scope {1};

  if (view.consume(TokenType::LBrace))
  {
   while (!view.is_at_end() && brace_scope > 0)
   {
    switch (view.peek().type)
    {
     break; case TokenType::LBrace: ++brace_scope;
     break; case TokenType::RBrace: --brace_scope;
     break; default: {}
    }
    view.advance();
   }

   view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);
  }
 }

 /**
  * Tidy macro call site. This allows for convenient assumptions during the expansion phase.
  */
 auto tidy_macro_call_site(macten::TokenStream<MactenAllToken>::TokenStreamView& view, macten::TokenStream<MactenAllToken>& target) -> void
 {
  using TokenType = MactenAllToken;
  view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);

  int16_t brace_scope {1};

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
 auto preprocess(macten::TokenStream<MactenAllToken>& source) -> macten::TokenStream<MactenAllToken> 
 {
  using TokenType = MactenAllToken;

  macten::TokenStream<MactenAllToken> processed_tokens {};
  auto source_view = source.get_view();

  while (!source_view.is_at_end())
  {
   const auto token = source_view.pop();

   if (token.is(TokenType::DeclarativeDefinition))
   {
    skip_macro_definition(source_view, processed_tokens);
    continue;
   }
   else if (token.is(TokenType::Identifier) 
        && m_declarative_macro_rules.contains(token.lexeme)
        && source_view.match_sequence(TokenType::Exclamation, TokenType::LSquare))
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

  // std::cout << "Raw \n======================================================\n";
  // std::cout << source_tokens.construct();
  // std::cout << "\n======================================================\n";

  source_tokens = preprocess(source_tokens);

  // std::cout << "After preprocess\n======================================================\n";
  // std::cout << source_tokens.construct();
  // std::cout << "\n======================================================\n";

  // Note: It's important that we get the view AFTER preprocess.
  auto source_tokens_view = source_tokens.get_view();
  const auto res = apply_macro_rules(result_tokens, source_tokens_view);

  std::cout << "Result\n======================================================\n";
  std::cout << result_tokens.construct();
  std::cout << "\n======================================================\n";

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
};

#endif /* MACTEN_H */