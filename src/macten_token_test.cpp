#include "macten_tokens.hpp"

class MactenParser : public detail::BaseParser<MactenTokenScanner, MactenToken>
{
 private:
  using Token = MactenToken;
 public:
   [[nodiscard]] explicit MactenParser(const std::string& file_path)
       : detail::BaseParser<MactenTokenScanner, MactenToken>(file_path)
   {
   }

   /**
    * Default Ctor prohibted.
    */
   constexpr MactenParser() = delete;

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
  auto declaration() noexcept -> void 
  {
   if (match(Token::DeclarativeDefinition))
   {
    consume(MactenToken::Identifier, "Expected macro name, found: " + current.lexeme + ".");
    const auto macro_name = previous.lexeme;

    // Parse the body.
    consume(Token::LBrace, "Expected '{', found: " + current.lexeme + ".");

    // Parse parameters.
    consume(Token::LParen, "Expected '(', found: " + current.lexeme + ".");

    while (!match(Token::RParen))
    {
      // Parse each parameter.
      std::cout << current.lexeme << " " << current.type.name() << '\n';
      advance();
    }

    // TODO: Extend multi character symbols.
    consume(Token::Equal, "Expected '=', found: " + current.lexeme + ".");
    consume(Token::GreaterThan, "Expected '>', found: " + current.lexeme + ".");

    consume(Token::LBrace, "Expected '{', found: " + current.lexeme + ".");
    while (!match(Token::RBrace))
    {
      std::cout << current.lexeme << " " << current.type.name() << '\n';
      advance();
    }

    consume(Token::RBrace, "Expected '}', found: " + current.lexeme + ".");
   }
  }
};

auto main() -> int 
{
 MactenParser scanner("../examples/python/basic_declarative_macro.py");
 if (!scanner.parse())
 {
  return 1;
 }
 return 0;
}