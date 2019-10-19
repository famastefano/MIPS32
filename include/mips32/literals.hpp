#pragma once

#include <cstdint>

namespace mips32::literals
{
constexpr std::uint32_t operator""_B( unsigned long long n )
{
  return ( std::uint32_t )n;
}

constexpr std::uint32_t operator""_KB( unsigned long long n )
{
  return ( std::uint32_t )( n * 1024ull );
}

constexpr std::uint32_t operator""_MB( unsigned long long n )
{
  return ( std::uint32_t )( n * 1024ull * 1024ull );
}

constexpr std::uint32_t operator""_GB( unsigned long long n )
{
  return ( std::uint32_t )( n * 1024ull * 1024ull * 1024ull );
}
} // namespace mips32::literals