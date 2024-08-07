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
 using ProceduralMacroRule = std::pair<std::vector<std::vector<std::string>>, bool>;

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
  auto set_name(const std::string& rule_name) -> void
  {
    this->name = rule_name;
  }

  /**
   * Create a new rule and return a reference to it.
   */
  auto create_rule(const std::string& rule_name) -> ProceduralMacroRule&
  {
   // Create an empty rule with the given name.
   // Note: This could potentially overwrite any 
   //
   // previously defined rule with the same name.
   this->rules[rule_name] = {};

   // Return the empty rule.
   return this->rules[rule_name];
  }

  auto dump_parse(CodeEmitter& emitter, const std::string& rule_name) const -> void
  {
    emitter.writeln("@staticmethod");
    TEMP emitter.begin_indent("def parse(input: ListStream, ast: Any):");
    emitter.writeln("return parse_fn(macten.ctx, \"" + rule_name + "\")(input, ast)");
  }

  auto dump_add_rules(CodeEmitter& emitter) const -> void
  {
    emitter.section("Rule Adder");
    TEMP emitter.begin_indent("def add_rules():");
    for (const auto& [_rule_name, rule] : this->rules)
    {
     const auto& [rule_definition, recursive] = rule;
     const auto get_name = [&](const std::string& _name) { return this->name + '_' + _name; };
     const auto _name = get_name(_rule_name);
     emitter.writeln("macten.ctx.add_rule(\"" + _name + "\", " + _name + ")");
    }
    const auto entry_rule = this->name + '_' + last_rule;
    emitter.writeln("macten.ctx.add_rule(\"" + this->name + "\", " + entry_rule + ")");
    emitter.newln();
  }
  
  /**
   * Dump python code for generating the rules.
   */
  auto dump_rules(CodeEmitter& emitter) const -> void 
  {
    for (const auto& [_rule_name, rule] : this->rules)
    {
     const auto& [rule_definition, recursive] = rule;
     const auto get_name = [&](const std::string& _name) { return this->name + "_" + _name; };
     const auto _name = get_name(_rule_name);
     {
       emitter.writeln("@dataclass");


       TEMP emitter.begin_indent("class " + _name + ":");

       emitter.writeln("_value: Any");
       emitter.newln();

       // Parse rule recursively.
       if (recursive)
       {
         dump_parse(emitter, _name);
         emitter.newln();
       }
      
       {
         const std::string function_name = recursive ? "_parse" : "parse";
         emitter.writeln("@staticmethod");
         TEMP emitter.begin_indent("def " + function_name + "(input: ListStream, ast: Any):");
         {
           {
             TEMP emitter.begin_indent("if input.empty():");
             emitter.writeln("return None, None");
           }

           bool can_be_empty = false;

           for (const auto& rule_values : rule_definition)
           {

             // The rule is optional.
             if (rule_values.empty())
             {
               can_be_empty = true;
               continue;
             }

             // Singular values.
             else if (rule_values.size() == 1)
             {
               const auto &rule_value = rule_values[0];
               {
                 {
                   TEMP emitter.begin_indent("while True:");
                   emitter.writeln("t_input = input.deepcopy()");
                   if (rules.contains(rule_value))
                   {
                    TEMP emitter.begin_indent("if (value := (" + get_name(rule_value) + ".parse(t_input, ast)))[1]:");
                    emitter.writeln("return value[0], " + _name + "(_value={'" + rule_value + "': value[1]})");
                   }
                   else if (rule_value == "ident")
                   {
                    TEMP emitter.begin_indent("if (value := (ident.parse(t_input))):");
                    emitter.writeln("return value[0], " + _name + "(_value=value[1])");
                   }
                   else if (rule_value == "number")
                   {
                    TEMP emitter.begin_indent("if (value := (number.parse(t_input))):");
                    emitter.writeln("return value[0], " + _name + "(_value=value[1])");
                   }
                   else 
                   {
                    if (rule_value.size() == 1)
                    {
                      TEMP emitter.begin_indent("if (value := (t_input.pop_if('" + rule_value + "'))):");
                      emitter.writeln("return t_input, " + _name + "(_value=value)");
                    }
                    else
                    {
                      TEMP emitter.begin_indent("if (value := (t_input.pop_if(\"" + rule_value + "\"))):");
                      emitter.writeln("return t_input, " + _name + "(_value=value)");
                    }
                   }
                   emitter.writeln("break");
                 }
               }
             }
             else
             {
               const auto &_rule_value = rule_values[0];
               {
                 TEMP emitter.begin_indent("while True:");
                 emitter.writeln("t_input = input.deepcopy()");
                 if (rules.contains(_rule_value))
                 {
                   const auto rule_name = get_name(_rule_value);

                   // Handle recursive self case
                  if (rule_name == _name)
                  {
                    TEMP emitter.begin_indent("if isinstance(ast, " + _name + "):");
                    emitter.writeln("value = {\"" + _rule_value + "\": ast}");
                  }
                  else 
                  {
                    TEMP emitter.begin_indent("if (value := (" + rule_name + ".parse(t_input, ast)))[1]:");
                    emitter.writeln("t_input, ast = value");
                    emitter.writeln("value = {\"" + _rule_value + "\": ast}");
                  }
                 }
                 else if (_rule_value == "ident")
                 {
                   TEMP emitter.begin_indent("if (value := (ident.parse(t_input)))[1]:");
                   emitter.writeln("t_input, ast = value");
                   emitter.writeln("value = {\"" + _rule_value + "\": ast}");
                 }
                 else if (_rule_value == "number")
                 {
                   TEMP emitter.begin_indent("if (value := (number.parse(t_input)))[1]:");
                   emitter.writeln("t_input, ast = value");
                   emitter.writeln("value = {\"" + _rule_value + "\": ast}");
                 }
                 else 
                 {
                  if (_rule_value.size() == 1)
                  {
                   TEMP emitter.begin_indent("if t_input.pop_if('" + _rule_value + "'):");
                   emitter.writeln("value = {}");
                  }
                  else
                  {
                   TEMP emitter.begin_indent("if t_input.pop_if(\"" + _rule_value + "\"):");
                   emitter.writeln("value = {}");
                  }
                 }
                 TEMP emitter.begin_indent();


                 for (std::size_t i {1}; i < rule_values.size(); i++)
                 {
                   const auto &rule_value = rule_values[i];
                   if (rules.contains(rule_value))
                   {
                     {
                       TEMP emitter.begin_indent("if (tmp := (" + get_name(rule_value) + ".parse(t_input, ast)))[1]:");
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
                     if (rule_value.size() == 1)
                     {
                       TEMP emitter.begin_indent("if not t_input.pop_if('" + rule_value + "'):");
                       emitter.writeln("break");
                     }
                     else
                     {
                       TEMP emitter.begin_indent("if not t_input.pop_if(\"" + rule_value + "\"):");
                       emitter.writeln("break");
                     }
                   }
                 }
                 emitter.writeln("return t_input, " + _name + "(_value=value)");
               }
               {
                 TEMP emitter.begin_indent();     
                 emitter.writeln("break");
               }
             }

           }
           if (can_be_empty)
           {
             emitter.writeln("return input, " + _name + "(_value=None)");
           }
           else
           {
             emitter.writeln("return None, None");
           }
         }
       }
     }
     emitter.newln();
    }
  }

  auto dump_driver(CodeEmitter& emitter) -> void
  {
    emitter.section("Driver");
    emitter.writeln("input = ListStream.from_string(\"\"\" \"\"\")");
    emitter.writeln("ast = None");
    {
      TEMP emitter.begin_indent("while input and not input.empty():");
      emitter.writeln("input, ast = " + this->name + "_" + this->last_rule + ".parse(input, ast)");
      {
        TEMP emitter.begin_indent("if ast is None:");
        emitter.writeln("print(\"Something went wrong!\")");
      }
      emitter.writeln("NodeUtils.print(ast)");
    }
  }

  /**
   * Dump python code which generates the procedural macro profile.
   */
  auto dump() const -> const std::string 
  {
    macten::CodeEmitter emitter{};

    emitter.comment("AUTO GENERATED CODE, DO NOT EDIT");
    emitter.section("Imports");
    emitter.writeln("import macten");
    emitter.writeln("from macten import ListStream, ProceduralMacroContext, ident, number, parse_fn, NodeUtils");
    emitter.writeln("from typing import Any");
    emitter.writeln("from dataclasses import dataclass");

    // Begin profile section.
    emitter.section("Profile: " + this->name);

    dump_rules(emitter);

    dump_add_rules(emitter);

    // For debugging. (Target a file later)
    return emitter.dump();
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


