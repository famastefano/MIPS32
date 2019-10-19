#pragma once
#include <mips32/io_device.hpp>

#include <string>

class Terminal : public mips32::IODevice
{
public:
  virtual void print_integer( std::uint32_t value ) noexcept;

  virtual void print_float( float value ) noexcept;

  virtual void print_double( double value ) noexcept;

  virtual void print_string( char const *string ) noexcept;

  virtual void read_integer( std::uint32_t *value ) noexcept;

  virtual void read_float( float *value ) noexcept;

  virtual void read_double( double *value ) noexcept;

  virtual void read_string( char *string, std::uint32_t max_count ) noexcept;

  virtual ~Terminal();

  std::uint32_t in_int = 0xAAAA'BBBB;
  float in_float = 3.1415f;
  double in_double = 1994.0915;
  std::string in_string = "_";

  std::int32_t out_int = 0;
  float out_float = 0;
  double out_double = 0;
  char out_char = '\0';
  std::string out_string;
};