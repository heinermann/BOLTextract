#include <string>
#include <iostream>
#include <filesystem>

#include "../cxxopts/include/cxxopts.hpp"

#include "bolt.h"

// Configure the command line
void configure(cxxopts::Options& cmd) {
  cmd.add_options()
    ("i,input", "input file (.z64)", cxxopts::value<std::string>())
    ("o,output", "output directory", cxxopts::value<std::string>())
    ("h,help", "show help")
    ;

  cmd.parse_positional({ "input", "output" });
  cmd.positional_help("INPUT_FILE OUTPUT_DIR");
}

void show_help(cxxopts::Options &cmd) {
  std::cerr << cmd.help() << std::endl;
}

int main(int argc, const char **argv)
{
  cxxopts::Options cmd("bolt-extract", "Extract BOLT archive from a Starcraft z64 rom.");
  configure(cmd);

  auto parsed = cmd.parse(argc, argv);
  
  if (parsed.count("help")) {
    show_help(cmd);
    return 0;
  }

  if (!parsed.count("input") || !parsed.count("output")) {
    std::cerr << "Missing input file (z64).\n";
    show_help(cmd);
    return 1;
  }

  std::string input_file = parsed["input"].as<std::string>();
  std::string output_dir = parsed["output"].as<std::string>();

#ifdef _DEBUG
  std::cerr << "Attach debugger, then press enter..." << std::endl;
  std::cin.ignore();
#endif

  BOLT::extract_bolt(
    std::filesystem::absolute(input_file).string(),
    std::filesystem::absolute(output_dir).string()
  );

  return 0;
}
