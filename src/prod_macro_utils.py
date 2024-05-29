from dataclasses import dataclass
from typing import List, Any
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
    _value: Any

    @staticmethod
    def parse(input: ListStream):
        v = input.peek()
        if v.isidentifier():
            return input, {'ident': ident(_value=input.pop(0))}
        return input, None

    def out(self):
        return self._value


@dataclass
class number:
    _value: Any

    @staticmethod
    def parse(input: ListStream):
        v = input.peek()
        if v.replace('.', '', 1).isdigit():
            return input, {'number': ident(_value=input.pop(0))}
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
        _node_print(node._value)
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
        _node_print(node._value, prefix=prefix)

def parse_fn(ctx, name):
    def parse(input: ListStream, ast: Any):
        result_ast = None
        t_input = input.deepcopy()
        previous_size = len(t_input.lst)
        while True:
            result = ctx.get_rule(name)._parse(t_input, result_ast)
            tmp_t_input, tmp_ast = result
            current_size = len(tmp_t_input.lst) if tmp_t_input else previous_size
            if current_size == previous_size or tmp_ast is None:
                return t_input, result_ast
            t_input = tmp_t_input
            result_ast = tmp_ast
            previous_size = current_size
        return t_input, result_ast
    return parse

