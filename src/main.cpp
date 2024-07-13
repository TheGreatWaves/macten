#include <filesystem>
#include <fstream>

#define DEFUG
#include "macten.hpp"

auto print_help() -> void
{
 std::cout << "Usage: help | generate <path> | run <path> | clean" << '\n';
}

auto handle_generate(const std::vector<std::string>& command) -> void
{
 if (command.size() < 2)
 {
  std::cerr << "Expected source path" << '\n';
  return;
 }
 
 const auto file = command[1];

 macten::MactenWriter writer(file, file+".error");
 if (writer.generate())
  std::cerr << "Failed to generate procedural macro files" << '\n';
 else
  std::cout << "Procedural macro files generated" << '\n';
}

auto handle_clean() -> void
{
 std::filesystem::remove_all(".macten");
 std::cout << "Removed macten files" << '\n';
}

auto handle_run(const std::vector<std::string>& command) -> void
{
 if (command.size() < 2)
 {
  std::cerr << "Expected source path" << '\n';
  return;
 }

 const auto file = command[1];

 std::string dest = "";
 if (command.size() < 3)
 {
  std::filesystem::path path(file);
  std::stringstream path_ss{};
  path_ss << path.parent_path().string();
  path_ss << '/';
  path_ss << path.stem().string();
  path_ss << ".macten";
  path_ss << path.extension().string();
  dest = path_ss.str();
 }
 else
 {
  dest = command[2];
 }


 macten::MactenWriter writer(file, dest);
 if (writer.process())
  std::cout << "Successfully processed macros" << '\n';
 else
  std::cerr << "Failed to process macros" << '\n';

}

auto main(int argc, char* argv[]) -> int 
{
 std::vector<std::string> cli_args{};

 for (int i {1}; i < argc; i++)
  cli_args.push_back(std::string(argv[i]));

 if (cli_args.empty())
 {
  std::cerr << "Expected command, try 'help'" << '\n';
  return 1;
 }

 const auto& command = cli_args[0];

 if (command == "help") print_help();
 else if (command == "generate") handle_generate(cli_args);
 else if (command == "run") handle_run(cli_args);
 else if (command == "clean") handle_clean();
 else 
 {
  std::cerr << "Invalid command: '" << command << "', try 'help'" << '\n';
  return 1;
 }

 return 0;
}
