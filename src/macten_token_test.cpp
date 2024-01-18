#define DEBUG

#include "macten_tokens.hpp"
#include "macten_all_tokens.hpp"

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
    consume(Token::LBrace, "Expected macro expansion body, missing  '{'.");

    current = this->scanner.scan_body(Token::LBrace, Token::RBrace);
    std::cout << "Macro body: \n" << current.lexeme << '\n';

    advance();

    consume(Token::RBrace, "Expected '}' the end of macro body, found: " + previous.lexeme);
   }
   else
   {
    advance();
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