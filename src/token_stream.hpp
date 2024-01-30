#ifndef MACTEN_TOKEN_STREAM_H
#define MACTEN_TOKEN_STREAM_H
#pragma once

#include <stack>
#include <concepts>
#include <sstream>
#include <string>
#include <algorithm>

// This is required for the Token class.
#include "macten_all_tokens.hpp"

namespace macten
{

 /**
  * A stream of tokens of the specified token type.
  */
template <typename TokenType, typename TokenTypeScanner>
struct TokenStream
{
 /**
  * Aliases.
  */
 using Token = cpp20scanner::Token<TokenType>;

 /**
  * Structures.
  */

 /**
  * The TokenStreamView provides a view into the TokenStream. The parameters for the view is constructed
  * using the current state of the TokenStream, which means that the view is static.
  * 
  * WARN: While the view is active and being used, modifying the parent TokenStream may cause unwanted behavior.
  */
 class TokenStreamView
 {
 public:
  /**
   * Methods.
   */

  /**
   * Ctor.
   */
   TokenStreamView(const TokenStream* ts)
   : m_current_pointer{0}
   , m_max_size{ts->size()}
   , m_target{ts}
   {
   }

  /**
   * Returns true when the view has been exhausted.
   */
  [[nodiscard]] auto is_at_end(std::size_t offset = 0) noexcept -> bool
  {
   return (m_current_pointer + offset) >= m_max_size;
  }

  /**
   * Returns the top Token with an offset.
   *
   * NOTE: If the index is greater than the size of the view, EndOfFile will be returned.
   */
  [[nodiscard]] auto peek(std::size_t offset = 0) noexcept -> Token
  {
    const bool invalid_target = m_target == nullptr;
    if (invalid_target || is_at_end(offset)) return Token();

    return m_target->at(m_current_pointer + offset);
  }

  /**
   * Returns true if the a given sequence is found, starting from the top.
   */
  template <std::same_as<TokenType>... TokenTypes>
  [[nodiscard]] auto match_sequence(TokenTypes... tokens)
  {
    std::size_t offset {0};
    return ((peek(offset++).type == tokens) && ...);
  }

  /**
   * Returns the top element and increment the pointer.
   *
   * NOTE: Size checking is not needed because peek() will return EndOfFile when out of bounds.
   */
  [[nodiscard]] auto pop() noexcept -> Token
  {
    const auto token = peek();
    advance();
    return token;
  }

  /**
   * Advance the pointer.
   */
  auto advance(std::size_t steps = 1) noexcept -> void
  {
    m_current_pointer += steps;
  }

  /**
   * Returns true if the top element matches any of the tokens specified.
   */
  template<std::same_as<TokenType> ...Tokens>
  [[nodiscard]] auto match(Tokens... tokens) -> bool 
  {
   const auto token = peek();
   return token.any_of(tokens...);
  }

  /**
   * Advance and return true if the token matches any of the specified token types.
   */
  template<std::same_as<TokenType> ...Tokens>
  [[nodiscard]] auto consume(Tokens... tokens) -> bool 
  {
   const auto token = peek();
   const bool matched = token.any_of(tokens...);

   if (matched)
     advance();

   return matched;
  }

  /**
   * While the top element matches one of the TokenTypes, move onto the next.
   */
  template<std::same_as<TokenType> ...Tokens>
  auto skip(Tokens... tokens) -> void
  {
   while (!is_at_end() && match(tokens...))
   {
    advance();
   }
  }

 private:
  /**
   * Members.
   */
  const std::size_t  m_max_size {0};
  std::size_t        m_current_pointer {0};
  const TokenStream* m_target {nullptr};
 };


 /**
  * Methods.
  */

 /**
  * Construct and returns the string representation of the token stream.
  */
 [[nodiscard]] auto construct() const noexcept-> std::string
 {
  std::stringstream ss;
  std::for_each(std::begin(m_tokens), std::end(m_tokens), [&](const Token& tok) {
   ss << tok.lexeme;
  });
  return ss.str();
 }

 /**
  * Add the string into the token stream.
  */
 auto add_string(const std::string& input) -> void
 {
   TokenTypeScanner scanner;
   scanner.set_source(input);
   while (!scanner.is_at_end())
   {
    push_back(scanner.scan_token());
   }
 }

 /**
  * Returns the token at the specified index.
  */
 [[nodiscard]] auto at(std::size_t index) const -> Token 
 {
  return m_tokens.at(index);
 }

 /**
  * Construct a token stream from string input.
  */
 static auto from_string(const std::string& input) -> TokenStream
 {
  TokenStream<TokenType, TokenTypeScanner> t;
  t.add_string(input);
  return t;
 }

 /**
  * Construct a token stream from file content.
  */
 static auto from_file(const std::string& path) -> TokenStream
 {
  TokenStream ts{};
  TokenTypeScanner scanner{};
  if (!scanner.read_source(path)) return ts;
  while (!scanner.is_at_end())
  {
    ts.push_back(scanner.scan_token());
  }
  return ts;
 }

 /**
  * Push the token to the back of the token stream.
  */
 auto push_back(Token tok) -> void
 {
   m_tokens.push_back(tok);
 }

 /**
  * Pop the last element.
  */
 auto pop_back() -> void
 {
   m_tokens.pop_back();
 }

 /**
  * Return the size of the token stream.
  */
 [[nodiscard]] auto size() const noexcept -> std::size_t
 {
  return m_tokens.size();
 }

 /**
  * Returns whether the token stream is empty.
  */
 [[nodiscard]] auto empty() const noexcept -> bool 
 {
  return m_tokens.empty();
 }

 /**
  * Returns an immutable view into the TokenStream.
  *
  * WARN: Do not mutate the TokenStream while a view is active.
  */
 [[nodiscard]] auto get_view() const noexcept -> TokenStreamView
 {
   return TokenStreamView(this);
 }

 /**
  * Members.
  */
 std::vector<Token> m_tokens;
};


} // namespace macten

#define TokenStreamType(TokenType) macten::TokenStream<TokenType, TokenType ## Scanner>

#endif /* MACTEN_TOKEN_STREAM_H */
