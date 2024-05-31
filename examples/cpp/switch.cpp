defmacten_proc switch {
 case_name { ident }
 body { ident }
 branch { case "case_name" { body } }
 branches { branches branch } | { branch }
 target_string { ident }
 target_ident { "target_string" }
 target { target_ident } | { target_string }
 match { switch target { branches } }
}
//
// Source code.
//
#include <iostream>


auto main() -> int
{
 switch![
  switch (name)
  {
   case "Mai": { stmt1 }
   case "Dan": { stmt2 }
  }
 ]
}
