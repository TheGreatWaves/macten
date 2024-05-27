#pragma once
#ifndef MACTEN_PROCEDURAL_MACRO_DEFINTION_H
#define MACTEN_PROCEDURAL_MACRO_DEFINTION_H

#include <map>
#include <vector>
#include <string>

#include "prod_macro_writer.hpp"

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define TEMP auto CONCAT(_TEMP_, __LINE__) =

namespace macten
{
/**
 * A procedural macro is made up of rules.
 * Each of the rule dictates how a given syntax can be parsed and understood.
 *
 * Say we get:
 *
 * defmacten_prod assignment {
 *   type { int | string | ident }
 *   variable { ident }
 *   declaration { type variable; }
 * }
 *
 * There are three seperate rules here, "type", "variable" and "declaration".
 */
 using ProceduralMacroRule = std::vector<std::string>;

/**
 * The ProceduralMacroProfile struct is for storing definitions of procedural macros.
 * It holds methods for building and dumping equivalent python definitions.
 * 
 * A 'profile' specifically, is a single procedural macro rule.
 * So given:
 *
 * defmacten_prod Foo {...}
 * defmacten_prod Bar {...}
 * 
 * We would generate two different profiles.
 */
struct ProceduralMacroProfile
{
  /**
   * Default Ctor.
   */
  explicit ProceduralMacroProfile() = default;

  /*
   * Builder APIs.
   */

  /**
   * Set procedural macro profile name.
   */
  auto set_name(const std::string& name) -> void
  {
    this->name = name;
  }

  /**
   * Create a new rule and return a reference to it.
   */
  auto create_rule(const std::string& name) -> ProceduralMacroRule&
  {
   // Create an empty rule with the given name.
   // Note: This could potentially overwrite any 
   //
   // previously defined rule with the same name.
   this->rules[name] = {};

   // Return the empty rule.
   return this->rules[name];
  }
  
  /**
   * Dump python code for generating the rules.
   */
  auto dump_rules(CodeEmitter& emitter) -> void 
  {
    for (const auto& [rule_name, rule_definition] : this->rules)
    {
     {
       emitter.writeln("@dataclass");
       const auto name = this->name + "_" + rule_name;
       TEMP emitter.begin_indent("class " + name + ":");

       for (const auto& [member_name, _] : this->rules)
        emitter.writeln("_" + member_name + ": any");

       {
         TEMP emitter.begin_indent("def parse(input: ListStream):");
         {
           {
             TEMP emitter.begin_indent("if input.empty():");
             emitter.writeln("return None");
           }
           for (const auto& rule_value : rule_definition)
           {
             {
               TEMP emitter.begin_indent("elif value := (input.pop_if(\"" + rule_value + "\")):");
               emitter.writeln("return " + name + "(_" + rule_name + "= value)");
             }
           }
           emitter.writeln("return None");
         }
       }
     }
    }
  }

  auto dump_driver(CodeEmitter& emitter) -> void
  {
    emitter.section("Driver");
    emitter.writeln("input = ListStream(\"int foo\")");
    emitter.writeln("print(assignment_type.parse(input))");
  }

  /**
   * Dump python code which generates the procedural macro profile.
   */
  auto dump() -> void 
  {
    macten::CodeEmitter emitter{};

    // Begin profile section.
    emitter.section("Profile: " + this->name);

    dump_rules(emitter);

    dump_driver(emitter);

    // For debugging. (Target a file later)
    emitter.dump();
  }

  /**
   * Members.
   */
  std::string                                name  {};
  std::map<std::string, ProceduralMacroRule> rules {};
};

}; // namespace macten

#undef CONCAT
#undef CONCAT_INNER
#undef TEMP 

#endif /*MACTEN_PROCEDURAL_MACRO_DEFINTION_H*/

