#pragma once

#include "ram.hpp"

#include <vector>
#include <utility>

namespace mips32
{
class RAMIO
{
public:
  constexpr RAMIO( RAM &ram ) noexcept : ram( ram ) {}

  std::vector<char> read( std::uint32_t address, std::uint32_t count, bool read_string = false ) const noexcept;

  void write( std::uint32_t address, void const *src, std::uint32_t count ) noexcept;

private:
  std::pair<std::uint32_t, bool> get_block( std::uint32_t address ) const noexcept;

  RAM &ram;
};
} // namespace mips32