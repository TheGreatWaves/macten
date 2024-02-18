#include "macten.hpp"
#include "token_stream.hpp"
#include "utils.hpp"

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
    const bool is_macro_call { macten::utils::is_macro_call(view) };

    const auto token = view.peek();

    if (is_arg)
    {
     const std::string argname = view.peek(1).lexeme;
     if (args.contains(argname))
     {
       view.advance();
       const std::string& arg_value = args[argname];
       const auto sub_ts = macten::TokenStream<MactenAllToken>::from_string(arg_value);
       auto sub_view = sub_ts.get_view();

       // A little ugly, but this is the best way to trim.
       static_cast<void>(sub_view.consume(TokenType::Tab, TokenType::Space));
       env->apply_macro_rules(target, sub_view);
     }
     else
     {
       target.push_back(token);
     }
    }
    else if (is_macro_call & env->has_macro(token.lexeme))
    {
      const auto args = view.between(MactenAllToken::LSquare, MactenAllToken::RSquare, false);
      view.advance(args.remaining_size()+3);
      if (!env->match_and_execute_macro(target, token.lexeme, args.construct()))
      {
       return false;
      }
    }
    else 
    {
     target.push_back(token);
    }

    view.advance();
   }

   return true;
 }
