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

#include <iostream>
#include <boost/program_options.hpp>
#include "multimap/thirdparty/mt.hpp"
#include "multimap/Map.hpp"

const char* HELP     = "help";
const char* IMPORT   = "import";
const char* EXPORT   = "export";
const char* OPTIMIZE = "optimize";

const char* SOURCE   = "source";
const char* TARGET   = "target";
const char* CREATE   = "create";
const char* BS       = "bs";
const char* NSHARDS  = "nshards";

namespace po = boost::program_options;

void requireOption(const po::variables_map& arguments, const char* option) {
  if (arguments.count(option) == 0) {
    mt::throwRuntimeError2("Option '--%s' is missing.", option);
  }
}

void runHelpCommand(const po::options_description& descriptions) {
  std::cout << descriptions << "\n";
}

void runImportCommand(const po::variables_map& arguments) {
  requireOption(arguments, SOURCE);
  requireOption(arguments, TARGET);

  multimap::Options options;
  if (arguments.count(CREATE)) {
    options.create_if_missing = true;
    if (arguments.count(BS)) {
      options.block_size = arguments[BS].as<std::size_t>();
    }
    if (arguments.count(NSHARDS)) {
      options.num_shards = arguments[NSHARDS].as<std::size_t>();
    }
  }

  multimap::Map::importFromBase64(arguments[TARGET].as<std::string>(),
                                  arguments[SOURCE].as<std::string>(), options);
}

void runExportCommand(const po::variables_map& arguments) {
  requireOption(arguments, SOURCE);
  requireOption(arguments, TARGET);

  multimap::Map::exportToBase64(arguments[SOURCE].as<std::string>(),
                                arguments[TARGET].as<std::string>());
}

void runOptimizeCommand(const po::variables_map& arguments) {
  requireOption(arguments, SOURCE);
  requireOption(arguments, TARGET);

  multimap::Options options;
  if (arguments.count(BS)) {
    options.block_size = arguments[BS].as<std::size_t>();
  } else {
    options.block_size = 0; // Use same block_size as SOURCE.
  }
  if (arguments.count(NSHARDS)) {
    options.num_shards = arguments[NSHARDS].as<std::size_t>();
  } else {
    options.num_shards = 0; // Use same num_shards as SOURCE.
  }

  multimap::Map::optimize(arguments[SOURCE].as<std::string>(),
                          arguments[TARGET].as<std::string>(), options);
}

int main(int argc, const char* argv[]) {
  po::options_description commands_description("COMMANDS");
  commands_description.add_options()(
      HELP, "Print this help message and exit.")(
      IMPORT, "Import key-value pairs from Base64-encoded text files.")(
      EXPORT, "Export key-value pairs to a Base64-encoded text file.")(
      OPTIMIZE, "Rewrite an instance performing various optimizations.");

  multimap::Options options;
  po::options_description options_description("OPTIONS", 100);
  options_description.add_options()(
      SOURCE, po::value<std::string>(), "Directory or file used as input.")(
      TARGET, po::value<std::string>(), "Directory or file used as output.")(
      CREATE, "Create new instance if missing.")(
      BS, po::value<std::size_t>(),
        ("Block size to use for a new instance. "
         "Default is " + std::to_string(options.block_size) + '.').c_str())(
      NSHARDS, po::value<std::size_t>(),
        ("Number of shards to use for a new instance. "
         "Default is " + std::to_string(options.num_shards) + '.').c_str());

  po::options_description descriptions;
  descriptions.add(commands_description).add(options_description);

  po::variables_map arguments;
  po::store(po::parse_command_line(argc, argv, descriptions), arguments);
  po::notify(arguments);

  try {
    if (arguments.count(HELP)) {
      runHelpCommand(descriptions);
    } else if (arguments.count(IMPORT)) {
      runImportCommand(arguments);
    } else if (arguments.count(EXPORT)) {
      runExportCommand(arguments);
    } else if (arguments.count(OPTIMIZE)) {
      runOptimizeCommand(arguments);
    } else {
      std::cout << "Try with --" << HELP << '\n';
    }
  } catch (std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

  // Add Copyright notice
  // Add link to website for more infor about params.
}
