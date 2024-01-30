#ifndef MACTEN_UTILS_H
#define MACTEN_UTILS_H

#include <map>
#include <optional>
#include "macten_tokens.hpp"

namespace macten
{
 namespace utils
 {
  /**
   * Map input to arglist.
   * The arguments are expected to be comma separated.
   * 
   * NOTE: Map is used instead of unordered_map because the expected number of arguments is rather low.
   */
  inline auto map_raw_args_string_to_names(const std::vector<std::string>& names, const std::string& raw_arglist) -> std::optional<std::map<std::string, std::string>>
  {
   using Token = MactenToken;
   std::map<std::string, std::string> mapping{};
   const auto expected_argcount = names.size();

   MactenTokenScanner scanner;
   scanner.set_source(raw_arglist);

   uint8_t argcount{0};
   while (!scanner.is_at_end())
   {
    scanner.skip_whitespace();
    const auto tok = scanner.scan_until_token(Token::Comma);
    const auto next = scanner.scan_token();

    if (next.type != Token::Comma && next.type != Token::EndOfFile)
    {
     // Unexpected.
     // TODO: Maybe better error handling here.
     return {};
    }

    if (argcount < expected_argcount)
    {
     mapping[names[argcount]] = tok.lexeme;
     argcount++;
    }

    scanner.skip_whitespace();
   }

   if (argcount != expected_argcount) return {};

   return mapping;
  }
 }
}

#endif /* MACTEN_UTILS_H */ 
