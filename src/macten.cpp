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
     const int index,
     macten::TokenStream<MactenAllToken>& target, 
     std::map<std::string, std::string> args
) const -> bool 
 {
   using TokenType = MactenAllToken;
   macten::TokenStream<MactenAllToken> temp_buffer;

   // Arity issue.
   if (m_params[index].pattern_mode == DeclarativeMacroParameter::PatternMode::Normal && args.size() != m_params[index].argument_names.size()) return false;

   auto view = m_token_stream[index].get_view();

   while (!view.is_at_end())
   {
    const bool _is_arg {
     view.match_sequence(TokenType::Dollar, TokenType::Identifier)
    };
    const bool is_macro_call { macten::utils::is_macro_call(view) };


    const auto _token = view.peek();

    if (_is_arg)
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
       env->apply_macro_rules(temp_buffer, sub_view);
     }
     else
     {
       temp_buffer.push_back(_token);
     }
    }
    else if (is_macro_call & env->has_macro(_token.lexeme))
    {
      auto arg_body = view.between(MactenAllToken::LSquare, MactenAllToken::RSquare, false);
      view.advance(arg_body.remaining_size()+3);

      std::stringstream args_string {};

      while (!arg_body.is_at_end()) 
      {
        const bool is_arg { arg_body.match_sequence(TokenType::Dollar, TokenType::Identifier) };
        const auto token = arg_body.peek();

        if (is_arg)
        {
         const std::string argname = arg_body.peek(1).lexeme;
         if (args.contains(argname))
         {
           arg_body.advance();
           const std::string& arg_value = args[argname];
           args_string << arg_value;
         }
         else
         {
           args_string << token.lexeme;
         }
        }
        else
        {
         args_string << token.lexeme;
        }
        arg_body.advance();
      }

      if (!env->match_and_execute_macro(temp_buffer, _token.lexeme, args_string.str()))
      {
       return false;
      }
    }
    else 
    {
     temp_buffer.push_back(_token);
    }

    view.advance();
   }

   auto temp_buffer_view = temp_buffer.get_view();
   env->apply_macro_rules(target, temp_buffer_view);

   return true;
 }
