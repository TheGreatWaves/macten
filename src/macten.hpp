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


 // Remove macro definitions. This is done to exclude it from the final generated source code.
 // TODO: Improve this! At the moment there is no easy way to skip whitespace.
 auto preprocess(TokenStreamType(MactenAllToken)& source) -> TokenStreamType(MactenAllToken) 
 {
  // TokenStreamType(MactenAllToken) processed_tokens {};
  // std::size_t source_stream_size = source.size();

  // for (std::size_t i = 0; i < source_stream_size; i++)
  // {
  //  const auto tok = source.at(i);
  //  std::size_t skip_step {0};

  //  if (tok.type == MactenAllToken::DeclarativeDefinition)
  //  {

  //   // NOTE: This is ugly and horrible. Improvement to API for tokens needed ASAP.
  //   if ((i+skip_step+1) < source_stream_size 
  //   && (source.at(i+skip_step+1).type == MactenAllToken::Space) || (source.at(i+skip_step+1).type == MactenAllToken::Tab))
  //   {
  //    skip_step++;
  //   }

  //   // Skip macro name.
  //   if ((i+skip_step+1) < source_stream_size && source.at(i+skip_step+1).type == MactenAllToken::Identifier)
  //   {
  //    skip_step++;
  //   }

  //   // NOTE: Hackfix. Need to remove.
  //   if ((i+skip_step+1) < source_stream_size 
  //   && (source.at(i+skip_step+1).type == MactenAllToken::Space) || (source.at(i+skip_step+1).type == MactenAllToken::Tab))
  //   {
  //    skip_step++;
  //   }

  //   // Skip macro body.
  //   if ((i+skip_step+1) < source_stream_size && source.at(i+skip_step+1).type == MactenAllToken::LBrace)
  //   {
  //    skip_step++;
  //    std::size_t brace_scope = 1;

  //    while ((i+skip_step+1) < source_stream_size)
  //    {
  //     skip_step++;
  //     const auto body_tok = source.at(i+skip_step);

  //     if (body_tok.type == MactenAllToken::LBrace)
  //     {
  //      brace_scope++;
  //     }
  //     else if (body_tok.type == MactenAllToken::RBrace)
  //     {
  //      brace_scope--;
  //     }

  //     if (brace_scope == 0) break;
  //    }
  //   }

  //   // NOTE: We don't need to care about the newline.
  //   if ((i+skip_step+1) < source_stream_size 
  //   && source.at(i+skip_step+1).type == MactenAllToken::Newline)
  //   {
  //    skip_step++;
  //   }

  //   i += skip_step;
  //   continue;
  //  }

  //  processed_tokens.push(tok);
  // }


  // return processed_tokens;
  return {};
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