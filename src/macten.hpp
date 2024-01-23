#pragma once
#ifndef MACTEN_H
#define MACTEN_H

#define DEBUG

#include <algorithm>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "macten_tokens.hpp"
#include "macten_all_tokens.hpp"

/**
 * Declarative Macro.
 */
struct DeclarativeTemplate
{
 DeclarativeTemplate() = default;

 explicit DeclarativeTemplate(std::string name, const std::string& body)
 : m_name(name)
 {
  MactenAllTokenScanner scanner;
  scanner.set_source(body);

  while (!scanner.is_at_end())
  {
   m_token_stream.push_back(scanner.scan_token());
  }
 }

 /**
  * Members.
  */
 std::string                                m_name;
 std::vector<detail::Token<MactenAllToken>> m_token_stream;
};

class DeclarativeMacroParser : public detail::BaseParser<MactenTokenScanner, MactenToken>
{
 private:
  using Token = MactenToken;
 public:
   [[nodiscard]] explicit DeclarativeMacroParser(const std::string& file_path)
       : detail::BaseParser<MactenTokenScanner, MactenToken>(file_path)
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

    while(!match(Token::RParen))
    {
     const auto argname = current.lexeme;
     log("Argname: " + argname);
     advance();
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
     m_macros.emplace_back(macro_name, macro_body);
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
 std::vector<std::pair<std::string, std::string>> m_macros;
};
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
   std::for_each(parser.m_macros.begin(), parser.m_macros.end(), [this](const auto& pair) {
     const auto& [macro_name, macro_body] = pair;
     m_declarative_macro_rules[macro_name] = DeclarativeTemplate(macro_name, macro_body);
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
  std::vector<detail::Token<MactenAllToken>> file_tokens;

  MactenAllTokenScanner scanner{};
  if (!scanner.read_source(m_source_path))
  {
   return false;
  }
  while (!scanner.is_at_end())
  {
   const auto tok = scanner.scan_token();

   if (m_declarative_macro_rules.contains(tok.lexeme))
   {
    const auto next1 = scanner.scan_token();
    const auto next2 = scanner.scan_token();

    if (next1.type == MactenAllToken::Exclamation 
    && next2.type == MactenAllToken::LSquare)
    {
     auto _ = scanner.scan_body(MactenAllToken::LSquare, MactenAllToken::RSquare);

     const auto& macro_body = m_declarative_macro_rules.at(tok.lexeme).m_token_stream;
     file_tokens.insert(std::end(file_tokens), std::begin(macro_body), std::end(macro_body));

     // Consume ']' of the arg list.
     _ = scanner.scan_token();
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