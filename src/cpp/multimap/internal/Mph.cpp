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

#include "multimap/internal/Mph.hpp"

#include "multimap/Arena.hpp"

namespace std {

template <>
struct default_delete<cmph_io_adapter_t> {
  void operator()(cmph_io_adapter_t* io_adapter) const {
    if (io_adapter) cmph_io_byte_vector_adapter_destroy(io_adapter);
  }
};

template <>
struct default_delete<cmph_config_t> {
  void operator()(cmph_config_t* config) const {
    if (config) cmph_config_destroy(config);
  }
};

}  // namespace std

namespace multimap {
namespace internal {

Mph::Mph(const boost::filesystem::path& filename)
    : cmph_(cmph_load(mt::open(filename.string(), "r").get())) {}

Mph Mph::build(const byte** keys, size_t nkeys) {
  std::unique_ptr<cmph_io_adapter_t> source(
      cmph_io_byte_vector_adapter(const_cast<byte**>(keys), nkeys));

  std::unique_ptr<cmph_config_t> config(cmph_config_new(source.get()));
  cmph_config_set_algo(config.get(), CMPH_BDZ);

  std::unique_ptr<cmph_t> cmph(cmph_new(config.get()));
  mt::Check::notNull(cmph.get(), "cmph_new() failed");
  MT_ASSERT_EQ(cmph_size(cmph.get()), nkeys);
  return Mph(std::move(cmph));
}

Mph Mph::build(const boost::filesystem::path& keys) {
  Arena arena;
  uint32_t keylen;
  std::vector<const byte*> keydata;
  const auto stream = mt::open(keys.string(), "r");
  while (mt::tryRead(stream.get(), &keylen, sizeof keylen)) {
    byte* key = arena.allocate(sizeof keylen + keylen);
    std::memcpy(key, &keylen, sizeof keylen);
    mt::read(stream.get(), key + sizeof keylen, keylen);
    keydata.push_back(key);
  }
  return build(keydata.data(), keydata.size());
}

size_t Mph::operator()(const Slice& key) const {
  return cmph_search(cmph_.get(), reinterpret_cast<const char*>(key.begin()),
                     key.size());
}

void Mph::writeToFile(const boost::filesystem::path& filename) const {
  const auto stream = mt::open(filename.string(), "w");
  mt::Check::notZero(cmph_dump(cmph_.get(), stream.get()),
                     "cmph_dump() failed");
}

Mph::Mph(std::unique_ptr<cmph_t> cmph) : cmph_(std::move(cmph)) {}

}  // namespace internal
}  // namespace multimap
