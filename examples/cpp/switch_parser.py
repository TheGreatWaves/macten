# AUTO GENERATED CODE, DO NOT EDIT

#=========#
# Imports #
#=========#

import macten
from macten import ListStream, ProceduralMacroContext, ident, number, parse_fn, NodeUtils
from typing import Any
from dataclasses import dataclass

#=================#
# Profile: switch #
#=================#

@dataclass
class switch_body:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if (value := (ident.parse(t_input))):
                return value[0], switch_body(_value=value[1])
            break
        return None, None

@dataclass
class switch_branch:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if t_input.pop_if("case"):
                value = {}
                if not t_input.pop_if('"'):
                    break
                if (tmp := (switch_case_name.parse(t_input, ast)))[1]:
                    t_input, value["case_name"] = tmp
                else:
                    break
                if not t_input.pop_if('"'):
                    break
                if not t_input.pop_if(':'):
                    break
                if not t_input.pop_if('{'):
                    break
                if (tmp := (switch_body.parse(t_input, ast)))[1]:
                    t_input, value["body"] = tmp
                else:
                    break
                if not t_input.pop_if('}'):
                    break
                return t_input, switch_branch(_value=value)
            break
        return None, None

@dataclass
class switch_branches:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        return parse_fn(macten.ctx, "switch_branches")(input, ast)

    @staticmethod
    def _parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if isinstance(ast, switch_branches):
                value = {"branches": ast}
                if (tmp := (switch_branch.parse(t_input, ast)))[1]:
                    t_input, value["branch"] = tmp
                else:
                    break
                return t_input, switch_branches(_value=value)
            break
        while True:
            t_input = input.deepcopy()
            if (value := (switch_branch.parse(t_input, ast)))[1]:
                return value[0], switch_branches(_value={'branch': value[1]})
            break
        return None, None

@dataclass
class switch_case_name:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if (value := (ident.parse(t_input))):
                return value[0], switch_case_name(_value=value[1])
            break
        return None, None

@dataclass
class switch_switch_str:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if t_input.pop_if("switch"):
                value = {}
                if (tmp := (switch_target.parse(t_input, ast)))[1]:
                    t_input, value["target"] = tmp
                else:
                    break
                if not t_input.pop_if('{'):
                    break
                if (tmp := (switch_branches.parse(t_input, ast)))[1]:
                    t_input, value["branches"] = tmp
                else:
                    break
                if not t_input.pop_if('}'):
                    break
                return t_input, switch_switch_str(_value=value)
            break
        return None, None

@dataclass
class switch_target:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        if input.empty():
            return None, None
        while True:
            t_input = input.deepcopy()
            if (value := (ident.parse(t_input))):
                return value[0], switch_target(_value=value[1])
            break
        return None, None


#============#
# Rule Adder #
#============#

def add_rules():
    macten.ctx.add_rule("switch_body", switch_body)
    macten.ctx.add_rule("switch_branch", switch_branch)
    macten.ctx.add_rule("switch_branches", switch_branches)
    macten.ctx.add_rule("switch_case_name", switch_case_name)
    macten.ctx.add_rule("switch_switch_str", switch_switch_str)
    macten.ctx.add_rule("switch_target", switch_target)
    macten.ctx.add_rule("switch", switch_switch_str)

