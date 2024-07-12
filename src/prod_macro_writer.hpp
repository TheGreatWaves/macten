#ifndef MACTEN_PROCEDURAL_MACRO_WRITER_H
#define MACTEN_PROCEDURAL_MACRO_WRITER_H

#include <iostream>
#include <sstream>
#include <functional>

namespace macten
{

struct ScopeCallback
{
  explicit ScopeCallback(std::function<void()> f)
  : callback(f)
  {
  }

  ~ScopeCallback()
  {
   callback();
  }

   explicit operator bool() const noexcept {
       return true;
   }

  std::function<void()> callback;
};

// CodeEmitter is used to generate Python code which is used to handle procedural macros. 
struct CodeEmitter
{
  explicit CodeEmitter() 
  {
   this->comment("AUTO GENERATED CODE, DO NOT EDIT");

   this->section("Imports");
   this->writeln("import macten");
   this->writeln("from macten import ListStream, ProceduralMacroContext, ident, number, parse_fn, NodeUtils");
   this->writeln("from typing import Any");
   this->writeln("from dataclasses import dataclass");
  }

  /**
   * Methods.
   */
  inline auto newln(std::size_t nlc = 1) -> void
  {
   this->need_indent = true;
   for (std::size_t i {0}; i < nlc; i++)
   {
    this->code << '\n';
   }
  }

  inline auto section(std::string_view name) -> void
  {
   std::size_t size = name.length() + 2;
   const std::string line = "#" + std::string(size, '=') + "#";
   this->newln();
   this->writeln(line);
   this->comment(std::string(name) + " #");
   this->writeln(line);
   this->newln();
  }

  inline auto match_indent() -> void 
  {
   for (std::size_t level{0}; level < this->indent_level; level++)
   {
    this->code << std::string(4, ' ');
   }
  }

  inline auto indent() -> void
  {
   this->indent_level++;
  }

  inline auto begin_indent(std::string_view line = "") -> ScopeCallback
  {
   if (!line.empty()) this->writeln(line);
   this->indent();
   return ScopeCallback([&] {this->dec_indent();});
  }

  inline auto dec_indent() -> void
  {
   this->indent_level--;
  }

  /**
   * Write line.
   */
  inline auto writeln(std::string_view line) -> void
  {
   this->match_indent();
   this->code << line;
   this->newln();
  }

  inline auto write(std::string_view line, std::string_view postfix = "") -> void
  {
   if (this->need_indent) this->match_indent();
   this->code << line << postfix;
   this->need_indent = false;
  }

  /**
   * Write comment.
   */
  inline auto comment(std::string_view message) -> void
  {
   this->match_indent();
   this->code << "# " << message;
   this->newln();
  }

  /**
   * Dump generated code. Outputs to cout if target is not specified.
   */
  inline auto dump(std::ostream& os = std::cout) -> const std::string
  {
   return this->code.str();
  }

  /**
   * Members.
   */
  std::stringstream code{};
  std::size_t       indent_level {0};
  bool              need_indent {true};
};


 
} // namespace macten

#endif /*MACTEN_PROCEDURAL_MACRO_WRITER_H*/
