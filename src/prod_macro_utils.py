from dataclasses import dataclass
from typing import List
from numbers import Number


@dataclass
class ListStream:
    lst: List[str]

    def pop_if(self, match):
        v = self.lst[0] if self.lst else None

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
        self.lst = [(value.strip()) for value in s.split(" ")]
        self.lst = list(filter(lambda v: v != "", self.lst))

    def deepcopy(self):
        return ListStream(' '.join(self.lst))


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
            return input, ident(_value=input.pop(0))
        return input, None

    def out(self):
        return self._value


@dataclass
class number:
    _value: str

    @staticmethod
    def parse(input: ListStream):
        v = input.peek()
        if v.replace('.', '', 1).isdigit():
            return input, ident(_value=input.pop(0))
        return input, None

    def out(self):
        return self._value


def node_print(node):
    print(f"{type(node).__name__}")
    if isinstance(node._value, dict):
        size = len(node._value)
        for i, (key, child) in enumerate(node._value.items()):
            not_last = i < size - 1
            tmp = "├─" if not_last else "└─"
            prefix = "|  " if not_last else "   "
            print(f"{tmp}{type(child).__name__}")
            _node_print(child, prefix=prefix)
    else:
        _node_print(node._value, prefix="|  ")
    print()


def _node_print(node, prefix=""):
    if isinstance(node, str):
        print(f"{prefix}└─{node}")
        return

    if isinstance(node._value, dict):
        size = len(node._value)
        for i, (key, child) in enumerate(node._value.items()):
            not_last = i < size - 1
            tmp = "├─" if not_last else "└─"
            print(f"{prefix}{tmp}{type(child).__name__}")
            _node_print(child, prefix=prefix + ("|  " if not_last else "   "))
    else:
        print(f"{prefix}└─{type(node).__name__}")
        _node_print(node._value, prefix=prefix + "   ")
