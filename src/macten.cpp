#include "macten.hpp"

auto DeclarativeTemplate::apply(
     MactenWriter* env,
     TokenStreamType(MactenAllToken)& target, 
     std::map<std::string, std::string> args
) const -> bool 
 {
   if (args.size() != m_arguments.size()) return false;

   TokenStreamType(MactenAllToken) result_token_stream {};

   for (std::size_t i = 0; i < m_token_stream.size(); i++)
   {
    const auto token = m_token_stream.at(i);

    const bool is_arg {token.type == MactenAllToken::Dollar
                   &&  (i+1)<m_token_stream.size()
                   &&  args.contains(m_token_stream.at(i+1).lexeme)};

    // An argname has been found, substitute value in.
    if (is_arg)
    {
     const auto argname = m_token_stream.at(i+1).lexeme;
     result_token_stream.add_string(args[argname]);

     // Skip over the argname.
     i++;
     continue;
    }

    const bool is_macro_call = {
     env->has_macro(token.lexeme)
     && (i+1) < m_token_stream.size()
     && m_token_stream.at(i+1).type == MactenAllToken::Exclamation
    };

    if (is_macro_call)
    {
      TokenStreamType(MactenAllToken) macro_call;
      do {
       macro_call.push_back(m_token_stream.at(i));
      } while (m_token_stream.at(i++).type != MactenAllToken::RSquare);
      i--;

      env->apply_macro_rules(result_token_stream, macro_call);
      continue;
    }
   
    result_token_stream.push_back(token);
   }

   // std::cout << "Result: \n========================\n" << result_token_stream.construct() << "\n========================\n";

   target.m_tokens.insert(std::end(target.m_tokens), std::begin(result_token_stream.m_tokens), std::end(result_token_stream.m_tokens));

   return true;
 }
