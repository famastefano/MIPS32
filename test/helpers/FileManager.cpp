#include "FileManager.hpp"

std::uint32_t FileManager::open( char const * name, char const * flags ) noexcept
{
  param.name = name;
  param.flags = flags;

  return fd_value;
}

std::uint32_t FileManager::read( std::uint32_t fd, char const * dst, std::uint32_t count ) noexcept
{
  param.fd = fd;
  param.dst = dst;
  param.count = count;

  return read_count;
}

std::uint32_t FileManager::write( std::uint32_t fd, char const * src, std::uint32_t count ) noexcept
{
  param.fd = fd;
  param.src = src;
  param.count = count;

  return write_count;
}

void FileManager::close( std::uint32_t fd ) noexcept
{
  param.fd = fd;
}

FileManager::~FileManager()
{}
