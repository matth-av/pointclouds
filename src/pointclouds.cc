#include "../include/pointclouds.h"
#include <chrono>
#include <iostream>
#include <zpp_bits.h>

namespace bob::types {

using namespace std::chrono;

namespace {
constexpr uint8_t payload_raw = 0;
constexpr uint8_t payload_compressed = 1;
} // namespace

auto PointCloud::serialize(bool compress) const -> bytes {
  constexpr std::size_t header_bytes = sizeof(uint64_t)   // timestamp
                                       + sizeof(uint64_t) // point_count
                                       + sizeof(uint8_t); // compression flag

  const auto timestamp = static_cast<uint64_t>(
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count());
  const uint64_t point_count = size();

  bytes buffer;

#ifdef WITH_CODEC
  if (compress) {
    const auto payload = this->compress();
    buffer.reserve(header_bytes + payload.size()); // reserve is a hint
    zpp::bits::out zpp_serialize{buffer};
    zpp_serialize(timestamp, point_count, payload_compressed, payload)
        .or_throw();
    return buffer;
  }
#endif

  // not compressed:

  buffer.reserve(header_bytes + positions.size() * sizeof(position) +
                 colors.size() * sizeof(color));
  zpp::bits::out zpp_serialize{buffer};
  zpp_serialize(timestamp, point_count, payload_raw).or_throw();
  zpp_serialize(*this).or_throw();
  return buffer;
}

auto PointCloud::deserialize(std::span<const std::byte> buffer) -> PointCloud {
  zpp::bits::in zpp_deserialize{buffer};

  uint64_t timestamp = 0; // read to advance the stream; unused
  uint64_t point_count = 0;
  uint8_t compression_flag = payload_raw;
  zpp_deserialize(timestamp, point_count, compression_flag).or_throw();

  if (compression_flag != payload_raw) {
#ifdef WITH_CODEC
    bytes payload;
    zpp_deserialize(payload).or_throw();
    return PointCloud::decompress(payload, point_count);
#else
    std::cerr << "Received a compressed PointCloudPacket, but this library "
                 "was built without codec support\n";
    return {};
#endif
  }

  PointCloud point_cloud;
  zpp_deserialize(point_cloud).or_throw();
  return point_cloud;
}

PointCloud operator+(PointCloud const &lhs, PointCloud const &rhs) {
  std::vector<position> positions;
  std::vector<color> colors;

  const auto lhs_size = lhs.positions.size();
  const auto rhs_size = rhs.positions.size();

  positions.reserve(lhs_size + rhs_size);
  positions.insert(positions.end(), lhs.positions.begin(), lhs.positions.end());
  positions.insert(positions.end(), rhs.positions.begin(), rhs.positions.end());

  colors.reserve(lhs_size + rhs_size);
  colors.insert(colors.end(), lhs.colors.begin(), lhs.colors.end());
  colors.insert(colors.end(), rhs.colors.begin(), rhs.colors.end());

  return PointCloud{positions, colors};
}

PointCloud operator+=(PointCloud &lhs, PointCloud const &rhs) {
  const auto lhs_size = lhs.positions.size();
  const auto rhs_size = rhs.positions.size();

  lhs.positions.reserve(lhs_size + rhs_size);
  lhs.positions.insert(lhs.positions.end(), rhs.positions.begin(),
                       rhs.positions.end());

  lhs.colors.reserve(lhs_size + rhs_size);
  lhs.colors.insert(lhs.colors.end(), rhs.colors.begin(), rhs.colors.end());

  return lhs;
}

} // namespace bob::types
