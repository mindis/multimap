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

namespace bfs = boost::filesystem;

namespace multimap {

namespace {

void checkDescriptor(const internal::Descriptor& descriptor,
                     const bfs::path& directory) {
  mt::Check::isEqual(
      internal::Descriptor::TYPE_IMMUTABLE_MAP, descriptor.map_type,
      "Wrong map type: expected type %s but found type %s in %s",
      internal::Descriptor::toString(internal::Descriptor::TYPE_IMMUTABLE_MAP),
      internal::Descriptor::toString(descriptor.map_type), directory.c_str());
}

std::string getFilePrefix() {
  return internal::Descriptor::getFilePrefix() + ".immutablemap";
}

std::string getPartitionPrefix(size_t index) {
  return getFilePrefix() + '.' + std::to_string(index);
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

ImmutableMap::Builder::Builder(const bfs::path& directory,
                               const Options& options)
    : options_(options), dlock_(directory.string()) {
  mt::Check::isEqual(std::distance(bfs::directory_iterator(directory),
                                   bfs::directory_iterator()),
                     1, "Must be empty: %s", directory.c_str());
  const auto num_buckets = mt::nextPrime(options.num_partitions);
  for (size_t i = 0; i != num_buckets; i++) {
    buckets_.emplace_back(internal::MphTable::Builder(
        directory / getPartitionPrefix(i), options_));
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
  descriptor.map_type = internal::Descriptor::TYPE_IMMUTABLE_MAP;
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
        dlock_.directory() / (getPartitionPrefix(i) + ".new"), options_));
  }
  Bytes old_key;
  Bytes old_value;
  for (Bucket& bucket : buckets_) {
    const bfs::path file_path = bucket.builder.releaseFile();
    {
      const mt::AutoCloseFile stream = mt::fopen(file_path, "r");
      while (readBytesFromStream(stream.get(), &old_key) &&
             readBytesFromStream(stream.get(), &old_value)) {
        select(new_buckets, old_key).builder.put(old_key, old_value);
      }
    }
    bfs::remove(file_path);
  }
  // Rename new bucket files and check if there are large ones.
  for (Bucket& new_bucket : new_buckets) {
    const auto old_file_path = new_bucket.builder.getFilePath();
    auto new_file_path = old_file_path;
    new_file_path.replace_extension();
    bfs::rename(old_file_path, new_file_path);
    if (new_bucket.builder.getFileSize() > options_.max_partition_size) {
      new_bucket.is_large = true;
    }
  }
  buckets_ = std::move(new_buckets);
  if (options_.verbose) {
    mt::log() << "Rehashing finished" << std::endl;
  }
}

ImmutableMap::ImmutableMap(const bfs::path& directory)
    : dlock_(directory.string()) {
  const auto descriptor = internal::Descriptor::readFromDirectory(directory);
  checkDescriptor(descriptor, directory);
  for (int i = 0; i < descriptor.num_partitions; i++) {
    const bfs::path prefix = directory / getPartitionPrefix(i);
    tables_.push_back(internal::MphTable(prefix));
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

std::vector<Stats> ImmutableMap::stats(const bfs::path& directory) {
  const auto descriptor = internal::Descriptor::readFromDirectory(directory);
  checkDescriptor(descriptor, directory);
  std::vector<Stats> stats(descriptor.num_partitions);
  for (int i = 0; i < descriptor.num_partitions; i++) {
    const bfs::path prefix = directory / getPartitionPrefix(i);
    stats[i] = internal::MphTable::stats(prefix);
  }
  return stats;
}

void ImmutableMap::buildFromBase64(const bfs::path& directory,
                                   const bfs::path& source) {
  buildFromBase64(directory, source, Options());
}

void ImmutableMap::buildFromBase64(const bfs::path& directory,
                                   const bfs::path& source,
                                   const Options& options) {
  Builder builder(directory, options);

  std::vector<bfs::path> file_paths;
  if (bfs::is_regular_file(source)) {
    file_paths.push_back(source);
  } else if (bfs::is_directory(source)) {
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

void ImmutableMap::exportToBase64(const bfs::path& directory,
                                  const bfs::path& target) {
  exportToBase64(directory, target, Options());
}

void ImmutableMap::exportToBase64(const bfs::path& directory,
                                  const bfs::path& target,
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
