#pragma once

#include <cstdint>

namespace mips32
{
// Floating Point register
union FPR
{
  float         f;
  std::uint32_t i32;

  double        d;
  std::uint64_t i64;
};
} // namespace mips32
