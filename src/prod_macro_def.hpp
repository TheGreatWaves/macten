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
 using ProceduralMacroRule = std::vector<std::vector<std::string>>;

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

       const auto get_name = [&](const std::string& name) { return this->name + "_" + name; };
       const auto name = get_name(rule_name);

       TEMP emitter.begin_indent("class " + name + ":");

       emitter.writeln("_value: Any");

       emitter.newln();

       {
        emitter.writeln("@staticmethod");
         TEMP emitter.begin_indent("def parse(input: ListStream):");
         {
           {
             TEMP emitter.begin_indent("if input.empty():");
             emitter.writeln("return None, None");
           }
           for (const auto& rule_values : rule_definition)
           {
             if (rule_values.empty()) continue;
             else if (rule_values.size() == 1)
             {
               const auto &rule_value = rule_values[0];
               {
                 {
                   TEMP emitter.begin_indent("while True:");
                   emitter.writeln("t_input = input.deepcopy()");
                   if (rules.contains(rule_value))
                   {
                    TEMP emitter.begin_indent("if (value := (" + get_name(rule_value) + ".parse(t_input)))[1]:");
                    emitter.writeln("return value[0], " + name + "(_value=value[1])");
                   }
                   else if (rule_value == "ident")
                   {
                    TEMP emitter.begin_indent("if (value := (ident.parse(t_input))):");
                    emitter.writeln("return value[0], " + name + "(_value=value[1])");
                   }
                   else if (rule_value == "number")
                   {
                    TEMP emitter.begin_indent("if (value := (number.parse(t_input))):");
                    emitter.writeln("return value[0], " + name + "(_value=value[1])");
                   }
                   else 
                   {
                    TEMP emitter.begin_indent("if (value := (t_input.pop_if(\"" + rule_value + "\"))):");
                    emitter.writeln("return t_input, " + name + "(_value=value)");
                   }
                   emitter.writeln("break");
                 }
               }
             }
             else
             {
               const auto &rule_value = rule_values[0];
               {
                 TEMP emitter.begin_indent("while True:");
                 emitter.writeln("t_input = input.deepcopy()");
                 if (rules.contains(rule_value))
                 {
                   TEMP emitter.begin_indent("if (value := (" + get_name(rule_value) + ".parse(t_input)))[1]:");
                   emitter.writeln("t_input, ast = value");
                   emitter.writeln("value = {\"" + rule_value + "\": ast}");
                 }
                 else if (rule_value == "ident")
                 {
                   TEMP emitter.begin_indent("if (value := (ident.parse(t_input)))[1]:");
                   emitter.writeln("t_input, ast = value");
                   emitter.writeln("value = {\"" + rule_value + "\": ast}");
                 }
                 else if (rule_value == "number")
                 {
                   TEMP emitter.begin_indent("if (value := (number.parse(t_input)))[1]:");
                   emitter.writeln("t_input, ast = value");
                   emitter.writeln("value = {\"" + rule_value + "\": ast}");
                 }
                 else 
                 {
                   TEMP emitter.begin_indent("if t_input.pop_if(\"" + rule_value + "\"):");
                   emitter.writeln("value = {}");
                 }
                 TEMP emitter.begin_indent();


                 for (std::size_t i {1}; i < rule_values.size(); i++)
                 {
                   const auto &rule_value = rule_values[i];
                   if (rules.contains(rule_value))
                   {
                     {
                       TEMP emitter.begin_indent("if (tmp := (" + get_name(rule_value) + ".parse(t_input)))[1]:");
                       emitter.writeln("t_input, value[\"" + rule_value + "\"] = tmp");
                     }
                     {
                       TEMP emitter.begin_indent("else:");
                       emitter.writeln("break");
                     }
                   }
                   else if (rule_value == "ident")
                   {
                     {
                       TEMP emitter.begin_indent("if (tmp := (ident.parse(t_input)))[1]:");
                       emitter.writeln("t_input, value[\"" + rule_value + "\"] = tmp");
                     }
                     {
                       TEMP emitter.begin_indent("else:");
                       emitter.writeln("break");
                     }
                   }
                   else if (rule_value == "number")
                   {
                     {
                       TEMP emitter.begin_indent("if (tmp := (number.parse(t_input)))[1]:");
                       emitter.writeln("t_input, value[\"" + rule_value + "\"] = tmp");
                     }
                     {
                       TEMP emitter.begin_indent("else:");
                       emitter.writeln("break");
                     }
                   }
                   else 
                   {
                     {
                       TEMP emitter.begin_indent("if not t_input.pop_if(\"" + rule_value + "\"):");
                       emitter.writeln("break");
                     }
                   }
                 }
                 emitter.writeln("return t_input, " + name + "(_value=value)");
               }
               {
                 TEMP emitter.begin_indent();     
                 emitter.writeln("break");
               }
             }

           }
           emitter.writeln("return None, None");
         }
       }
     }
     emitter.newln();
    }
  }

  auto dump_driver(CodeEmitter& emitter) -> void
  {
    emitter.section("Driver");
    emitter.writeln("input = ListStream(\"\"\" foo : double ; int bar ; \"\"\")");

    {
      TEMP emitter.begin_indent("while input and not input.empty():");
      emitter.writeln("input, ast = " + this->name + "_" + this->last_rule + ".parse(input)");
      TEMP emitter.begin_indent("if ast is not None:");
      emitter.writeln("print(ast)");
    }
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
  std::string                                last_rule {};
};

}; // namespace macten

#undef CONCAT
#undef CONCAT_INNER
#undef TEMP 

#endif /*MACTEN_PROCEDURAL_MACRO_DEFINTION_H*/


