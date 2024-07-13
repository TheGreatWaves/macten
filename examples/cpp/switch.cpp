defmacten_proc switch {
  case_name { ident }
  body { ident }
  branch { case "case_name": { body } }
  branches { branches branch } | { branch }
  target { ident }
  switch_str { switch target { branches } }
}

#include <iostream>
#define TREE std::cout << "GOT TREE\n";
#define TREAD std::cout << "GOT TREAD\n";

int main()
{
 const std::string name = "tread";
 switch![
 switch name
 {
     case "tree": { TREE }
     case "tread": { TREAD }
 }
 ]
}

