#pragma once
#ifndef MACTEN_H
#define MACTEN_H

#include <string>
#include "macten_all_tokens.hpp"

/**
 * Declarative Macro.
 */
struct DeclarativeTemplate
{
 explicit DeclarativeTemplate(std::string_view name, const std::string& body)
 : m_name(name)
 {
  MactenAllTokenScanner scanner;
  scanner.set_source(body);

  while (!scanner.is_at_end())
  {
   m_token_stream.push_back(scanner.scan_token());
  }
 }

 /**
  * Members.
  */
 const std::string                          m_name;
 std::vector<detail::Token<MactenAllToken>> m_token_stream;
};

#endif /* MACTEN_H */