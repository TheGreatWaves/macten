#include <iostream>
#define TREE std::cout << "GOT TREE\n";
#define TREAD std::cout << "GOT TREAD\n";

int main()
{
 const std::string name = "tread";
 const auto size = name.size();
 if (0 < size) switch (name[0]) {
   break; case 't': if (1 < size) switch (name[1]) {
     break; case 'r': if (2 < size) switch (name[2]) {
       break; case 'e': if (3 < size) switch (name[3]) {
         break; case 'a': if (4 < size) switch (name[4]) {
           break; case 'd': 
           {
             if (5 == size) TREAD
           }
         }
         break; case 'e': 
         {
           if (4 == size) TREE
         }
       }
     }
   }
 }
 
}

