from dataclasses import dataclass
from typing import List
from numbers import Number


@dataclass
class ListStream:
    lst: List[str]

    def pop_if(self, match):
        v = self.lst[0]

        if v == match:
            return self.pop()

        return None

    def at(self, idx):
        return self.lst[idx]

    def peek(self, idx=0):
        return self.lst[idx]

    def pop(self, idx=0):
        return self.lst.pop(idx)

    def empty(self):
        return len(self.lst) == 0

    def __init__(self, s):
        self.lst = [value.strip() for value in s.split(" ")]


class ProceduralMacroContext:
    def __init__(self):
        # Storage for all procedural macro rules.
        self.rules = {}
        self.known = set()

        self.add_rule("ident", ident)
        self.add_rule("number", number)

    def add_rule(self, name, rule):
        self.rules[name] = rule

    def get_rule(self, name):
        return self.rules[name]


@dataclass
class ident:
    _value: str

    @staticmethod
    def parse(input: ListStream):
        v = input.peek()
        if isinstance(v, str):
            return ident(_value=input.pop(0))
        return None

    def out(self):
        return self._value


@dataclass
class number:
    _value: str

    @staticmethod
    def parse(input: ListStream):
        v = input.peek()
        if v.replace('.', '', 1).isdigit():
            return ident(_value=input.pop(0))
        return None

    def out(self):
        return self._value


def node_print(node):
    _node_print(node)
    print()


def _node_print(node):
    if isinstance(node, str):
        print(node, end=" ")
    elif isinstance(node, list):
        for child in node:
            _node_print(child)
    else:
        _node_print(node._value)
