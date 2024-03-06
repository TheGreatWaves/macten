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

struct DeclarativeMacroParameter
{
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
  * Returns a parameter object with corresponds to the parameter input in the form of a stream view.
  */
 DeclarativeMacroParameter(macten::TokenStream<MactenToken>::TokenStreamView parameter_view)
 : pattern{}
 , pattern_mode(PatternMode::Normal)
 , variadic_pattern{}
 {
  if (parameter_view.peek().is(MactenToken::EndOfFile))
  {
   pattern_mode = PatternMode::Empty;
   return;
  }

  // Populate pattern and argument names.
  while (!parameter_view.is_at_end())
  {
   const auto token = parameter_view.pop();

    if (token.is(MactenToken::Dollar))
    {
      if (parameter_view.peek().is(MactenToken::Identifier))
      {
       argument_names.push_back(parameter_view.pop().lexeme);
       pattern.push_back(token);
      }
      else if (parameter_view.peek().is(MactenToken::LParen))
      {
       pattern_mode = PatternMode::Asterisk;

       parameter_view.advance();
       auto pview = parameter_view.between(MactenToken::LParen, MactenToken::RParen);


       parameter_view.advance(pview.remaining_size() + 1);

       while (!pview.is_at_end())
       {
        const auto& token = pview.peek();

        if (token.is(MactenToken::Dollar))
        {
         variadic_pattern.push_back(pview.pop());

         // Get the variadic container name.
         variadic_container_name = pview.pop().lexeme;
        }
        else
        {
         variadic_pattern.push_back(pview.pop());
        }
       }
      }
      else
      {
       std::cerr << "Expected variable name after '$' symbol." << '\n';
      }
    }
    else
    {
     pattern.push_back(token);
    }
  }
 }

 /**
  * Checks whether the input arguments matches the variadic template.
  */
 [[nodiscard]] auto match_variadic(macten::TokenStream<MactenToken>::TokenStreamView input) const noexcept -> bool
 {
  if (pattern_mode == PatternMode::Normal) return true;
  if (input.peek().is(MactenToken::EndOfFile)) return false;

  if (input.is_at_end()) return false;

  while (!input.peek().is(MactenToken::EndOfFile))
  {
   for (std::size_t idx = 0; idx < variadic_pattern.size(); idx++)
   {
    const auto current_expected_token = variadic_pattern[idx];

    if (current_expected_token.is(MactenToken::Dollar)) 
    {
     if (input.peek().is(MactenToken::LParen))
     {
      input.advance();
      const auto body = input.between(MactenToken::LParen, MactenToken::RParen);
      input.advance(body.remaining_size() + 1);
     }
     else
     {
      input.advance();
     }
     continue;
    }

    const auto token = input.pop();
    const auto current_token_type = token.type;
    if (current_token_type != current_expected_token.type)
    {
     return false;
    }

    // Lexeme (keyword) mismatch.
    if (current_expected_token.type == MactenToken::Identifier && token.lexeme != current_expected_token.lexeme)
    {
     return false;
    }
   }
  }

  return true;
 }

 /**
  * Checks whether the input token stream matches the pattern of the expected parameters or not.
  */
 [[nodiscard]] auto match(macten::TokenStream<MactenToken>::TokenStreamView input) const noexcept -> bool
 {
  // Match parameterless macro.
  if (input.peek().is(MactenToken::EndOfFile) && pattern_mode == PatternMode::Empty) return true;
  for (std::size_t idx = 0; idx < pattern.size(); idx++)
  {
   const auto current_expected_token = pattern[idx];

   if (current_expected_token.type == MactenToken::Dollar) 
   {
    if (input.peek().is(MactenToken::LParen))
    {
     input.advance();
     const auto body = input.between(MactenToken::LParen, MactenToken::RParen);
     input.advance(body.remaining_size() + 1);
    }
    else
    {
     input.advance();
    }
    continue;
   }

   const auto token = input.pop();
   const auto current_token_type = token.type;


   if (current_token_type != current_expected_token.type)
   {
    // For debugging, remove this later.
    std::cerr << "[DEBUG - match] "<< current_token_type.name() << " != " << current_expected_token.type.name() << '\n';
    return false;
   }

   // Lexeme (keyword) mismatch.
   if (current_expected_token.type == MactenToken::Identifier && token.lexeme != current_expected_token.lexeme)
   {
    return false;
   }
  }

  // There is more to match, however we only expect a finite number of arguments.
  if (pattern_mode == PatternMode::Normal && !input.peek().is(MactenToken::EndOfFile)) 
  {
   return false;
  }



  return match_variadic(input);
 }

