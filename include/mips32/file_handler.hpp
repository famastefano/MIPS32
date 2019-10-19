#pragma once

#include <cstdint>

namespace mips32
{

/**
 * File Handler Interface
 * 
 * Interface used to simulate the file related syscalls
 * 
 * These are callbacks, that the CPU calls to perform File I/O.
 *
 * #!#!#!
 * The MIPS32 simulator relies on the correct implementation
 * of this interface. Every details is explained in the documentation.
 *
 * [READ THE DOCUMENTATION CAREFULLY]
 * !#!#!#
 **/
class FileHandler
{
public:
  
  // Requests to open a file
  // It adheres to the C `fopen`
  // 
  // Returns a number that will be used to identify the opened file
  virtual std::uint32_t open( char const *name, char const *flags ) noexcept = 0;

  // Requests to write up to `count` bytes into `dst`, by reading them from the file `fd`
  // 
  // Returns the number of bytes written to `dst`, negative if error
  // 
  // #!#!#!
  // [WARNING] It is undefined behaviour to write more than `count` bytes
  // !#!#!#
  virtual std::uint32_t read( std::uint32_t fd, char const *dst, std::uint32_t count ) noexcept = 0;

  // Requests to write up to `count` bytes from `src`, by writing them into the file `fd`
  // 
  // Returns the number of bytes written to `src`, negative if error
  // 
  // #!#!#!
  // [WARNING] It is undefined behaviour to read more than `count` bytes from `src`
  // !#!#!#
  virtual std::uint32_t write( std::uint32_t fd, char const *src, std::uint32_t count ) noexcept = 0;

  // Close a file
  virtual void close( std::uint32_t fd ) noexcept = 0;

  virtual ~FileHandler()
  {}
};
}