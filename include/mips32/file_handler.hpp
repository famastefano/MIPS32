#pragma once

#include <cstdint>

namespace mips32
{
class FileHandler
{
public:
  virtual std::uint32_t open( char const *name, char const *flags ) noexcept = 0;

  virtual std::uint32_t read( std::uint32_t fd, char const *dst, std::uint32_t count ) noexcept = 0;

  virtual std::uint32_t write( std::uint32_t fd, char const *src, std::uint32_t count ) noexcept = 0;

  virtual void close( std::uint32_t fd ) noexcept = 0;

  virtual ~FileHandler()
  {}
};
} // namespace mips32