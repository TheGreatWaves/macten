#define DEBUG

#include "macten.hpp"

auto main() -> int 
{
 MactenWriter writer("../examples/python/basic_declarative_macro.py", "throwaway.txt");
 writer.process();
}