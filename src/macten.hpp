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
 */
struct DeclarativeTemplate
{
private:
 using Token = cpp20scanner::Token<MactenAllToken>;

public:

 DeclarativeTemplate() = default;

 explicit DeclarativeTemplate(std::string name, const std::string& body, std::vector<std::string> args = {})
 : m_name(name)
 , m_arguments(args)
 {
  MactenAllTokenScanner scanner;
  scanner.set_source(body);

  while (!scanner.is_at_end())
  {
   m_token_stream.push_back(scanner.scan_token());
  }
 }

 [[nodiscard]] auto apply(
     MactenWriter* env,
     TokenStreamType(MactenAllToken)& target, 
     std::map<std::string, std::string> args = {}
  ) const -> bool;

 /**
  * Members.
  */
 std::string                     m_name;
 std::vector<std::string>        m_arguments;
 TokenStreamType(MactenAllToken) m_token_stream;
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
  auto skip_whitespace() noexcept -> void
  {
   while (check(Token::Tab))
   {
     advance();
   }
  }

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
     current = this->scanner.scan_body(Token::LBrace, Token::RBrace);
     auto macro_body = current.lexeme;

     // This is less than ideal.
     while (macro_body.ends_with('\n') 
        ||  macro_body.ends_with('\t') 
        ||  macro_body.ends_with(' '))
     {
      macro_body.pop_back();
     }

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
 inline static const std::map<std::string, std::string> EmptyArgList {};
public:

 explicit MactenWriter(std::string_view path, std::string_view output_name)
 : m_source_path{ path }
 , m_output_name{ output_name }
 {
 }

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

 auto has_macro(const std::string& name) -> bool
 {
  return m_declarative_macro_rules.contains(name);
 }

 auto apply_macro_rules(TokenStreamType(MactenAllToken)& target, TokenStreamType(MactenAllToken)& source) -> bool
 {
  auto source_view = source.get_view();

  // while (!source_view.is_at_end())
  // {
   // const auto token = source_view.peek();

   // // TODO: Perhaps this should be changed to another separate routine.
   // const auto macro_call_found = m_declarative_macro_rules.contains(token.lexeme);

   // if (macro_call_found)
   // {
    
   // }
  // }

  // for (std::size_t index = 0; index < source_size; index++)
  // {
  //  const auto token = source.at(index);
  //  const auto macro_call_found = m_declarative_macro_rules.contains(token.lexeme);
  // }

  // while (!source.empty())
  // {
  //  const auto token = source.pop();
  // }

  //  if (macro_call_found)
  //  {
  //   const auto next1 = scanner.scan_token();
  //   const auto next2 = scanner.scan_token();

  //   if (next1.type == MactenAllToken::Exclamation 
  //   && next2.type == MactenAllToken::LSquare)
  //   {
  //    const auto& macro_rule = m_declarative_macro_rules.at(tok.lexeme);
  //    const auto arg_body = scanner.scan_body(MactenAllToken::LSquare, MactenAllToken::RSquare);
  //    const auto args = macten::utils::map_raw_args_string_to_names(macro_rule.m_arguments, arg_body.lexeme);

  //    if (!macro_rule.apply(this, target, args.value_or(EmptyArgList)))
  //    {
  //     std::cerr << "Failed to apply macro rule.\n";
  //     return false;
  //    }

  //    const auto closing_square = scanner.scan_token();

  //    if (closing_square.type != MactenAllToken::RSquare)
  //    {
  //     std::cerr << "Expected closing square, found: '" << closing_square.lexeme << "'\n";
  //     return false;
  //    }

  //    continue;
  //   }
  //  }
  //  target.push_back(tok);
  // }
  // return true;

  return true;
 }

 /**
  * Remove macro definitions from the final generated code.
  */
 auto preprocess(TokenStreamType(MactenAllToken)& source) -> TokenStreamType(MactenAllToken) 
 {
  using TokenType = MactenAllToken;

  TokenStreamType(MactenAllToken) processed_tokens {};
  auto source_view = source.get_view();

  while (!source_view.is_at_end())
  {
   const auto token = source_view.pop();

   if (token.is(MactenAllToken::DeclarativeDefinition))
   {
    source_view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline, TokenType::Identifier);

    int16_t brace_scope {1};

    if (source_view.consume(TokenType::LBrace))
    {
     while (!source_view.is_at_end() && brace_scope > 0)
     {
      switch (source_view.peek().type)
      {
       break; case TokenType::LBrace: ++brace_scope;
       break; case TokenType::RBrace: --brace_scope;
       break; default: {}
      }
      source_view.advance();
     }

     source_view.skip(TokenType::Space, TokenType::Tab, TokenType::Newline);
    }
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
  TokenStreamType(MactenAllToken) result_tokens;
  auto source_tokens = TokenStreamType(MactenAllToken)::from_file(m_source_path);
  source_tokens = preprocess(source_tokens);

  std::cout << "After preprocessor\n======================================================\n";
  std::cout << source_tokens.construct();
  std::cout << "\n======================================================\n";

  const auto res = apply_macro_rules(result_tokens, source_tokens);

  // std::cout << "\n===================================\n";
  // for (const auto& t : result_tokens.m_tokens)
  // {
  //  std::cout << t.lexeme;
  // }
  // std::cout << "\n===================================\n";

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
 const std::string                                    m_source_path;
 const std::string                                    m_output_name;
 DeclarativeMacroRules                                m_declarative_macro_rules;
};

#endif /* MACTEN_H */