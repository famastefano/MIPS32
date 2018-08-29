#pragma once

#include <cstdint>
#include <string_view>

constexpr std::uint32_t operator""_r1( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 6 );
}
constexpr std::uint32_t operator""_r2( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 11 );
}
constexpr std::uint32_t operator""_r3( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 16 );
}

constexpr std::uint32_t operator""_cp1( char const *c, std::size_t n ) noexcept
{
  using namespace std::literals;

  constexpr std::uint32_t opcode{0b010001 << 26};

  std::string_view view( c, n );

  constexpr std::string_view fmt_S_D_encode_table[] = {
      "ADD"sv,
      "SUB"sv,
      "MUL"sv,
      "DIV"sv,
      "SQRT"sv,
      "ABS"sv,
      "MOV"sv,
      "NEG"sv,
      "ROUND_L"sv,
      "TRUNC_L"sv,
      "CEIL_L"sv,
      "FLOOR_L"sv,
      "ROUND_W"sv,
      "TRUNC_W"sv,
      "CEIL_W"sv,
      "FLOOR_W"sv,
      "SEL"sv,
      "MOVCF"sv,
      "MOVZ"sv,
      "MOVN"sv,
      "SELEQZ"sv,
      "RECIP"sv,
      "RSQRT"sv,
      "SELNEZ"sv,
      "MADDF"sv,
      "MSUBF"sv,
      "RINT"sv,
      "CLASS"sv,
      "MIN"sv,
      "MAX"sv,
      "MINA"sv,
      "MAXA"sv,
      "CVT_S"sv,
      "CVT_D"sv,
      "_"sv, // reserved *
      "_"sv, // reserved *
      "CVT_W"sv,
      "CVT_L"sv,
      "CVT_PS"sv,
      "_"sv, // reserved *
  };

  constexpr std::string_view fmt_W_L_encode_table[] = {
      "CMP_AF"sv,
      "CMP_UN"sv,
      "CMP_EQ"sv,
      "CMP_UEQ"sv,
      "CMP_LT"sv,
      "CMP_ULT"sv,
      "CMP_LE"sv,
      "CMP_ULE"sv,
      "CMP_SAF"sv,
      "CMP_SUN"sv,
      "CMP_SEQ"sv,
      "CMP_SUEQ"sv,
      "CMP_SLT"sv,
      "CMP_SULT"sv,
      "CMP_SLE"sv,
      "CMP_SULE"sv,
      "_"sv, // reserved *
      "CMP_OR"sv,
      "CMP_UNE"sv,
      "CMP_NE"sv,
  };

  std::uint32_t code{0};
  for ( auto const &in : fmt_S_D_encode_table ) {
    if ( in == view ) return code | opcode;
    ++code;
  }

  code = 0;
  for ( auto const &in : fmt_W_L_encode_table ) {
    if ( in == view ) return code | opcode;
    ++code;
  }

  return std::uint32_t( 63 ); // function 63 is always reserved
}