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
 const std::string           m_name;
 std::vector<MactenAllToken> m_token_stream;
};


#endif /* MACTEN_H */