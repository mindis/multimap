// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdio>
#include <iostream>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/operations.hpp"

const char* HELP     = "help";
const char* STATS    = "stats";
const char* IMPORT   = "import";
const char* EXPORT   = "export";
const char* OPTIMIZE = "optimize";

const char* BS       = "--bs";
const char* CREATE   = "--create";
const char* NSHARDS  = "--nshards";

const auto COMMANDS  = { HELP, STATS, IMPORT, EXPORT, OPTIMIZE };
const auto OPTIONS   = { BS, CREATE, NSHARDS };

struct CommandLine {
  std::string command, map, path;
  std::map<std::string, std::string> options;
};

bool isCommand(const std::string& argument) {
  return std::find(std::begin(COMMANDS), std::end(COMMANDS), argument) !=
         std::end(COMMANDS);
}

bool isOption(const std::string& argument) {
  return std::find(std::begin(OPTIONS), std::end(OPTIONS), argument) !=
         std::end(OPTIONS);
}

CommandLine parseCommandLine(int argc, const char** argv) {
  CommandLine cmd_line;
  const auto end = std::next(argv, argc);
  auto it = std::next(argv);
  if (it != end) {
    mt::check(isCommand(*it), "Expected COMMAND when reading '%s'.", *it);
    cmd_line.command = *it++;
    if (it != end) {
      mt::check(!isOption(*it), "Expected MAP when reading '%s'.", *it);
      cmd_line.map = *it++;
      if (it != end) {
        if (!isOption(*it)) {
          cmd_line.path = *it++;
        }
        while (it != end) {
          if (*it == CREATE) {
            cmd_line.options[*it++];
            continue;
          }
          if (*it == BS) {
            const auto option = *it++;
            mt::check(it != end, "Missing argument for '%s'.", option);
            cmd_line.options[option] = *it;
            continue;
          }
          if (*it == NSHARDS) {
            const auto option = *it++;
            mt::check(it != end, "Missing argument for '%s'.", option);
            cmd_line.options[option] = *it;
            continue;
          }
          mt::failFormat("Expected option when reading '%s'.", *it);
        }
      }
    }
  }

  return cmd_line;
}

multimap::Options initOptions(const CommandLine& cmd_line) {
  multimap::Options options;
  options.create_if_missing = cmd_line.options.count(CREATE);
  if (cmd_line.options.count(BS)) {
    options.block_size = std::stoul(cmd_line.options.at(BS));
  }
  if (cmd_line.options.count(NSHARDS)) {
    options.num_shards = std::stoul(cmd_line.options.at(NSHARDS));
  }
  return options;
}

void runHelpCommand(const char* tool_name) {
  const multimap::Options default_options;
  std::printf(
      "USAGE\n"
      "\n  %s COMMAND MAP [PATH] [OPTIONS]"
      "\n\nCOMMANDS\n"
      "\n  %-10s     Print this help message and exit."
      "\n  %-10s     Print statistics about an instance."
      "\n  %-10s     Import key-value pairs from Base64-encoded text files."
      "\n  %-10s     Export key-value pairs to a Base64-encoded text file."
      "\n  %-10s     Rewrite an instance performing various optimizations."
      "\n\nOPTIONS\n"
      "\n  %-9s      Create a new instance if missing when importing data."
      "\n  %-9s NUM  Block size to use for a new instance. Default is %lu."
      "\n  %-9s NUM  Number of shards to use for a new instance. Default is "
      "%lu."
      "\n\nEXAMPLES\n"
      "\n  %s %-8s path/to/map"
      "\n  %s %-8s path/to/map path/to/input"
      "\n  %s %-8s path/to/map path/to/input/base64.csv"
      "\n  %s %-8s path/to/map path/to/input/base64.csv %s"
      "\n  %s %-8s path/to/map path/to/output/base64.csv"
      "\n  %s %-8s path/to/map path/to/output"
      "\n  %s %-8s path/to/map path/to/output %s 128"
      "\n  %s %-8s path/to/map path/to/output %s 46"
      "\n  %s %-8s path/to/map path/to/output %s 46 %s 128"
      "\n\n"
      "\nCopyright (C) 2015 Martin Trenkmann"
      "\n<http://multimap.io>\n",
      tool_name,
      HELP,
      STATS,
      IMPORT,
      EXPORT,
      OPTIMIZE,
      CREATE,
      BS, default_options.block_size,
      NSHARDS, default_options.num_shards,
      tool_name, STATS,
      tool_name, IMPORT,
      tool_name, IMPORT,
      tool_name, IMPORT, CREATE,
      tool_name, EXPORT,
      tool_name, OPTIMIZE,
      tool_name, OPTIMIZE, BS,
      tool_name, OPTIMIZE, NSHARDS,
      tool_name, OPTIMIZE, NSHARDS, BS);
}

void runStatsCommand(const CommandLine& cmd_line) {
//  std::cout << multimap::stats(cmd_line.map) << std::endl;
}

void runImportCommand(const CommandLine& cmd_line) {
  const auto options = initOptions(cmd_line);
  multimap::importFromBase64(cmd_line.map, cmd_line.path, options);
}

void runExportCommand(const CommandLine& cmd_line) {
  multimap::exportToBase64(cmd_line.map, cmd_line.path);
}

void runOptimizeCommand(const CommandLine& cmd_line) {
  const auto options = initOptions(cmd_line);
  multimap::optimize(cmd_line.map, cmd_line.path, options);
}

int main(int argc, const char** argv) {
  if (argc < 2) {
    runHelpCommand(*argv);
    return EXIT_FAILURE;
  }

  CommandLine cmd_line;
  try {
    cmd_line = parseCommandLine(argc, argv);
  } catch (std::exception& error) {
    std::cerr << "Invalid command line: " << error.what()
              << "\nTry " << *argv << ' ' << HELP << std::endl;
  }

  try {
    if (cmd_line.command == HELP) {
      runHelpCommand(*argv);
      return EXIT_SUCCESS;
    }

    if (cmd_line.command == STATS) {
      runStatsCommand(cmd_line);
      return EXIT_SUCCESS;
    }

    if (cmd_line.command == IMPORT) {
      runImportCommand(cmd_line);
      return EXIT_SUCCESS;
    }

    if (cmd_line.command == EXPORT) {
      runExportCommand(cmd_line);
      return EXIT_SUCCESS;
    }

    if (cmd_line.command == OPTIMIZE) {
      runOptimizeCommand(cmd_line);
      return EXIT_SUCCESS;
    }

  } catch (std::exception& error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  }
}
