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
  CheckOption(variables, "bs");

  multimap::Options options;
  options.verbose = true;
  options.error_if_exists = true;
  options.create_if_missing = true;
  options.block_size = variables["bs"].as<std::size_t>();
  multimap::Map map(variables["to"].as<std::string>(), options);

  const auto nkeys = variables["nkeys"].as<std::size_t>();
  const auto nvalues = variables["nvalues"].as<std::size_t>();
  const auto nvalues_total = nkeys * nvalues;

  const auto one_million = 1000000;
  const auto ten_million = 10 * one_million;
  auto gen = multimap::internal::SequenceGenerator::New();
  for (std::size_t i = 0; i != nvalues_total;) {
    map.Put(std::to_string(i % nkeys), gen->Generate(100));
    if (++i % ten_million == 0) {
      const auto num_written = i / one_million;
      multimap::internal::System::Log() << num_written << "M values written\n";
    }
  }
}
// time ./benchmarks --write --to /media/disk2/multimap/ --nkeys 100000
// --nvalues 1000
// real 2m17.776s
// user 2m6.780s
// sys  0m1.876s
//
// time ./benchmarks --write --to /media/disk2/multimap/ --nkeys 100000
// --nvalues 2000
// real 4m26.646s
// user 4m12.912s
// sys  0m3.980s
//
// time ./benchmarks --write --to /media/disk2/multimap/ --nkeys 100000 --nvalues 2000 (block size 4096)
// real 4m9.552s
// user 4m0.355s
// sys  0m2.496s
//
// time ./benchmarks --write --to /media/disk2/multimap/ --nkeys 100000 --nvalues 2000 (block size 4096 incl superblock)
// real 4m18.683s
// user 4m13.120s
// sys  0m2.988s
//
// http://leveldb.googlecode.com/svn/tags/1.17/doc/benchmark.html
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 1000000 --nvalues 1 (block size 1024)
// real 0m3.341s
// user 0m2.388s
// sys  0m0.780s
//
// AFTER USING MMAP
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 100000 --nvalues 1000 --bs 512
// real 1m42.164s
// user 1m33.414s
// sys  0m8.373s
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 100000 --nvalues 2000 --bs 512
// real 3m29.256s
// user 3m9.988s
// sys  0m15.693s
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 100000 --nvalues 2000 --bs 4096
// real 3m9.738s
// user 3m3.219s
// sys  0m5.420s
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 1000000 --nvalues 1 --bs 1024
// real 0m7.047s
// user 0m2.600s
// sys  0m2.132s
//
// time ./benchmarks --write --to /media/disk1/multimap/ --nkeys 1000000 --nvalues 1 --bs 128
// real 0m3.146s
// user 0m1.992s
// sys  0m1.140s

void RunReadCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");
  CheckOption(variables, "bs");

  multimap::Options options;
  options.block_size = variables["bs"].as<std::size_t>();
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
// time ./benchmarks --read --from /media/disk2/multimap (--nkeys
// 100000 --nvalues 1000)
// real 0m49.474s
// user 0m1.460s
// sys  0m3.024s
//
// time ./benchmarks --read --from /media/disk2/multimap (--nkeys
// 100000 --nvalues 2000) run #1
// real 74m22.456s
// user 0m5.856s
// sys  0m44.103s
//
// time ./benchmarks --read --from /media/disk2/multimap (--nkeys
// 100000 --nvalues 2000) run #2
// real 0m7.222s
// user 0m2.432s
// sys  0m4.656s
//
// time ./benchmarks --read --from /media/disk2/multimap (--nkeys
// 100000 --nvalues 2000) with aio
// real 41m48.499s
// user 0m14.069s
// sys  0m37.522s
//
// time ./benchmarks --read --from /media/disk2/multimap/ (--nkeys 100000 --nvalues 2000) (block size 4096)
// real 30m34.760s
// user 0m8.573s
// sys  0m22.769s
//
// time ./benchmarks --read --from /media/disk2/multimap/ (--nkeys 100000 --nvalues 2000) (block size 4096 incl superblock)
// real 48m3.614s
// user 0m9.593s
// sys  0m33.486s
//
// http://leveldb.googlecode.com/svn/tags/1.17/doc/benchmark.html
//
// time ./benchmarks --read --from /media/disk1/multimap/ [--nkeys 1000000 --nvalues 1 (block size 1024)]
// real 2m51.915s
// user 0m8.229s
// sys  0m10.305s
//
// AFTER USING MMAP
//
// time ./benchmarks --read --from /media/disk1/multimap/ --bs 512 (--nkeys 100000 --nvalues 1000)
// real 3m4.845s
// user 0m2.308s
// sys  0m2.740s
//
// time ./benchmarks --read --from /media/disk1/multimap/ --bs 512 (--nkeys 100000 --nvalues 2000) run #1
// real 6m8.760s
// user 0m4.332s
// sys  0m5.552s
//
// time ./benchmarks --read --from /media/disk1/multimap/ --bs 512 (--nkeys 100000 --nvalues 2000) run #2
// real 0m4.880s
// user 0m3.928s
// sys  0m0.932s
//
// time ./benchmarks --read --from /media/disk1/multimap/ --nkeys 100000 --nvalues 2000 --bs 4096
// real 7m44.459s
// user 0m3.160s
// sys  0m6.004s
//
// time ./benchmarks --read --from /media/disk1/multimap/ --nkeys 1000000 --nvalues 1 --bs 1024
// real 1m13.829s
// user 0m2.784s
// sys  0m1.236s
//
// time ./benchmarks --read --from /media/disk1/multimap/ --nkeys 1000000 --nvalues 1 --bs 128
// real 0m9.749s
// user 0m2.272s
// sys  0m0.240s

void RunCopyCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");
  CheckOption(variables, "to");

  multimap::Copy(variables["from"].as<std::string>(),
                 variables["to"].as<std::string>());
}

void RunImportCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");
  CheckOption(variables, "to");

  multimap::Import(variables["to"].as<std::string>(),
                   variables["from"].as<std::string>());
}

void RunExportCommand(const po::variables_map& variables) {
  CheckOption(variables, "from");
  CheckOption(variables, "to");

  multimap::Export(variables["from"].as<std::string>(),
                   variables["to"].as<std::string>());
}
// time ./benchmarks --export --from /media/disk2/multimap --to /tmp/multimap-export.csv (--nkeys 100000 --nvalues 2000))
// real 176m9.858s
// user 3m9.320s
// sys  1m38.142s
//
// AFTER USING MMAP
//
// time ./benchmarks --export --from /media/disk1/multimap/ --to /tmp/multimap-export.csv --bs 512
// real 173m23.808s
// user 135m7.251s
// sys  0m29.298s

// TODO Add options for block-size and sort.
// http://unix.stackexchange.com/questions/87908/how-do-you-empty-the-buffers-and-cache-on-a-linux-system
// ~/bin/linux-fincore --pages=false --summarize --only-cached
// /media/disk2/multimap/*
// sudo sh -c 'echo 1 >/proc/sys/vm/drop_caches'
// sudo sh -c 'echo 2 >/proc/sys/vm/drop_caches'
// sudo sh -c 'echo 3 >/proc/sys/vm/drop_caches'
int main(int argc, char* argv[]) {
  po::options_description commands_description("COMMANDS");
  commands_description.add_options()("help",
                                     "Print this help message and exit.")(
      "write", "Write a Multimap to directory.")(
      "read", "Read a Multimap from directory.")(
      "copy", "Copy a Multimap from one directory to another.")(
      "import", "Import base64 encoded csv data into a Multimap.")(
      "export", "Export a Multimap to a base64 encoded csv file.");

  po::options_description options_description("OPTIONS");
  options_description.add_options()("from", po::value<std::string>(),
                                    "Directory or file to read from.")(
      "to", po::value<std::string>(), "Directory or file to write to.")(
      "nkeys", po::value<std::size_t>(), "Number of keys to put.")(
      "nvalues", po::value<std::size_t>(), "Number of values per key to put.")(
      "bs", po::value<std::size_t>(), "Block size.");

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
    } else if (variables.count("import")) {
      RunImportCommand(variables);
    } else if (variables.count("export")) {
      RunExportCommand(variables);
    } else {
      std::cout << "Try with --help\n";
    }
  } catch (std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
