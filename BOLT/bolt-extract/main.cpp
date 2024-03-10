#include <string>
#include <iostream>
#include <filesystem>

#include "../cxxopts/include/cxxopts.hpp"

#include "bolt.h"

// Configure the command line
void configure(cxxopts::Options& cmd) {
  cmd.add_options()
    ("i,input", "input file", cxxopts::value<std::string>())
    ("b,big", "Use Big Endian byte order (N64, CD-i)")
    ("a,algo", "Choose algorithm to use.", cxxopts::value<std::string>()->default_value(""), "cdi|dos|n64|gba|win")
    ("o,output", "output directory (optional, defaults to input file's directory)", cxxopts::value<std::string>())
    ("h,help", "show help")
    ;

  cmd.parse_positional({ "input", "output" });
  cmd.positional_help("INPUT_FILE [OUTPUT_DIR]");
}

void show_help(cxxopts::Options &cmd) {
  std::cerr << cmd.help() << std::endl;
}

void check_debugger() {
#ifdef _DEBUG
  std::cerr << "Attach debugger, then press enter..." << std::endl;
  std::cin.ignore();
#endif
}


std::map<std::string, BOLT::algorithm_t> algorithm_mappings = {
  {"cdi", BOLT::algorithm_t::CDI},
  {"dos", BOLT::algorithm_t::DOS},
  {"msdos", BOLT::algorithm_t::DOS},
  {"n64", BOLT::algorithm_t::N64},
  {"gba", BOLT::algorithm_t::N64},
  {"z64", BOLT::algorithm_t::N64},
  {"win", BOLT::algorithm_t::WIN},
  {"windows", BOLT::algorithm_t::WIN},
};

BOLT::algorithm_t determine_algorithm(const std::filesystem::path &input_file, std::string algorithm) {
  if (algorithm.empty()) {
    algorithm = input_file.extension().string();
    algorithm.erase(0, 1);
  }

  if (algorithm_mappings.contains(algorithm)) {
    return algorithm_mappings.at(algorithm);
  }
  return BOLT::algorithm_t::UNKNOWN;
}

int main(int argc, const char **argv)
{
  cxxopts::Options cmd("bolt-extract", "Extract Mass Media's BOLT archive from binaries.");
  configure(cmd);

  auto parsed = cmd.parse(argc, argv);
  
  if (parsed.count("help")) {
    show_help(cmd);
    return 0;
  }

  if (!parsed.count("input")) {
    std::cerr << "Missing input file.\n";
    show_help(cmd);
    return 1;
  }

  std::string input_file = parsed["input"].as<std::string>();
  std::filesystem::path input_path = std::filesystem::absolute(input_file);

  std::filesystem::path output_path = input_path.parent_path() / input_path.stem();
  if (parsed.count("output")) {
    output_path = std::filesystem::absolute(parsed["output"].as<std::string>());
  }
  
  BOLT::g_big_endian = parsed["big"].as<bool>();

  check_debugger();

  BOLT::algorithm_t algorithm = determine_algorithm(input_path, parsed["algo"].as<std::string>());
  if (algorithm == BOLT::algorithm_t::UNKNOWN) {
    std::cerr << "Please choose a supported algorithm.\n";
    show_help(cmd);
    return 1;
  }

  BOLT::extract_bolt(input_path, output_path, algorithm);
  return 0;
}
