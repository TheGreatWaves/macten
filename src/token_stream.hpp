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

   TokenStreamView(std::size_t start_index, std::size_t max_size, const TokenStream* ts)
   : m_current_pointer{ start_index }
   , m_max_size{ max_size }
   , m_target{ ts }
   {
   }

  /**
   * Returns true when the view has been exhausted.
   */
  [[nodiscard]] auto is_at_end(std::size_t offset = 0) const noexcept -> bool
  {
   return (m_current_pointer + offset) >= m_max_size;
  }

  /**
   * Returns the top Token with an offset.
   *
   * NOTE: If the index is greater than the size of the view, EndOfFile will be returned.
   */
  [[nodiscard]] auto peek(std::size_t offset = 0) const noexcept -> Token
  {
    const bool invalid_target = m_target == nullptr;
    if (invalid_target || is_at_end(offset)) return Token();

    return m_target->at(m_current_pointer + offset);
  }

  /**
   * Returns the top Token with an offset.
   */
  [[nodiscard]] auto peek_back(std::size_t offset = 1) const noexcept -> Token
  {
    const bool invalid_target = m_target == nullptr;
    const int index = static_cast<int>(m_current_pointer) - offset;
    if (invalid_target || index < 0) return Token();

    return m_target->at(index);
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
   * Create a sub view.
   */
  [[nodiscard]] auto sub_view(std::size_t size = 0) -> TokenStreamView
  {
    return TokenStreamView{
     m_current_pointer,
     m_current_pointer + size,
     m_target
    };
  }

  /**
   * Return the remaining size of the view.
   */
  [[nodiscard]] auto remaining_size() const noexcept -> std::size_t
  {
    return m_max_size - m_current_pointer;
  }

  /**
   * Return the size of the view.
   */
  [[nodiscard]] auto size() const noexcept -> std::size_t
  {
    return m_max_size;
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

  /**
   * Return the string form of the view.
   */
  [[nodiscard]] auto construct() const noexcept -> std::string
  {
    std::stringstream ss {};

    std::size_t offset {0};   
    while (!is_at_end(offset))
    {
     ss << peek(offset).lexeme;
     offset++;
    }

    return ss.str();
  }

  /**
   * Starting from the current point, scan until the specified token type is found. 
   * Return a new view which holds all of the tokens before the given point.
   */
  auto until(TokenType token_type) -> TokenStreamView
  {
   std::size_t offset {0};

   while (!peek(offset).is(token_type))
   {
    offset++;
   }

   return TokenStreamView {
     m_current_pointer,
     m_current_pointer + offset,
     m_target
   };
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
