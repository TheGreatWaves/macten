#include "macten.hpp"
#include "token_stream.hpp"

/**
 * Apply declarative template macro on the target token stream. This function returns the succcess status of the macro expansion.
 * Note that the expansion is recursive, which is why we need to pass in MactenWriter pointer so we can further expand the contents of our macro expansion.
 * TODO: Better error diagnostics would be great.
 */
auto DeclarativeTemplate::apply(
     MactenWriter* env,
     const std::string& indentation,
     macten::TokenStream<MactenAllToken>& target, 
     std::map<std::string, std::string> args
) const -> bool 
 {
   using TokenType = MactenAllToken;

   // Arity issue.
   if (args.size() != m_arguments.size()) return false;

   auto view = m_token_stream.get_view();

   while (!view.is_at_end())
   {
    const bool is_arg {
     view.match_sequence(TokenType::Dollar, TokenType::Identifier)
    };
    const bool is_macro_call {
     view.match_sequence(TokenType::Identifier, TokenType::Exclamation, TokenType::LSquare)
    };

    const auto token = view.pop();

    if (token.is(TokenType::Newline))
    {
     target.push_back(token);
     target.add_string(indentation);
     continue;
    }
    else if (is_macro_call && env->has_macro(token.lexeme))
    {
     continue;
    }
    else if (is_arg)
    {
     const std::string argname = view.peek().lexeme;
     view.advance();

     if (args.contains(argname))
     {
      target.add_string(args[argname]);
     }
     else
     {
      std::cerr << "Failed to substitute argument name '" << argname << "'\n";
      return false;
     }
     continue;
    }

    target.push_back(token);
   }
   return true;
 }
