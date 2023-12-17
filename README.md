# Macten
> [!NOTE]  
> _Macten_ is the short form of _macro extension_.

Macten is a basic transpiler/code generation tool which can power up any programming language by giving it the power of macros.

# Motivation
There are many rules which we adhere to when writing code, one of the most important one is [DRY](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself). Repeated code is almost always bad, not only does it add clutter to the codebase, it also increases the maintenance effort and hurts scalability. To combat this, different programming languages provide their own flavor of features which aims to help developers: C has the preprocessor directives, C++ provide you with metatemplate programming, python has decorators, etc. While each of these features are powerful in their own right, there is one which stands tall amongst all: lisp macros.
