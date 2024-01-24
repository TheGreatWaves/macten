#pragma once
#ifndef MACTEN_H
#define MACTEN_H

#define DEBUG

#include <algorithm>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "utils.hpp"
#include "macten_tokens.hpp"
#include "macten_all_tokens.hpp"


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

 auto apply(std::vector<Token>& target) const -> void
 {
   target.insert(std::end(target), std::begin(m_token_stream), std::end(m_token_stream));
 }

 /**
  * Members.
  */
 std::string                                      m_name;
 std::vector<std::string>                         m_arguments;
 std::vector<cpp20scanner::Token<MactenAllToken>> m_token_stream;
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
  // TODO: Move this logic into scanner.
  std::vector<cpp20scanner::Token<MactenAllToken>> file_tokens;

  MactenAllTokenScanner scanner{};
  if (!scanner.read_source(m_source_path))
  {
   return false;
  }
  while (!scanner.is_at_end())
  {
   const auto tok = scanner.scan_token();
   const auto macro_call_found = m_declarative_macro_rules.contains(tok.lexeme);

   if (macro_call_found)
   {
    const auto next1 = scanner.scan_token();
    const auto next2 = scanner.scan_token();

    if (next1.type == MactenAllToken::Exclamation 
    && next2.type == MactenAllToken::LSquare)
    {
     const auto& macro_rule = m_declarative_macro_rules.at(tok.lexeme);
     const auto arg_body = scanner.scan_body(MactenAllToken::LSquare, MactenAllToken::RSquare);
     const auto args = macten::utils::map_raw_args_string_to_names(macro_rule.m_arguments, arg_body.lexeme);

     if (args)
     {
      for (const auto& [k, v] : args.value())
      {
      std::cout << "Arg: " << k << " -> " << v << '\n';
      }
     }

     macro_rule.apply(file_tokens);
     const auto closing_square = scanner.scan_token();

     if (closing_square.type != MactenAllToken::RSquare)
     {
      std::cerr << "Expected closing square, found: '" << closing_square.lexeme << "'\n";
      return false;
     }

     continue;
    }
   }
   file_tokens.push_back(tok);
  }

  for (const auto& t : file_tokens)
  {
   std::cout << t.lexeme;
  }

  return true;
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
 std::unordered_map<std::string, DeclarativeTemplate> m_declarative_macro_rules;
};

#endif /* MACTEN_H */