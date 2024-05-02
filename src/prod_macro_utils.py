from dataclasses import dataclass
from typing import List
from numbers import Number


@dataclass
class ListStream:
    lst: List[str]

    def pop_if(self, match):
        v = self.lst[0]

        if v == match:
            self.pop()
            return True

        return False

    def at(self, idx):
        return self.lst[idx]

    def peek(self, idx=0):
        return self.lst[idx]

    def pop(self, idx=0):
        return self.lst.pop(idx)


class ProceduralMacroContext:
    def __init__(self):

        # Storage for all procedural macro rules.
        self.rules = {}

    def add_rule(self, name, rule):
        self.rules[name] = rule

    def get_rule(self, name):
        return self.rules[name]


@dataclass
class ident:
    lexeme: str

    def parse(s):
        v = s.peek()
        if isinstance(v, str):
            return ident(lexeme=s.pop(0))
        return None

    def out(self):
        return self.lexeme


@dataclass
class number:
    lexeme: str

    def parse(s):
        v = s.peek()
        if v.replace('.', '', 1).isdigit():
            return ident(lexeme=s.pop(0))
        return None

    def out(self):
        return self.lexeme
