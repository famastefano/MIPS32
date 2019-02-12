#include "Terminal.hpp"

void Terminal::print_integer( std::uint32_t value ) noexcept
{
  out_int = ( std::int32_t )value;
}

void Terminal::print_float( float value ) noexcept
{
  out_float = value;
}

void Terminal::print_double( double value ) noexcept
{
  out_double = value;
}

void Terminal::print_string( char const * string ) noexcept
{
  if ( !string )
    return;

  out_string = string;
}

void Terminal::read_integer( std::uint32_t * value ) noexcept
{
  if ( !value )
    return;

  *value = in_int;
}

void Terminal::read_float( float * value ) noexcept
{
  if ( !value )
    return;

  *value = in_float;
}

void Terminal::read_double( double * value ) noexcept
{
  if ( !value )
    return;

  *value = in_double;
}

void Terminal::read_string( char * string, std::uint32_t max_count ) noexcept
{
  if ( !string )
    return;

  if ( max_count <= in_string.size() )
    for ( std::uint32_t i = 0; i < max_count; ++i )
      string[i] = in_string[i];
  else
    for ( std::uint32_t i = 0; i < max_count; ++i )
      string[i] = '_';
}

Terminal::~Terminal()
{}
