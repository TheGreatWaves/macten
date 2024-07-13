# USER IMPLEMENTATION - switch HANDLER

#=========#
# Imports #
#=========#

import macten
from macten import NodeUtils
from dataclasses import dataclass
from typing import Any


#==================#
# Register Handler #
#==================#

def add_handler():
    macten.handler.add("switch", handle)

#==================#
# Handler Function #
#==================#

def handle(ast):
    # TODO: Implementation of "switch" handler
    process(ast)

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
        print(f'const auto size = {varname}.size();')
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

    # t.print()
    # print()

    t.ast(target)
