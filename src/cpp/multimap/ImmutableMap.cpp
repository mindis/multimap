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

namespace multimap {

namespace {

std::string getPrefix() { return "multimap.immutablemap"; }

std::string getNameOfIdFile() { return getPrefix() + ".id"; }

std::string getNameOfLockFile() { return getPrefix() + ".lock"; }

std::string getTablePrefix(size_t index) {
  return getPrefix() + '.' + std::to_string(index);
}

std::string getNameOfMphfFile(size_t index) {
  return internal::MphTable::getNameOfMphfFile(getTablePrefix(index));
}

std::string getNameOfDataFile(size_t index) {
  return internal::MphTable::getNameOfDataFile(getTablePrefix(index));
}

std::string getNameOfListsFile(size_t index) {
  return internal::MphTable::getNameOfListsFile(getTablePrefix(index));
}

std::string getNameOfStatsFile(size_t index) {
  return internal::MphTable::getNameOfStatsFile(getTablePrefix(index));
}

// index is in [0, 64) where 0 is the LSB.
bool isBitSet(size_t hash, size_t index) { return (hash >> index) & 1; }

void writeBytes(std::FILE* stream, const Bytes& bytes) {
  const uint32_t size = bytes.size();
  MT_ASSERT_TRUE(mt::fwrite(stream, &size, sizeof size));
  MT_ASSERT_TRUE(mt::fwrite(stream, bytes.data(), size));
}

bool readBytes(std::FILE* stream, std::vector<char>* bytes) {
  uint32_t size;
  if (mt::fread(stream, &size, sizeof size)) {
    bytes->resize(size);
    MT_ASSERT_TRUE(mt::fread(stream, bytes->data(), size));
    return true;
  }
  return false;
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

ImmutableMap::Builder::Builder(const boost::filesystem::path& directory)
    : directory_lock_(directory) {
  mt::Check::isEqual(
      std::distance(boost::filesystem::directory_iterator(directory),
                    boost::filesystem::directory_iterator()),
      1, "Directory must be empty '%s'", directory.c_str());
  buckets_.reset(new BucketNode(0, directory / (getPrefix() + ".b")));
}

void ImmutableMap::Builder::put(const Bytes& key, const Bytes& value) {
  buckets_->put(key, value);
}

std::vector<ImmutableMap::Stats> ImmutableMap::Builder::build(
    const Options& options) {
  std::vector<Stats> stats;
  buckets_->forEachLeaf([&](BucketNode* bucket) {
    const auto datafile = bucket->close();
    stats.push_back(internal::MphTable::build(datafile, options));
    boost::filesystem::remove(datafile);
  });
  return stats;
}

ImmutableMap::Builder::BucketNode::BucketNode(
    uint32_t depth, const boost::filesystem::path& filename)
    : depth_(depth), filename_(filename), stream_(mt::fopen(filename, "w+")) {}

ImmutableMap::Builder::BucketNode::~BucketNode() {
  stream_.reset();
  //  boost::filesystem::remove(filename_);
}

void ImmutableMap::Builder::BucketNode::put(const Bytes& key,
                                            const Bytes& value, size_t hash) {
  if (isLeaf()) {
    writeBytes(stream_.get(), key);
    writeBytes(stream_.get(), value);
    payload_ += value.size();
    if (payload_ > MAX_PAYLOAD && !is_large_) {
      // Split bucket.
      std::vector<char> key, value;
      lhs_.reset(new BucketNode(depth_ + 1, filename_.string() + '0'));
      rhs_.reset(new BucketNode(depth_ + 1, filename_.string() + '1'));
      MT_ASSERT_TRUE(mt::fseek(stream_.get(), 0, SEEK_SET));

      mt::log() << "Splitting bucket " << filename_ << " of size " << payload_
                << std::endl;

      while (readBytes(stream_.get(), &key) &&
             readBytes(stream_.get(), &value)) {
        put(Bytes(key.data(), key.size()), Bytes(value.data(), value.size()));
      }
      stream_.reset();
      MT_ASSERT_TRUE(boost::filesystem::remove(filename_));
      if (lhs_->payload_ > MAX_PAYLOAD) {
        lhs_->is_large_ = true;
      }
      if (rhs_->payload_ > MAX_PAYLOAD) {
        rhs_->is_large_ = true;
      }

      mt::log() << "New bucket 0 " << lhs_->filename_ << " of size "
                << lhs_->payload_ << std::endl;
      mt::log() << "New bucket 1 " << rhs_->filename_ << " of size "
                << rhs_->payload_ << std::endl;
    }
  } else {
    return (isBitSet(hash, depth_) ? rhs_ : lhs_)->put(key, value, hash);
  }
}

ImmutableMap::ImmutableMap(const boost::filesystem::path& directory)
    : directory_lock_(directory) {}

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

const internal::MphTable* ImmutableMap::MphTableNode::get(size_t hash,
                                                          size_t depth) const {
  if (isLeaf()) return table.get();
  return (isBitSet(hash, depth) ? right : left)->get(hash, ++depth);
}

}  // namespace multimap
