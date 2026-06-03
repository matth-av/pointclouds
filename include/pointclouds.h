#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace bob::types {

struct uint2 {
  unsigned int x, y = 0;

  bool operator==(const uint2 other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const uint2 other) const { return !operator==(other); }
};

struct alignas(4) position {
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  int16_t __pad = 0;
};

constexpr bool operator==(const position &lhs, const position &rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z &&
         lhs.__pad == rhs.__pad;
}
constexpr bool operator!=(const position &lhs, const position &rhs) {
  return !(rhs == lhs);
}

struct color {
  unsigned char r, g, b, a = 0;

  bool operator==(const color other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
  }
  bool operator!=(const color other) const { return !operator==(other); }
};

struct position_bounds {
  position min{(std::numeric_limits<int16_t>::max)(),
               (std::numeric_limits<int16_t>::max)(),
               (std::numeric_limits<int16_t>::max)(), 0};
  position max{(std::numeric_limits<int16_t>::min)(),
               (std::numeric_limits<int16_t>::min)(),
               (std::numeric_limits<int16_t>::min)(), 0};
};

using bytes = std::vector<std::byte>;

class PointCloud {
public:
  std::vector<position> positions;
  std::vector<color> colors;
  position_bounds bounds;

  auto size() const { return positions.size(); }
  auto empty() const { return positions.empty(); }

  bytes serialize(bool compress = false) const;
  static PointCloud deserialize(std::span<const std::byte> buffer);

private:
  bytes compress() const;
  static PointCloud decompress(const bytes &buffer, unsigned long point_count);
};

PointCloud operator+(PointCloud const &lhs, PointCloud const &rhs);
PointCloud operator+=(PointCloud &lhs, const PointCloud &rhs);

struct PointCloudPacket {
  // out packet needs these explicitly sized types to ensure portability
  // between unix and windows systems
  uint64_t timestamp;
  uint64_t point_count;
  uint8_t compressed;
  bytes data;
};

} // namespace bob::types
