# Macten
> [!NOTE]  
> _Macten_ is the short form of _macro extension_.

Macten is a basic transpiler/code generation tool which can power up any programming language by giving it the power of macros.

# Motivation
There are many rules which we adhere to when writing code, one of the most important one is [DRY](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself). Repeated code is almost always bad, not only does it add clutter to the codebase, it also increases the maintenance effort and hurts scalability. To combat this, different programming languages provide their own flavor of features which aims to help developers: C has the preprocessor directives, C++ provide you with metatemplate programming, python has decorators, etc. While each of these features are powerful in their own right, there is one which stands tall amongst all: lisp macros.

# Declarative macro
A `declarative macro` is the simplest type of macro, it is essentially just string substitution. Thanks to their simplicity, declarative macros are easy to write and simple to use. I was first introduced to them in C, in the form of preprocessor directives. They could be used like this:
```c
#define join(a,b) a ## b
// ...
int JOIN(var, 1) = 5; // Exapnds to `int var1 = 5`
```
The problem with the design of the preprocessor directives is that defining a multiline macro is not supported. You would need to end your lines with a trailing slash like this:
```c
#define add_print(a, b) \
 do {\
    int c = a + b; \
    printf("sum: %d\n", c); \
 }\
 while (0) 
```
This is less than ideal. So instead of using this design, we will take inspiration from a newer language which has a much better design for declarative macros: [rust](https://doc.rust-lang.org/book/ch19-06-macros.html).
Here is how our macro may be defined:
```rust
macro_rules! add_print {
    ($a:expr,$b:expr) => {
        {
          let c = $a + $b;
          println!("{}", c);
        }
    }
}
```
As you can see rust provides a much better support for multi line macros. It's also good to note that rust provides a clear distinction between function calls and macro calls by using the `!` operator (We will also use this design).

A macten declarative marco can be defined using the `defmacten_dec` keyword.

Python example:

```py
defmacten_dec list {
    () => {
        (l := list(),
         l.append(1),
         l.append(2),
         l.append(3),
         l
        )[-1]
    }
}

if __name__ == "__main__":
    print(list![])
```
Turns into:
```py
if __name__ == "__main__":
    print((l := list(),
         l.append(1),
         l.append(2),
         l.append(3),
         l
        )[-1])
```

# Variadic Arguments

We can denote variadic arugments using the following syntax: `$($argname)`.
```
defmacten_dec min {
    ($x, $($y,)) => {min($x, min![$y])}
    ($x,) => {$x}
}
```
Note that this parameter type expects an input which looks something like: `min![1, 2, 3, 4, 5,]`. The trailing comma is necessary and there is no way to avoid it. 

# Expansion Order
The expansion order of macros can result in some very surprising behaviors. The following example illustrates an example which might seem counter intuitive.
```py
defmacten_dec min {
    ($x, $($y,)) => {min($x, min![$y])}
    ($x,) => {$x}
}

defmacten_dec list {
    () => {1,2,3,4,5,}
}

# Case 1
min![list![]] 

# Case 2
min![(list![])]
```

Case 1 will not work because parenthesis is required for invoking the macro. So in this case, we will instead match against the tokens `"list"`, `"!"`, `"["`, `"]"`.

Case 2 will not work because the expansion of `list![]` will be captured as one collective token. Instead of processing each tokens from the expansion, it will instead match a single token (with the lexeme being the expansion of `list![]`).

The solution is simply to use a caller helper macro function.

```py
defmacten_dec caller {
  ($m, $a) => {$m![$a]}
}

caller![min, (list![])]
# Result: min(1, min(2, min(3, 4)))
```

By utilizing a caller, we are able to pass in the expansion of `list![]` as a token stream to `min![]` rather it being a single token.

# DSL
A domain specific language, DSL for short, is a small language embedded in another. This behavior can be imitated quite nicely using the macro system.

```py
defmacten_dec command {
 (eval $a;) => {$a}
 (eval $a; $(eval $b;)) => {
  command![eval ($a);]
  command![$b]
 }
}

command![
 eval (3*3);
 eval (2+3);
]
```

# Advanced Macros
Inspired by `Nim`, `Lisp` and `Rust`, advanced macros allow you to directly mutate the ast. Of course since `Macten` is generic, language configurations for the parser will have to be user defined.

Advanced macros will work in 3 steps:
1. An abstract syntax tree (AST) is generated from the macro body according to user defined parser rules and constructs.
2. The AST is mutated into the form which the user specifies.
3. The AST is translated back into code and pasted.

The abstract syntax tree can be generated using user defined parsing rules in the form of *images*. Each language could have their own *image* which is basically a description of the layout of different constructs. 

# Parse Rules
We can take some inspiration from `lex` and `yacc`. The user can define parse rules by defining what an expression looks like.

For example, we could define a variable declaration statement like this:
```
defmacten_proc declaration {
    varname { ident }
    type { string } | { int }
    declaration { varname: type; }
}
```

Here, `ident` is a special built-in rule for capturing any lexeme which qualifies as an identifier.

So given an input which looks like this:
```rust
foo: int;
```
We can retrive the following AST:
```
declaration
├── type
│   └── int
└── varname
    └── ident
        └── foo
```

# Recursive Rules
Rules can be recursively parsed.
```
defmacten_proc declaration {
    varname { ident }
    type { string } | { int }
    declaration { varname: type; }
    declarations { declarations declaration } | { declaration }
}
```
Meaning this:
```rust
foo1: int;
foo2: int;
```
Gets parsed into:
```
declarations
├── declaration
│    ├── type
│    │    └── int
│    └── varname
│         └── ident
│              └── foo1
└── declarations
     └── declaration
         ├── type
         │    └── int
         └── varname
              └── ident
                   └── foo2
```
# Helper Library
A basic library is included for dealing with ASTs. For example, given `declarations`, we can collect all of the `declaration` nodes in an array using `NodeUtils.into_list`:
```py
ast = declaration_declarations.parse(input)

# NodeUtils.into_list simply collects all children except one with the same name
# as the parent node. So here, we simply collect: 
# [{'declaration': declaration_declaration(...), ...}]
declarations = NodeUtils.into_list(ast)

# now we can extract the value from the list of dicts
# [declaration_declaration(...), ...]
declarations = [d['declaration'] for d in declarations]
```

To retrieve a child node, use `NodeUtils.get(parent, child_name, singular?)`. When singular is specified to be true, the deepest single value will be returned. This only works if the specified child node of the parent only has one child, and this remains true all the way down the tree.

An example illustrates this best:
```py
# parse `declaration` (not plural)
input = ListStream.from_string("foo: int;")
ast = declaration_declaration.parse(input)

# returns "foo" (string)
varname = NodeUtils.get(ast, 'varname', singular=True)

# returns declaration_varname(...) (node)
varname_node = NodeUtils.get(ast, 'varname')
```
