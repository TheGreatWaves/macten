from dataclasses import dataclass
from typing import List


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
