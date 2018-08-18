#pragma once

#include <mips32/file_handler.hpp>

#include <string>

class FileManager : public mips32::FileHandler
{
public:
  virtual std::uint32_t open( char const *name, char const *flags ) noexcept;

  virtual std::uint32_t read( std::uint32_t fd, char const *dst, std::uint32_t count ) noexcept;

  virtual std::uint32_t write( std::uint32_t fd, char const *src, std::uint32_t count ) noexcept;

  virtual void close( std::uint32_t fd ) noexcept;

  virtual ~FileManager();

  inline void reset()
  {
    param.name.clear();
    param.flags.clear();
    param.dst = param.src = nullptr;
    param.fd = param.count = 0;
  }

  struct Param
  {
    std::string name, flags;
    char const * dst, *src;
    std::uint32_t fd, count;
  };

  static constexpr std::uint32_t fd_value{ 0xBBBB'DDDD };
  static constexpr std::uint32_t read_count{ 42 };
  static constexpr std::uint32_t write_count{ 142 };

  Param param{};
};