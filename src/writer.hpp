#ifndef MACTEN_WRITER
#define MACTEN_WRITER

#include <string>
#include <string_view>
#include <fstream>

/**
 * The writer class is responsible for writing to files.
 */
struct Writer
{
 /*
  * Methods. 
  */

  /*
   * Ctor.
   */
  explicit Writer(std::string_view path) noexcept
  : m_path{ path }
  {
  }

  /**
   * Substitute.
   */
 void substitute(std::string_view target, std::string_view replacement)
 {
  std::ifstream infile;
  infile.open(m_path, std::ifstream::in);


  if (infile.good())
  {
   std::ofstream outfile("test.txt");
   std::string in;
   while (std::getline(infile, in))
   {
    if (in == target)
    {
     outfile << replacement;
    }
    else
    {
     outfile << in;
    }
    outfile << '\n';
   }
  }
 }
 

 /*
  * Members. 
  */

  std::string m_path;
};

#endif /* MACTEN_WRITER */