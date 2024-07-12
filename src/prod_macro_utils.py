from dataclasses import dataclass
from typing import List, Any
from numbers import Number


@dataclass
class ListStream:
    lst: List[str]

    @staticmethod
    def from_string(input: str):
        head = 0
        tail = 0
        size = len(input)

        lst = []

        while tail < size:
            head = tail
            char = input[tail]

            # skip white space
            while char == ' ' or char == '\n':
                tail += 1
                head = tail
                if tail < size:
                    char = input[tail]
                else:
                    return ListStream(lst=lst)

            # Group identifier
            if char.isidentifier() or char == "_":
                tail += 1
                while tail < size and input[tail].isalnum() or input[tail] == "_":
                    tail += 1
                lst.append(input[head:tail])
            else:
                lst.append(input[tail])
                tail += 1

        return ListStream(lst=lst)

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

    @staticmethod
    def from_raw(s):
        lst = [(value.strip()) for value in s.split(" ")]
        lst = list(filter(lambda v: v != "", lst))
        return ListStream(lst=lst)

    def deepcopy(self):
        return ListStream(self.lst.copy())


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

class ASTHandler:
    def __init__(self):
        self.handler = dict()

    def add(self, name, handler):
        self.handler[name] = handler

    def get(self, name):
        if name in self.handler:
            return self.handler[name]
        else:
            print(f'[ Error: handler for "{name}" not found. ]')
            return NodeUtils.print

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


@dataclass
class Node:
    name: str
    _value: dict

    @staticmethod
    def new(name):
        return Node(name=name, _value={})

    def add_child(self, name, node):
        self._value[name] = node
        return self._value[name]

    def new_child(self, name):
        n = Node.new(name)
        self._value[name] = n
        return self._value[name]

    def get_child(self, name):
        return self._value.get(name)


# Collection of node utility functions
class NodeUtils:
    @staticmethod
    def get(node, name="", singular=False):
        if len(name) == 0:
            cursor = node
            while not isinstance(cursor, str):
                if isinstance(cursor._value, dict):
                    if len(cursor._value) == 1:
                        _, cursor = next(iter(cursor._value.items()))
                        continue
                    else:
                        return None
                return cursor._value
            return cursor
        elif singular:
            return NodeUtils.get(node._value.get(name))
        else:
            return node._value.get(name)

    # Note: only works with recursive rules.
    @staticmethod
    def into_list(node):
        # None if:
        # 1. Node is invalid
        # 2. Node does not have a dict as a value
        # 3. dict size is only 1
        if node is None or not isinstance(node._value, dict) or len(node._value) == 1:
            return None

        result = []
        node_name = type(node).__name__
        node_name = node_name[node_name.find('_') + 1:]
        size = len(node_name)

        def get_name(node):
            return type(node).__name__[-size:]

        current = node

        while get_name(current) == node_name:
            filtered = filter(lambda v: v != node_name, current._value.keys())

            entry = {}
            for value in filtered:
                entry[value] = current._value[value]
            result.append(entry)

            current = current._value.get(node_name)

        return result



    @staticmethod
    def print(node):

        if isinstance(node, Node):
            print(node.name)
        else:
            print(type(node).__name__)

        if isinstance(node._value, dict):
            size = len(node._value)
            for i, (key, child) in enumerate(node._value.items()):
                not_last = i < size - 1
                tmp = "├─" if not_last else "└─"
                prefix = "|  " if not_last else "   "

                if isinstance(child, Node):
                    print(f"{tmp}{child.name}")
                else:
                    print(f"{tmp}{type(child).__name__}")

                NodeUtils._node_print(child, prefix=prefix)
        else:
            NodeUtils._node_print(node._value)
        print()

    @staticmethod
    def _node_print(node, prefix=""):
        if isinstance(node, str):
            print(f"{prefix}└─{node}")
        elif node is None:
            print(f"{prefix}└─None")
        elif isinstance(node._value, dict):
            size = len(node._value)
            for i, (key, child) in enumerate(node._value.items()):
                not_last = i < size - 1
                tmp = "├─" if not_last else "└─"
                if isinstance(child, Node):
                    print(f"{prefix}{tmp}{child.name}")
                else:
                    print(f"{prefix}{tmp}{type(child).__name__}")

                NodeUtils._node_print(child, prefix=prefix + ("|  " if not_last else "   "))
        else:
            NodeUtils._node_print(node._value, prefix=prefix)

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

def init():
    global ctx
    ctx = ProceduralMacroContext()

    global handler
    handler = ASTHandler()
