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

# Advanced Macros
Inspired by `Nim`, `Lisp` and `Rust`, advanced macros allow you to directly mutate the ast. Of course since `Macten` is generic, language configurations for the parser will have to be user defined.

Advanced macros will work in 3 steps:
1. An abstract syntax tree (AST) is generated from the macro body according to user defined parser rules and constructs.
2. The AST is mutated into the form which the user specifies.
3. The AST is translated back into code and pasted.

The abstract syntax tree can be generated using user defined parsing rules in the form of *images*. Each language could have their own *image* which is basically a description of the layout of different constructs. 

# Parse Rules
todo

# AST Facility
todo

# Translation
todo
