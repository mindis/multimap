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

#include "multimap/internal/Arena.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

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
    : cmph_(cmph_load(mt::fopen(filename.string(), "r").get())) {}

Mph Mph::build(const uint8_t** keys, size_t nkeys, const Options& options) {
  std::unique_ptr<cmph_io_adapter_t> source(
      cmph_io_byte_vector_adapter(const_cast<uint8_t**>(keys), nkeys));
  std::unique_ptr<cmph_config_t> config(cmph_config_new(source.get()));

  cmph_config_set_algo(config.get(), options.algorithm);
  if (options.b != 0) cmph_config_set_b(config.get(), options.b);
  if (options.c != 0) cmph_config_set_graphsize(config.get(), options.c);

  std::unique_ptr<cmph_t> cmph(cmph_new(config.get()));
  mt::Check::notNull(cmph.get(), "cmph_new() failed");
  MT_ASSERT_EQ(cmph_size(cmph.get()), nkeys);
  return Mph(std::move(cmph));
}

Mph Mph::build(const boost::filesystem::path& keys, const Options& options) {
  Arena arena;
  uint32_t keylen;
  std::vector<const uint8_t*> keydata;
  const auto stream = mt::fopen(keys.string(), "r");
  while (mt::freadMaybe(stream.get(), &keylen, sizeof keylen)) {
    uint8_t* key = arena.allocate(sizeof keylen + keylen);
    std::memcpy(key, &keylen, sizeof keylen);
    mt::fread(stream.get(), key + sizeof keylen, keylen);
    keydata.push_back(key);
  }
  return build(keydata.data(), keydata.size(), options);
}

void Mph::dump(const boost::filesystem::path& filename) const {
  const auto stream = mt::fopen(filename.string(), "w");
  mt::Check::notZero(cmph_dump(cmph_.get(), stream.get()),
                     "cmph_dump() failed");
}

Mph::Mph(std::unique_ptr<cmph_t> cmph) : cmph_(std::move(cmph)) {}

}  // namespace internal
}  // namespace multimap
