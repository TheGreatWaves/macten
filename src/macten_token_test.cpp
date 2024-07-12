#define DEBUG

#include "macten.hpp"

auto main() -> int 
{
 macten::MactenWriter writer("../examples/cpp/switch_throw.cpp", "throwaway.txt");
 writer.process();
}
