// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
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

#include "multimap/ImmutableMap.hpp"

#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Descriptor.hpp"
#include "multimap/internal/TsvFileReader.hpp"
#include "multimap/internal/TsvFileWriter.hpp"
#include "multimap/thirdparty/mt/check.hpp"
#include "multimap/Arena.hpp"

namespace multimap {

namespace {

std::string getFilePrefix(const boost::filesystem::path& basedir) {
  return internal::Descriptor::getFullFilePrefix(basedir) + ".immutablemap";
}

std::string getPartitionPrefix(const boost::filesystem::path& basedir,
                               size_t index) {
  return getFilePrefix(basedir) + '.' + std::to_string(index);
}

template <typename Element>
Element& select(std::vector<Element>& elements, const Slice& key) {
  return elements[std::hash<Slice>()(key) % elements.size()];
}

template <typename Element>
const Element& select(const std::vector<Element>& elements, const Slice& key) {
  return elements[std::hash<Slice>()(key) % elements.size()];
}

}  // namespace

size_t ImmutableMap::Limits::maxKeySize() {
  return internal::MphTable::Limits::maxKeySize();
}

size_t ImmutableMap::Limits::maxValueSize() {
  return internal::MphTable::Limits::maxValueSize();
}

ImmutableMap::Builder::Builder(const boost::filesystem::path& directory,
                               const Options& options)
    : options_(options), dlock_(directory.string()) {
  mt::Check::isEqual(
      std::distance(boost::filesystem::directory_iterator(directory),
                    boost::filesystem::directory_iterator()),
      1, "Must be empty: %s", directory.c_str());
  const auto num_buckets = mt::nextPrime(options.num_partitions);
  for (size_t i = 0; i != num_buckets; i++) {
    buckets_.emplace_back(internal::MphTable::Builder(
        getPartitionPrefix(directory, i), options_));
  }
}

void ImmutableMap::Builder::put(const Slice& key, const Slice& value) {
  Bucket& bucket = select(buckets_, key);
  bucket.builder.put(key, value);
  if ((bucket.builder.getFileSize() > options_.max_partition_size) &&
      !bucket.is_large) {
    rehash();
  }
}

std::vector<Stats> ImmutableMap::Builder::build() {
  std::vector<Stats> stats;
  for (size_t i = 0; i != buckets_.size(); i++) {
    if (options_.verbose) {
      mt::log() << "Building partition " << (i + 1) << " of " << buckets_.size()
                << std::endl;
    }
    stats.push_back(buckets_[i].builder.build());
  }

  internal::Descriptor descriptor;
  descriptor.map_type = internal::Descriptor::IMMUTABLE_MAP;
  descriptor.num_partitions = buckets_.size();
  descriptor.writeToDirectory(dlock_.directory());

  return stats;
}

void ImmutableMap::Builder::rehash() {
  const auto num_new_buckets = mt::nextPrime(buckets_.size() * 2);
  if (options_.verbose) {
    mt::log() << "Rehashing from " << buckets_.size() << " to "
              << num_new_buckets << " buckets" << std::endl;
  }
  std::vector<Bucket> new_buckets;
  for (size_t i = 0; i != num_new_buckets; i++) {
    new_buckets.emplace_back(internal::MphTable::Builder(
        getPartitionPrefix(dlock_.directory(), i) + ".new", options_));
  }
  Bytes old_key;
  Bytes old_value;
  for (Bucket& bucket : buckets_) {
    const boost::filesystem::path file_path = bucket.builder.releaseFile();
    {
      const mt::AutoCloseFile stream = mt::fopen(file_path, "r");
      while (readBytesFromStream(stream.get(), &old_key) &&
             readBytesFromStream(stream.get(), &old_value)) {
        select(new_buckets, old_key).builder.put(old_key, old_value);
      }
    }
    boost::filesystem::remove(file_path);
  }
  // Rename new bucket files and check if there are large ones.
  for (Bucket& new_bucket : new_buckets) {
    const auto old_file_path = new_bucket.builder.getFilePath();
    auto new_file_path = old_file_path;
    new_file_path.replace_extension();
    boost::filesystem::rename(old_file_path, new_file_path);
    if (new_bucket.builder.getFileSize() > options_.max_partition_size) {
      new_bucket.is_large = true;
    }
  }
  buckets_ = std::move(new_buckets);
  if (options_.verbose) {
    mt::log() << "Rehashing finished" << std::endl;
  }
}

ImmutableMap::ImmutableMap(const boost::filesystem::path& directory)
    : dlock_(directory.string()) {
  const auto desc = internal::Descriptor::readFromDirectory(directory);
  internal::Descriptor::validate(desc, internal::Descriptor::IMMUTABLE_MAP);
  for (int i = 0; i < desc.num_partitions; i++) {
    const auto full_prefix = getPartitionPrefix(directory, i);
    tables_.push_back(internal::MphTable(full_prefix));
  }
}

std::unique_ptr<Iterator> ImmutableMap::get(const Slice& key) const {
  return select(tables_, key).get(key);
}

void ImmutableMap::forEachKey(Procedure process) const {
  for (const auto& table : tables_) {
    table.forEachKey(process);
  }
}

void ImmutableMap::forEachValue(const Slice& key, Procedure process) const {
  select(tables_, key).forEachValue(key, process);
}

void ImmutableMap::forEachEntry(BinaryProcedure process) const {
  for (const auto& table : tables_) {
    table.forEachEntry(process);
  }
}

std::vector<Stats> ImmutableMap::getStats() const {
  std::vector<Stats> stats(tables_.size());
  for (size_t i = 0; i != tables_.size(); i++) {
    stats[i] = tables_[i].getStats();
  }
  return stats;
}

Stats ImmutableMap::getTotalStats() const { return Stats::total(getStats()); }

std::vector<Stats> ImmutableMap::stats(
    const boost::filesystem::path& directory) {
  const auto desc = internal::Descriptor::readFromDirectory(directory);
  internal::Descriptor::validate(desc, internal::Descriptor::IMMUTABLE_MAP);
  std::vector<Stats> stats(desc.num_partitions);
  for (int i = 0; i < desc.num_partitions; i++) {
    stats[i] = internal::MphTable::stats(getPartitionPrefix(directory, i));
  }
  return stats;
}

void ImmutableMap::buildFromBase64(const boost::filesystem::path& directory,
                                   const boost::filesystem::path& source) {
  buildFromBase64(directory, source, Options());
}

void ImmutableMap::buildFromBase64(const boost::filesystem::path& directory,
                                   const boost::filesystem::path& source,
                                   const Options& options) {
  Builder builder(directory, options);

  std::vector<boost::filesystem::path> file_paths;
  if (boost::filesystem::is_regular_file(source)) {
    file_paths.push_back(source);
  } else if (boost::filesystem::is_directory(source)) {
    file_paths = mt::listFilePaths(source);
  } else {
    mt::fail("No such file or directory: %s", source.c_str());
  }

  Bytes key;
  Bytes value;
  for (const auto& file_path : file_paths) {
    if (options.verbose) {
      mt::log() << "Importing " << file_path.string() << std::endl;
    }
    internal::TsvFileReader reader(file_path);
    while (reader.read(&key, &value)) {
      builder.put(key, value);
    }
  }
  builder.build();
}

void ImmutableMap::exportToBase64(const boost::filesystem::path& directory,
                                  const boost::filesystem::path& target) {
  exportToBase64(directory, target, Options());
}

void ImmutableMap::exportToBase64(const boost::filesystem::path& directory,
                                  const boost::filesystem::path& target,
                                  const Options& options) {
  ImmutableMap map(directory);
  internal::TsvFileWriter writer(target);
  for (size_t i = 0; i != map.tables_.size(); i++) {
    if (options.verbose) {
      mt::log() << "Exporting partition " << (i + 1) << " of "
                << map.tables_.size() << std::endl;
    }
    if (options.compare) {
      Arena arena;
      std::vector<Slice> values;
      map.tables_[i].forEachEntry([&writer, &arena, &options, &values](
          const Slice& key, Iterator* iter) {
        arena.deallocateAll();
        values.reserve(iter->available());
        values.clear();
        while (iter->hasNext()) {
          values.push_back(iter->next().makeCopy(
              [&arena](size_t size) { return arena.allocate(size); }));
        }
        std::sort(values.begin(), values.end(), options.compare);
        auto range_iter = makeRangeIterator(values.begin(), values.end());
        writer.write(key, &range_iter);
      });
    } else {
      map.tables_[i].forEachEntry([&writer](const Slice& key, Iterator* iter) {
        writer.write(key, iter);
      });
    }
  }
}

}  // namespace multimap
