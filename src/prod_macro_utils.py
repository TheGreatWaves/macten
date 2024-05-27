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
        self.lst = s.split(" ")


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
    value: str

    def parse(s):
        v = s.peek()
        if isinstance(v, str):
            return ident(value=s.pop(0))
        return None

    def out(self):
        return self.value


@dataclass
class number:
    value: str

    def parse(s):
        v = s.peek()
        if v.replace('.', '', 1).isdigit():
            return ident(value=s.pop(0))
        return None

    def out(self):
        return self.value
