#pragma once

#include <cstdint>

namespace mips32
{
/**
 * This interface is used to implement some syscalls
 * that communicate to "the external world", such as
 * terminal I/O (stdin, stdout, stderr, etc.) and so on.
 *
 * They work like callbacks, when that particular syscall
 * is executed, the CPU calls one of the following functions.
 *
 * #!#!#!
 * The MIPS32 simulator relies on the correct implementation
 * of this interface. Every details is explained in the documentation.
 *
 * [READ THE DOCUMENTATION CAREFULLY]
 * !#!#!#
 **/
class IODevice
{
public:
  // Writes an integer to the device
  virtual void print_integer( std::uint32_t value ) noexcept = 0;

  // Writes a float to the device
  virtual void print_float( float value ) noexcept = 0;

  // Writes a double to the device
  virtual void print_double( double value ) noexcept = 0;

  // Writes a string to the device
  //
  // #!#!#!
  // [WARNING] `string` could be NOT null terminated.
  // [WARNING] Check against null
  // !#!#!#
  virtual void print_string( char const *string ) noexcept = 0;

  // Reads an integer from the device and stores it inside `value`
  // `value` is guaranteed to point to a valid address.
  virtual void read_integer( std::uint32_t *value ) noexcept = 0;

  // Reads a float from the device and stores it inside `value`
  // `value` is guaranteed to point to a valid address.
  virtual void read_float( float *value ) noexcept = 0;

  // Reads a double from the device and stores it inside `value`
  // `value` is guaranteed to point to a valid address.
  virtual void read_double( double *value ) noexcept = 0;

  // Reads a string from the device and stores it into `string`.
  //
  // #!#!#!
  // [WARNING] Writing more than `max_count` characters is undefined behaviour.
  // [WARNING] Check against null
  // !#!#!#
  virtual void read_string( char *string, std::uint32_t max_count ) noexcept = 0;

  virtual ~IODevice()
  {}
};
} // namespace mips32
