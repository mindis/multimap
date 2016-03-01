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
#include "multimap/internal/Varint.hpp"

namespace multimap {

namespace {

std::string getPrefix() { return "multimap.immutablemap"; }

std::string getNameOfIdFile() { return getPrefix() + ".id"; }

std::string getNameOfLockFile() { return getPrefix() + ".lock"; }

std::string getPrefix(size_t index) {
  return getPrefix() + '.' + std::to_string(index);
}

std::string getNameOfMphfFile(size_t index) {
  return internal::MphTable::getNameOfMphfFile(getPrefix(index));
}

std::string getNameOfDataFile(size_t index) {
  return internal::MphTable::getNameOfDataFile(getPrefix(index));
}

std::string getNameOfListsFile(size_t index) {
  return internal::MphTable::getNameOfListsFile(getPrefix(index));
}

std::string getNameOfStatsFile(size_t index) {
  return internal::MphTable::getNameOfStatsFile(getPrefix(index));
}

template <typename Element>
Element& select(std::vector<Element>& vec, const Bytes& key) {
  return vec[std::hash<Bytes>()(key) % vec.size()];
}

template <typename Element>
Element& select(std::vector<Element>& vec, const std::vector<char>& key) {
  return vec[std::hash<Bytes>()(Bytes(key.data(), key.size())) % vec.size()];
}

}  // namespace

ImmutableMap::Id ImmutableMap::Id::readFromDirectory(
    const boost::filesystem::path& directory) {
  return readFromFile(directory / getNameOfIdFile());
}

ImmutableMap::Id ImmutableMap::Id::readFromFile(
    const boost::filesystem::path& filename) {
  Id id;
  const auto stream = mt::fopen(filename, "r");
  mt::fread(stream.get(), &id, sizeof id);
  return id;
}

void ImmutableMap::Id::writeToFile(
    const boost::filesystem::path& filename) const {
  const auto stream = mt::fopen(filename, "w");
  mt::fwrite(stream.get(), this, sizeof *this);
}

uint32_t ImmutableMap::Limits::maxKeySize() {
  return internal::MphTable::Limits::maxKeySize();
}

uint32_t ImmutableMap::Limits::maxValueSize() {
  return internal::MphTable::Limits::maxValueSize();
}

ImmutableMap::Builder::Builder(const boost::filesystem::path& directory,
                               const Options& options)
    : options_(options), dlock_(directory, getNameOfLockFile()) {
  mt::Check::isEqual(
      std::distance(boost::filesystem::directory_iterator(directory),
                    boost::filesystem::directory_iterator()),
      1, "Directory must be empty '%s'", directory.c_str());
  buckets_.resize(mt::nextPrime(options.num_buckets));
  for (size_t i = 0; i != buckets_.size(); i++) {
    auto& bucket = buckets_[i];
    bucket.filename = directory / getPrefix(i);
    bucket.stream = mt::fopen(bucket.filename, "w+");
  }
}

ImmutableMap::Builder::~Builder() {
  for (auto& bucket : buckets_) {
    //    bucket.stream.reset();
    boost::filesystem::remove(bucket.filename);
  }
}

void ImmutableMap::Builder::put(const Bytes& key, const Bytes& value) {
  auto& bucket = select(buckets_, key);
  MT_ASSERT_TRUE(writeBytes(bucket.stream.get(), key));
  MT_ASSERT_TRUE(writeBytes(bucket.stream.get(), value));
  bucket.size += value.size();
  if (bucket.size > options_.max_bucket_size && !bucket.is_large) {
    // Double number of buckets and rehash all records.

    mt::log() << "Rehashing caused by " << bucket.filename << std::endl;

    std::vector<Bucket> new_buckets(mt::nextPrime(buckets_.size() * 2));
    for (size_t i = 0; i != new_buckets.size(); i++) {
      auto& new_bucket = new_buckets[i];
      new_bucket.filename = dlock_.directory() / (getPrefix(i) + ".new");
      new_bucket.stream = mt::fopen(new_bucket.filename, "w+");
    }
    std::vector<char> old_key;
    std::vector<char> old_value;
    for (auto& bucket : buckets_) {
      MT_ASSERT_TRUE(mt::fseek(bucket.stream.get(), 0, SEEK_SET));
      while (readBytes(bucket.stream.get(), &old_key) &&
             readBytes(bucket.stream.get(), &old_value)) {
        auto& new_bucket = select(new_buckets, old_key);
        writeBytes(new_bucket.stream.get(), old_key);
        writeBytes(new_bucket.stream.get(), old_value);
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

    mt::log() << "Rehashing done. #buckets " << buckets_.size() << std::endl;
  }
}

std::vector<ImmutableMap::Stats> ImmutableMap::Builder::build() {
  std::vector<Stats> stats;
  for (auto& bucket : buckets_) {
    bucket.stream.reset();
    stats.push_back(internal::MphTable::build(bucket.filename, options_));
    boost::filesystem::remove(bucket.filename);
  }
  return stats;
}

ImmutableMap::ImmutableMap(const boost::filesystem::path& directory)
    : dlock_(directory) {}

ImmutableMap::~ImmutableMap() {}

std::unique_ptr<Iterator> ImmutableMap::get(const Bytes& key) const {}

std::vector<ImmutableMap::Stats> ImmutableMap::getStats() const {}

ImmutableMap::Stats ImmutableMap::getTotalStats() const {}

std::vector<ImmutableMap::Stats> ImmutableMap::stats(
    const boost::filesystem::path& directory) {}

void ImmutableMap::buildFromBase64(const boost::filesystem::path& directory,
                                   const boost::filesystem::path& input) {}

void ImmutableMap::buildFromBase64(const boost::filesystem::path& directory,
                                   const boost::filesystem::path& input,
                                   const Options& options) {}

void ImmutableMap::exportToBase64(const boost::filesystem::path& directory,
                                  const boost::filesystem::path& output) {}

void ImmutableMap::exportToBase64(const boost::filesystem::path& directory,
                                  const boost::filesystem::path& output,
                                  const Options& options) {}

}  // namespace multimap
