#pragma once

#include <cstdint>

namespace mips32
{
struct Header
{
  bool          valid{ false };
  bool          dirty{ false };
  std::uint32_t tag;
};
} // namespace mips32
