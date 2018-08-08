#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace mips32
{

class RAM;
class Cache;

class MMU
{
public:
  struct Segment
  {
    static inline constexpr std::uint32_t KERNEL = 0x0000'0001;
    static inline constexpr std::uint32_t SUPERVISOR = 0x0000'0003;
    static inline constexpr std::uint32_t USER = 0x0000'0007;
    static inline constexpr std::uint32_t DEBUG = 0x0000'000F;
    static inline constexpr std::uint32_t CACHED = 0x0000'0010;

    std::uint32_t base_address,
      limit, access_flags;

    inline bool contains( std::uint32_t address ) const noexcept { return base_address <= address && address < base_address + limit; }
    inline bool has_access( std::uint32_t access_flags ) const noexcept { return this->access_flags & access_flags; }
  };

  MMU( RAM &ram, std::initializer_list<Segment> segments )
    noexcept;

  std::uint32_t *access( std::uint32_t address, std::uint32_t access_flags ) noexcept;

private:
  std::vector<Segment> segments;

  RAM &ram;
};
} // namespace mips32