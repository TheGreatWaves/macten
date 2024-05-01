#ifndef MACTEN_PROCEDURAL_MACRO_WRITER_H
#define MACTEN_PROCEDURAL_MACRO_WRITER_H

#include <iostream>
#include <sstream>

namespace macten
{

// CodeEmitter is used to generate Python code which is used to handle procedural macros. 
struct CodeEmitter
{
  explicit CodeEmitter() 
  {
   this->comment("AUTO GENERATED CODE, DO NOT EDIT");

   this->section("Imports");
   this->write("import \"prod_macro_utils.py\"");

   this->section("Structures / Storage");
   this->comment("STORAGE FOR ALL PROCEDURAL MACRO RULES");
   this->write("ctx = ProceduralMacroContext()");

   this->section("Driver");
   this->write("if __name__ == \"__main__\":");
   this->inc_indent();
   this->write("print(ctx.rules)");
   this->dec_indent();
  }

  /**
   * Methods.
   */
  inline auto newln(std::size_t nlc = 1) -> void
  {
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
   this->write(line);
   this->comment(std::string(name) + " #");
   this->write(line);
   this->newln();
  }

  inline auto match_indent() -> void 
  {
   for (std::size_t level{0}; level < this->indent_level; level++)
   {
    this->code << std::string(4, ' ');
   }
  }

  inline auto inc_indent() -> void
  {
   this->indent_level++;
  }

  inline auto dec_indent() -> void
  {
   this->indent_level--;
  }

  /**
   * Write line.
   */
  inline auto write(std::string_view line) -> void
  {
   match_indent();
   this->code << line << '\n';
  }

  /**
   * Write comment.
   */
  inline auto comment(std::string_view message) -> void
  {
   match_indent();
   this->code << "# " << message << '\n';
  }

  /**
   * Dump generated code. Outputs to cout if target is not specified.
   */
  inline auto dump(std::ostream& os = std::cout) -> void
  {
   os << this->code.str();
  }

  /**
   * Members.
   */
  std::stringstream code{};
  std::size_t indent_level {0};
};


 
} // namespace macten

#endif /*MACTEN_PROCEDURAL_MACRO_WRITER_H*/
