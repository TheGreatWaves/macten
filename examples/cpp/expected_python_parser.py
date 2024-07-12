# AUTO GENERATED CODE, DO NOT EDIT

#=========#
# Imports #
#=========#

from prod_macro_utils import ListStream, ProceduralMacroContext, ident, number, parse_fn, NodeUtils
from typing import Any
from dataclasses import dataclass

#======================#
# Structures / Storage #
#======================#

# STORAGE FOR ALL PROCEDURAL MACRO RULES
ctx = ProceduralMacroContext()

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

ctx.add_rule("switch_body", switch_body)

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

ctx.add_rule("switch_branch", switch_branch)

@dataclass
class switch_branches:
    _value: Any

    @staticmethod
    def parse(input: ListStream, ast: Any):
        return parse_fn(ctx, "switch_branches")(input, ast)

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

ctx.add_rule("switch_branches", switch_branches)

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

ctx.add_rule("switch_case_name", switch_case_name)

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

ctx.add_rule("switch_switch_str", switch_switch_str)

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

ctx.add_rule("switch_target", switch_target)


#============#
# Trie stuff #
#============#

@dataclass
class trie_node:
    children: dict
    data: Any
    leaf: bool

    @staticmethod
    def new():
        return trie_node(children={}, leaf=False, data=None)

@dataclass 
class trie:
    root: trie_node

    @staticmethod 
    def new():
        return trie(root=trie_node.new())

    def add_entry(self, word, data=None):
        if len(word) == 0:
            print('word is empty!')
            return

        current = self.root
        size = len(word)
        for idx in range(size):
            letter = word[idx]
            if letter not in current.children:
                current.children[letter] = trie_node.new()
            current.children[letter].leaf |= (idx == size - 1)
            if data:
                current.children[letter].data = data

            current = current.children[letter]

    @staticmethod
    def print_node(node, prefix=""):
        size = len(node.children)
        for i, (letter, child) in enumerate(node.children.items()):
            connector = "├─" if i < size - 1 else "└─"
            if node.children[letter].leaf:
                connector = connector[:-1]
                letter = f'[{letter}]'
            print(f'{prefix}{connector}{letter}')
            pref = "| " if i < size - 1 else "  "
            trie.print_node(child, prefix=prefix+pref)
        

    def print(self):
        print('trie')
        trie.print_node(self.root)

    @staticmethod
    def ast_helper(varname, layer, ln, space):
        print(f'if ({ln} < size) switch ({varname}[{ln}]) {"{"}')
        # print(("  " * ln) + '{')
        for n, child in layer.children.items():
            if not isinstance(child, str):
                print(f"{"  " * (space + 1)}break; case '{n}': ", end="")
                if not child.leaf:
                    trie.ast_helper(varname, child, ln+1, space+1)
                else:
                    print()
                    print(("  " * (space + 1)) + "{")
                    print(("  " * (space + 2)) + f"if ({ln+1} == size) {child.data}")

                    if len(child.children):
                        print(f"{("  " * (space + 2))}elif ", end="")
                        trie.ast_helper(varname, child, ln+1, space+2)

                    print(("  " * (space + 1)) + "}")

        print(("  " * space) + '}')


    def ast(self, varname):
        print(f'const auto size = {varname}.size()')
        trie.ast_helper(varname, self.root, ln=0, space=0)

def process(ast):
    target = NodeUtils.get(ast, 'target', singular=True)
    branches = NodeUtils.get(ast, "branches")
    branches = NodeUtils.into_list(branches)
    branches = [d['branch'] for d in branches]
    branches = [(NodeUtils.get(n, 'case_name', singular=True), NodeUtils.get(n, 'body', singular=True)) for n in branches]

    # Build trie structure.
    t = trie.new()

    for case, data in branches:
        t.add_entry(case, data)

    t.print()

    print()

    t.ast(target)

#========#
# Driver #
#========#

input = ListStream.from_string("""
switch name
{
    case "foo": { do_foo }
    case "bar": { do_bar }
    case "food": { do_food }
}
""")

ast = None
while input and not input.empty():
    input, ast = switch_switch_str.parse(input, ast)
    if ast is None:
        print("Something went wrong!")
    NodeUtils.print(ast)
    process(ast)
