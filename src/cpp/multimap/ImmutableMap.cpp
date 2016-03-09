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
#include "multimap/internal/TsvFileReader.hpp"
#include "multimap/internal/TsvFileWriter.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {

namespace {

std::string getPrefix() { return "multimap.immutablemap"; }

std::string getNameOfIdFile() { return getPrefix() + ".id"; }

std::string getNameOfLockFile() { return getPrefix() + ".lock"; }

std::string getPrefix(size_t index) {
  return getPrefix() + '.' + std::to_string(index);
}

template <typename Element>
Element& select(std::vector<Element>& elements, const Range& key) {
  return elements[std::hash<Range>()(key) % elements.size()];
}

template <typename Element>
const Element& select(const std::vector<Element>& elements, const Range& key) {
  return elements[std::hash<Range>()(key) % elements.size()];
}

}  // namespace

ImmutableMap::Id ImmutableMap::Id::readFromDirectory(
    const boost::filesystem::path& directory) {
  return readFromFile(directory / getNameOfIdFile());
}

ImmutableMap::Id ImmutableMap::Id::readFromFile(
    const boost::filesystem::path& filename) {
  Id id;
  const auto stream = mt::open(filename.string(), "r");
  mt::read(stream.get(), &id, sizeof id);
  return id;
}

void ImmutableMap::Id::writeToFile(
    const boost::filesystem::path& filename) const {
  const auto stream = mt::open(filename.string(), "w");
  mt::write(stream.get(), this, sizeof *this);
}

uint32_t ImmutableMap::Limits::maxKeySize() {
  return internal::MphTable::Limits::maxKeySize();
}

uint32_t ImmutableMap::Limits::maxValueSize() {
  return internal::MphTable::Limits::maxValueSize();
}

ImmutableMap::Builder::Builder(const boost::filesystem::path& directory,
                               const Options& options)
    : options_(options), dlock_(directory.string(), getNameOfLockFile()) {
  mt::Check::isEqual(
      std::distance(boost::filesystem::directory_iterator(directory),
                    boost::filesystem::directory_iterator()),
      1, "Directory must be empty '%s'", directory.c_str());
  buckets_.resize(mt::nextPrime(options.num_buckets));
  for (size_t i = 0; i != buckets_.size(); i++) {
    auto& bucket = buckets_[i];
    bucket.filename = directory / getPrefix(i);
    bucket.stream = mt::open(bucket.filename.string(), "w+");
  }
}

void ImmutableMap::Builder::put(const Range& key, const Range& value) {
  Bucket& bucket = select(buckets_, key);
  key.writeToStream(bucket.stream.get());
  value.writeToStream(bucket.stream.get());
  bucket.size += value.size();
  if (bucket.size > options_.max_bucket_size && !bucket.is_large) {
    // Double number of buckets and rehash all records.
    std::vector<Bucket> new_buckets(mt::nextPrime(buckets_.size() * 2));
    if (!options_.quiet)
      mt::log() << "Rehashing from " << buckets_.size() << " to "
                << new_buckets.size() << " buckets triggered by "
                << bucket.filename.string() << std::endl;
    for (size_t i = 0; i != new_buckets.size(); i++) {
      auto& new_bucket = new_buckets[i];
      new_bucket.filename = dlock_.directory() / (getPrefix(i) + ".new");
      new_bucket.stream = mt::open(new_bucket.filename.string(), "w+");
    }
    Bytes old_key;
    Bytes old_value;
    for (auto& bucket : buckets_) {
      mt::seek(bucket.stream.get(), 0, SEEK_SET);
      while (readBytesFromStream(bucket.stream.get(), &old_key) &&
             readBytesFromStream(bucket.stream.get(), &old_value)) {
        Bucket& new_bucket = select(new_buckets, old_key);
        writeBytesToStream(new_bucket.stream.get(), old_key);
        writeBytesToStream(new_bucket.stream.get(), old_value);
        new_bucket.size += old_value.size();
      }
      bucket.stream.reset();
      boost::filesystem::remove(bucket.filename);
    }
    // Rename new bucket files and check if there are large ones.
    for (auto& new_bucket : new_buckets) {
      const auto old_filename = new_bucket.filename;
      const auto new_filename = new_bucket.filename.replace_extension();
      boost::filesystem::rename(old_filename, new_filename);
      if (new_bucket.size > options_.max_bucket_size) {
        new_bucket.is_large = true;
      }
    }
    buckets_ = std::move(new_buckets);
    if (!options_.quiet) mt::log() << "Rehashing finished" << std::endl;
  }
}

std::vector<ImmutableMap::Stats> ImmutableMap::Builder::build() {
  std::vector<Stats> stats;
  for (auto& bucket : buckets_) {
    bucket.stream.reset();
    stats.push_back(internal::MphTable::build(
        bucket.filename.string(), bucket.filename.string(), options_));
    boost::filesystem::remove(bucket.filename);
  }
  return stats;
}

ImmutableMap::ImmutableMap(const boost::filesystem::path& directory)
    : dlock_(directory.string()) {}

std::unique_ptr<Iterator> ImmutableMap::get(const Range& key) const {
  return select(tables_, key).get(key);
}

void ImmutableMap::forEachKey(Procedure process) const {
  for (const auto& table : tables_) {
    table.forEachKey(process);
  }
}

void ImmutableMap::forEachValue(const Range& key, Procedure process) const {
  select(tables_, key).forEachValue(key, process);
}

void ImmutableMap::forEachEntry(BinaryProcedure process) const {
  for (const auto& table : tables_) {
    table.forEachEntry(process);
  }
}

std::vector<ImmutableMap::Stats> ImmutableMap::getStats() const {
  std::vector<ImmutableMap::Stats> stats(tables_.size());
  for (size_t i = 0; i != tables_.size(); i++) {
    stats[i] = tables_[i].getStats();
  }
  return stats;
}

ImmutableMap::Stats ImmutableMap::getTotalStats() const {
  return Stats::total(getStats());
}

std::vector<ImmutableMap::Stats> ImmutableMap::stats(
    const boost::filesystem::path& directory) {
  const auto id = Id::readFromDirectory(directory);
  std::vector<ImmutableMap::Stats> stats(id.num_tables);
  for (size_t i = 0; i != id.num_tables; i++) {
    stats[i] = internal::MphTable::stats(getPrefix(i));
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

  std::vector<boost::filesystem::path> files;
  if (boost::filesystem::is_regular_file(source)) {
    files.push_back(source);
  } else if (boost::filesystem::is_directory(source)) {
    files = mt::Files::list(source);
  } else {
    mt::fail("Not a regular file or directory '%s'", source.c_str());
  }

  Bytes key;
  Bytes value;
  for (const auto& file : files) {
    if (!options.quiet) {
      mt::log() << "Importing " << file.string() << std::endl;
    }
    internal::TsvFileReader reader(file);
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
    if (!options.quiet) {
      mt::log() << "Exporting partition " << (i + 1) << " of "
                << map.tables_.size() << std::endl;
    }
    map.tables_[i].forEachEntry([&writer](const Range& key, Iterator* iter) {
      writer.write(key, iter);
    });
  }
}

}  // namespace multimap
