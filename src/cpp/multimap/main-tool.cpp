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
#include <cinttypes>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/operations.hpp"

const auto HELP     = "help";
const auto STATS    = "stats";
const auto IMPORT   = "import";
const auto EXPORT   = "export";
const auto OPTIMIZE = "optimize";

const auto BS       = "--bs";
const auto CREATE   = "--create";
const auto NSHARDS  = "--nshards";

const auto COMMANDS = { HELP, STATS, IMPORT, EXPORT, OPTIMIZE };
const auto OPTIONS  = { BS, CREATE, NSHARDS };

struct CommandLine {
  struct Error : public std::runtime_error {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

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
  CommandLine cmd;
  const auto end = std::next(argv, argc);
  auto it = std::next(argv);

  using E = CommandLine::Error;
  mt::check<E>(it != end, "No COMMAND given");
  mt::check<E>(isCommand(*it), "Expected COMMAND when reading '%s'", *it);
  cmd.command = *it++;

  if (cmd.command != HELP) {
    mt::check<E>(it != end, "No MAP given");
    mt::check<E>(boost::filesystem::is_directory(*it),
                 "The directory '%s' does not exist", *it);
    cmd.map = *it++;
    if (cmd.command != STATS) {
      mt::check<E>(it != end, "No PATH given");
      mt::check<E>(boost::filesystem::exists(*it),
                   "The path '%s' does not exist", *it);
      cmd.path = *it++;
      while (it != end) {
        if (*it == CREATE) {
          cmd.options[*it++];
          continue;
        }
        if (*it == BS) {
          const auto option = *it++;
          mt::check<E>(it != end, "No value given for '%s'", option);
          cmd.options[option] = *it;
          continue;
        }
        if (*it == NSHARDS) {
          const auto option = *it++;
          mt::check<E>(it != end, "No value given for '%s'", option);
          cmd.options[option] = *it;
          continue;
        }
        mt::fail<E>("Expected option when reading '%s'", *it);
      }
    }
  }
  return cmd;
}

multimap::Options initOptions(const CommandLine& cmd) {
  multimap::Options options;
  options.create_if_missing = cmd.options.count(CREATE);
  if (cmd.options.count(BS)) {
    options.block_size = std::stoul(cmd.options.at(BS));
  }
  if (cmd.options.count(NSHARDS)) {
    options.num_shards = std::stoul(cmd.options.at(NSHARDS));
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
    "\n  %-9s NUM  Number of shards to use for a new instance. Default is %lu."
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

void runStatsCommand(const CommandLine& cmd) {
  const auto names = multimap::internal::Table::Stats::names();
  const int name_width =
      std::max_element(names.begin(), names.end(),
                       [](const std::string& a, const std::string& b) {
                         return a.size() < b.size();
                       })->size();

  const auto stats = multimap::stats(cmd.map);
  const auto totals = multimap::internal::Table::Stats::total(stats).toVector();
  const int value_width =
      std::to_string(*std::max_element(totals.begin(), totals.end())).size();

  const auto stars = [](double value, double max) {
    MT_REQUIRE_LE(value, max);
    return value != 0 ? std::string(std::ceil(30 * value / max), '*') : "";
  };

  const auto max = multimap::internal::Table::Stats::max(stats).toVector();
  for (std::size_t i = 0; i != stats.size(); ++i) {
    const auto stat = stats[i].toVector();
    std::printf("SHARD #%" PRIuMAX "\n", i);
    for (std::size_t j = 0; j != stat.size(); ++j) {
      std::printf("%-*s %-*" PRIu64 " %s\n", name_width, names[j].c_str(),
                  value_width, stat[j], stars(stat[j], max[j]).c_str());
    }
    std::printf("\n");
  }

  std::printf("TOTAL\n");
  for (std::size_t i = 0; i != totals.size(); ++i) {
    std::printf("%-*s %-*" PRIu64 "\n", name_width, names[i].c_str(),
                value_width, totals[i]);
  }
}

void runImportCommand(const CommandLine& cmd) {
  const auto options = initOptions(cmd);
  multimap::importFromBase64(cmd.map, cmd.path, options);
}

void runExportCommand(const CommandLine& cmd) {
  multimap::exportToBase64(cmd.map, cmd.path);
}

void runOptimizeCommand(const CommandLine& cmd) {
  const auto options = initOptions(cmd);
  multimap::optimize(cmd.map, cmd.path, options);
}

int main(int argc, const char** argv) {
  if (argc < 2) {
    runHelpCommand(*argv);
    return EXIT_FAILURE;
  }

  try {
    const auto cmd = parseCommandLine(argc, argv);

    if (cmd.command == HELP) {
      runHelpCommand(*argv);
      return EXIT_SUCCESS;
    }

    if (cmd.command == STATS) {
      runStatsCommand(cmd);
      return EXIT_SUCCESS;
    }

    if (cmd.command == IMPORT) {
      runImportCommand(cmd);
      return EXIT_SUCCESS;
    }

    if (cmd.command == EXPORT) {
      runExportCommand(cmd);
      return EXIT_SUCCESS;
    }

    if (cmd.command == OPTIMIZE) {
      runOptimizeCommand(cmd);
      return EXIT_SUCCESS;
    }

  } catch (CommandLine::Error& error) {
    std::cerr << "Invalid command line: " << error.what()
              << "\nTry " << *argv << ' ' << HELP << std::endl;

  } catch (std::exception& error) {
    std::cerr << error.what() << std::endl;
  }

  return EXIT_FAILURE;
}
