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

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include "multimap/internal/Check.hpp"
#include "multimap/internal/Generator.hpp"
#include "multimap/internal/System.hpp"
#include "multimap/Map.hpp"

namespace po = boost::program_options;

void CheckOption(const po::variables_map& variables, const char* option) {
  if (variables.count(option) == 0) {
    multimap::internal::Throw("Option '--%s' is missing.", option);
  }
}

void RunHelpCommand(const po::options_description& options) {
  std::cout << options << "\n";
}

void RunWriteCommand(const po::variables_map& variables) {
  CheckOption(variables, "to");
  CheckOption(variables, "nkeys");
  CheckOption(variables, "nvalues");

  multimap::Options options;
  options.verbose = true;
  options.error_if_exists = true;
  options.create_if_missing = true;
  multimap::Map map(variables["to"].as<std::string>(), options);

  const auto nkeys = variables["nkeys"].as<std::size_t>();
  const auto nvalues = variables["nvalues"].as<std::size_t>();
  const auto nvalues_total = nkeys * nvalues;

  const auto one_million = 1000000;
  const auto ten_million = 10 * one_million;
  multimap::internal::Generator generator(nkeys);
  for (std::size_t i = 0; i != nvalues_total;) {
    map.Put(generator.Generate(42), generator.Generate(23));
    if (++i % ten_million == 0) {
      const auto num_written = i / one_million;
      multimap::internal::System::Log() << num_written << "M values written\n";
    }
  }
  map.PrintProperties();
}
// time ./multimap-benchmarks --write --to /media/disk2/multimap/ --nkeys 100000 --nvalues 1000
// real 2m17.776s
// user 2m6.780s
// sys  0m1.876s
//
// time ./multimap-benchmarks --write --to /media/disk2/multimap/ --nkeys 100000 --nvalues 2000
// real 4m26.646s
// user 4m12.912s
// sys  0m3.980s

void RunReadCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");

  multimap::Options options;
  multimap::Map map(variables["from"].as<std::string>(), options);

  map.ForEachKey([&map](const multimap::Bytes& key) {
    auto iter = map.Get(key);
    for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
      assert(!iter.GetValue().empty());

      static std::size_t i = 0;
      static const auto one_million = 1000000;
      static const auto ten_million = 10 * one_million;
      if (++i % ten_million == 0) {
        const auto num_read = i / one_million;
        multimap::internal::System::Log() << num_read << "M values read\n";
      }
    }
  });
}
// time ./multimap-benchmarks --read --from /media/disk2/multimap (--nkeys 100000 --nvalues 1000)
// real 0m49.474s
// user 0m1.460s
// sys  0m3.024s
//
// time ./multimap-benchmarks --read --from /media/disk2/multimap (--nkeys 100000 --nvalues 2000) run #1
// real 74m22.456s
// user 0m5.856s
// sys  0m44.103s
//
// time ./multimap-benchmarks --read --from /media/disk2/multimap (--nkeys 100000 --nvalues 2000) run #2
// real 0m7.222s
// user 0m2.432s
// sys  0m4.656s
//
// time ./multimap-benchmarks --read --from /media/disk2/multimap (--nkeys 100000 --nvalues 2000) with aio
// real 41m48.499s
// user 0m14.069s
// sys  0m37.522s

void RunCopyCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");
  CheckOption(variables, "to");

  multimap::Copy(variables["from"].as<std::string>(),
                 variables["to"].as<std::string>());
}

// TODO Add options for block-size and sort.
// http://unix.stackexchange.com/questions/87908/how-do-you-empty-the-buffers-and-cache-on-a-linux-system
// ~/bin/linux-fincore --pages=false --summarize --only-cached /media/disk2/multimap/*
// sudo sh -c 'echo {1,2,3} >/proc/sys/vm/drop_caches'
int main(int argc, char* argv[]) {
  po::options_description commands_description("COMMANDS");
  commands_description.add_options()(
      "help", "Print this help message and exit.")(
      "write", "Write a Multimap to directory.")(
      "read", "Read a Multimap from directory.")(
      "copy", "Copy a Multimap from one directory to another.");

  po::options_description options_description("OPTIONS");
  options_description.add_options()(
      "from", po::value<std::string>(), "Directory to read a Multimap from.")(
      "to", po::value<std::string>(), "Directory to write a Multimap to.")(
      "nkeys", po::value<std::size_t>(), "Number of keys to put.")(
      "nvalues", po::value<std::size_t>(), "Number of values per key to put.");

  po::options_description cmdline_options;
  cmdline_options.add(commands_description).add(options_description);

  po::variables_map variables;
  po::store(po::parse_command_line(argc, argv, cmdline_options), variables);
  po::notify(variables);

  try {
    if (variables.count("help")) {
      RunHelpCommand(cmdline_options);
    } else if (variables.count("write")) {
      RunWriteCommand(variables);
    } else if (variables.count("read")) {
      RunReadCommand(variables);
    } else if (variables.count("copy")) {
      RunCopyCommand(variables);
    } else {
      std::cout << "Try with --help\n";
    }
  } catch (std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
