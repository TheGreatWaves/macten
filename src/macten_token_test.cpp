#define DEBUG

#include "macten.hpp"

auto main() -> int 
{
 MactenWriter writer("../examples/cpp/switch.cpp", "throwaway.txt");
 writer.process();
}