 /**
  * Match pattern with the input and retrieve argument names and their value.
  */
 [[nodiscard]] auto map_args(macten::TokenStream<MactenAllToken>::TokenStreamView& input) const noexcept -> std::optional<std::map<std::string, std::string>>
 {
  std::map<std::string, std::string> argmap {};

  std::size_t argcount {0};

  for (const auto& expected : pattern)
  {
   const auto& expected_tty = expected.type;
   input.skip(MactenAllToken::Newline, MactenAllToken::Tab, MactenAllToken::Space);
   const auto token = input.pop();

   if (expected_tty == MactenToken::Dollar)
   {
    // Arity error.
    if ((argcount+1) > argument_names.size())
    {
      std::cerr << "Arity error, declared parameter count mismatched.\n";
      return {};
    }

    std::string argval {};
    const std::string& argname = argument_names[argcount];

    // Handle grouping.
    if (token.is(MactenAllToken::LParen))
    {
     const auto expr = input.between(MactenAllToken::LParen, MactenAllToken::RParen);
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
   else
   {
    if (token.type.name() != expected_tty.name())
    {
     return {};
    }
   }
  }

  if (pattern_mode != PatternMode::Normal)
  {
   argmap[variadic_container_name] = input.construct();

   while (!input.is_at_end()) input.advance();
  }

  return argmap;
 }

 PatternMode                                   pattern_mode {};
 std::vector<cpp20scanner::Token<MactenToken>> pattern {};
 std::vector<std::string>                      argument_names {};
 std::string                                   variadic_container_name{};
 std::vector<cpp20scanner::Token<MactenToken>> variadic_pattern {};
};

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
 explicit DeclarativeTemplate(const std::string& name, const std::vector<std::string>& body, const std::vector<DeclarativeMacroParameter>& parameters)
 : m_name(name)
 , m_params(parameters)
 {
  for (const auto& s : body)
  {
   m_token_stream.push_back(macten::TokenStream<MactenAllToken>::from_string(s));
  }
 }

 // Apply declarative template macro, the expansion is written to a target token stream.
 [[nodiscard]] auto apply(
     MactenWriter* env,
     const int index,
     const std::string& indentation,
     macten::TokenStream<MactenAllToken>& target, 
     std::map<std::string, std::string> args = {}
  ) const -> bool;

 /**
  * Returns an argument to argument name match. If the argument match fails, none is returned.
  */
 [[nodiscard]] auto map_args(std::size_t index, 
   macten::TokenStream<MactenAllToken>::TokenStreamView& arg_all_tokens_view) const noexcept -> std::optional<std::map<std::string, std::string>>
 {
  const auto& param = m_params.at(index);
  return param.map_args(arg_all_tokens_view);
 }

 /**
  * Try matching input against all patterns inside the environment and returns the corresponding macro index.
  */
 [[nodiscard]] auto match(macten::TokenStream<MactenToken>::TokenStreamView view) const noexcept -> int 
 {
  for (int index {0}; index < m_params.size(); index++)
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
   * Return a vector of tokens expected by the argument list that
   * is currently being parsed.
   * 
   * // Note: Head token should be LParen '('.
   */
  auto parse_args() noexcept -> std::pair<std::vector<std::string>, std::vector<MactenToken>>
  {
   std::vector<MactenToken> macro_tokens {};
   std::vector<std::string> macro_args {};

   // Move into the body of the '('.
   int scope { 1 };

   while (!this->scanner.is_at_end())
   {
    if (current.is(Token::LParen)) scope++;
    else if (current.is(Token::RParen))
    {
     scope--;

     if (scope <= 0)
     {
      advance();
      break;
     }
    }

    if (match(Token::Dollar))
    {
     macro_tokens.push_back(previous.type);

     consume(Token::Identifier, "Expected argument name.");
     const auto arg_name = previous.lexeme;
     macro_args.push_back(arg_name);
     macro_tokens.push_back(previous.type);
    }
    else
    {
      advance();
      const auto consumed = previous.lexeme;
      macro_tokens.push_back(previous.type);
    }
   }

   return {std::move(macro_args), std::move(macro_tokens)};
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


    std::vector<std::string>               branch_bodies;
    std::vector<DeclarativeMacroParameter> branch_parameters;

    while (!match(Token::RBrace))
    {
     const auto parameter_signature = this->scanner.scan_body(Token::LParen, Token::RParen);
     const auto parameter_signature_stream = macten::TokenStream<MactenToken>::from_string(parameter_signature.lexeme);
     const auto parameter_signature_view = parameter_signature_stream.get_view();
     advance();
    
     branch_parameters.emplace_back(parameter_signature_view);

     consume(Token::RParen, "Expected arguments, missing ')', found: '" + current.lexeme + "'.");
    

     std::vector<std::string> macro_args {};
     std::vector<MactenToken> macro_tokens {};

     consume(Token::Equal, "Expected '=', found: '" + current.lexeme + "'.");
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
    
      branch_bodies.emplace_back(token_stream_result.construct());

     // std::cout << "\nMacro body:\n===================================================\n";
     // std::cout << macro_body;
     // std::cout << "\n===================================================\n";
      advance();
      advance();
    }

    }

     m_macros.emplace_back(
      std::move(macro_name), 
      std::move(branch_bodies), 
      std::move(branch_parameters));
   }
   else
   {
    advance(true);
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

    const auto all_token_stream = macten::TokenStream<MactenAllToken>::from_string(args);
    auto all_token_stream_view = all_token_stream.get_view();

    do 
    {
     const auto token_stream = macten::TokenStream<MactenToken>::from_string(all_token_stream_view.construct());
     auto token_stream_view = token_stream.get_view();

     // Find match.
     const int idx = macro_rule.match(token_stream_view);
     if (idx == -1) return false;

     auto args_mapping = macro_rule.map_args(idx, all_token_stream_view);

     if (!args_mapping.has_value())
     {
      std::cerr << "Failed to create argument mapping for macro '" << macro_name << "'\n";
      return false;
     }


     bool success = macro_rule.apply(this, idx, "", target, args_mapping.value());
     if (!success)
     {
      std::cerr << "Something went really wrong...\n";
      return false;
     }

     while (all_token_stream_view.peek().any_of(MactenAllToken::Newline))
     {
       target.push_back(all_token_stream_view.pop());
     }
    }
    while ((!all_token_stream_view.is_at_end()));
    return true;
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