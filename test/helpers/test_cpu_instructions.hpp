#pragma once

#include <cstdint>
#include <string_view>

constexpr std::uint32_t operator""_cpu( char const *s, std::size_t n ) noexcept
{
  using namespace std::literals;

  struct Info
  {
    std::string_view name;
    std::uint32_t    opcode;
  };

  constexpr Info opcode_table[] = {
      {"ADD"sv, 0b100'000 },
      {"ADDIU"sv, 0b001'001 << 26},
      {"ADDIUPC"sv, std::uint32_t( 0b111'011 ) << 26},
      {"ADDU"sv, 0b100'001},
      {"ALUIPC"sv, std::uint32_t( 0b111'011 ) << 26 | std::uint32_t( 0b11'111 ) << 16},
      {"AND"sv, 0b100'100},
      {"ANDI"sv, 0b001'100 << 26},
      {"AUI"sv, 0b001'111 << 26},
      {"AUIPC"sv, std::uint32_t( 0b111'011 ) << 26 | std::uint32_t( 0b11'110 ) << 16},
      {"BEQ"sv, 0b000'100 << 26},
      {"BAL"sv, 0b000'001 << 26 | 0b10'001 << 16},
      {"BALC"sv, std::uint32_t( 0b111010 ) << 26},
      {"BC"sv, std::uint32_t( 0b110010 ) << 26},
      {"BC1EQZ"sv, 0b010001 << 26 | 0b01'001 << 16},
      {"BC1NEZ"sv, 0b010001 << 26 | 0b01'101 << 16},
      {"BEQ"sv, 0b000'100 << 26},
      {"BGEZ"sv, 0b000'001 << 26 | 0b00'001 << 16},
      {"BLEZALC"sv, 0b000'110 << 26},
      {"BGEZALC"sv, 0b000'110 << 26},
      {"BGTZALC"sv, 0b000'111 << 26},
      {"BLTZALC"sv, 0b000'111 << 26},
      {"BEQZALC"sv, 0b001'000 << 26},
      {"BNEZALC"sv, 0b001'000 << 26},
      {"BLEZC"sv, 0b010'110 << 26},
      {"BGEZC"sv, 0b010'110 << 26},
      {"BGEC"sv, 0b010'110 << 26},
      {"BLEC"sv, 0b010'110 << 26},
      {"BGTZC"sv, 0b010'111 << 26},
      {"BLTZC"sv, 0b010'111 << 26},
      {"BLTC"sv, 0b010'111 << 26},
      {"BGTC"sv, 0b010'111 << 26},
      {"BGEUC"sv, 0b000'110 << 26},
      {"BLEUC"sv, 0b000'110 << 26},
      {"BLTUC"sv, 0b000'111 << 26},
      {"BGTUC"sv, 0b000'111 << 26},
      {"BEQC"sv, 0b001'000 << 26},
      {"BNEC"sv, 0b011'000 << 26},
      {"BEQZC"sv, std::uint32_t( 0b110'110 ) << 26},
      {"BNEZC"sv, std::uint32_t( 0b111'110 ) << 26},
      {"BGTZ"sv, 0b000'111 << 26},
      {"BLEZ"sv, 0b000'110 << 26},
      {"BLTZ"sv, 0b000'001 << 26},
      {"BNE"sv, 0b000'101 << 26},
      {"BOVC"sv, 0b001'000 << 26},
      {"BNVC"sv, 0b011'000 << 26},
      {"BREAK"sv, 0b001'101},
      {"CLO"sv, std::uint32_t( 0b1'010'001 )},
      {"CLZ"sv, std::uint32_t( 0b1'010'000 )},
      {"DI"sv, 0b010'000 << 26 | 0b01'011 << 21 | 0b01'100 << 11},
      {"DIV"sv, 0b00'010 << 6 | 0b011'010},
      {"MOD"sv, 0b00'011 << 6 | 0b011'010},
      {"DIVU"sv, 0b00'010 << 6 | 0b011'011},
      {"MODU"sv, 0b00'011 << 6 | 0b011'011},
      {"EI"sv, 0b010'000 << 26 | 0b01'011 << 21 | 0b01'100 << 11 | 1 << 5},
      {"ERET"sv, 0b010'000 << 26 | 1 << 25 | 0b011'000},
      {"EXT"sv, 0b011'111 << 26},
      {"INS"sv, 0b011'111 << 26 | 0b000'100},
      {"J"sv, 0b000'010 << 26},
      {"JAL"sv, 0b000'011 << 26},
      {"JALR"sv, 0b001'001},
      {"JIALC"sv, std::uint32_t( 0b111'110 ) << 26},
      {"JIC"sv, std::uint32_t( 0b110'110 ) << 26},
      {"JR"sv, 0b001'001},
      {"LB"sv, std::uint32_t( 0b100'000 ) << 26},
      {"LBU"sv, std::uint32_t( 0b100'100 ) << 26},
      {"LDC1"sv, std::uint32_t( 0b110'101 ) << 26},
      {"LH"sv, std::uint32_t( 0b100'001 ) << 26},
      {"LHU"sv, std::uint32_t( 0b100'101 ) << 26},
      {"LSA"sv, 0b000'101},
      {"LUI"sv, 0b001'111 << 26},
      {"LW"sv, std::uint32_t( 0b100'011 ) << 26 },
      {"LWC1"sv, std::uint32_t( 0b110'001 ) << 26 },
      {"LWPC"sv, std::uint32_t( 0b111'011 ) << 26 | 1 << 19},
      {"LWUPC"sv, std::uint32_t( 0b111'011 ) << 26 | 1 << 20},
      {"MFC0"sv, 0b010'000 << 26},
      {"MFC1"sv, 0b010'001 << 26},
      {"MFHC0"sv, 0b010'000 << 26 | 0b10 << 21},
      {"MFHC1"sv, 0b010'001 << 26 | 0b11 << 21},
      {"MTC0"sv, 0b010'000 << 26 | 0b100 << 21},
      {"MTC1"sv, 0b010'001 << 26 | 0b100 << 21},
      {"MTHC0"sv, 0b010'000 << 26 | 0b110 << 21},
      {"MTHC1"sv, 0b010'001 << 26 | 0b111 << 21},
      {"MUL"sv, 0b10 << 6 | 0b011'000},
      {"MUH"sv, 0b11 << 6 | 0b011'000},
      {"MULU"sv, 0b10 << 6 | 0b011'001},
      {"MUHU"sv, 0b11 << 6 | 0b011'001},
      {"NAL"sv, 1 << 26 | 1 << 20},
      {"NOP"sv, 0},
      {"NOR"sv, 0b100'111},
      {"OR"sv, 0b100'101},
      {"ORI"sv, 0b001'101 << 26},
      {"ROTR"sv, 1 << 21 | 0b10},
      {"ROTRV"sv, 1 << 6 | 0b110},
      {"SB"sv, std::uint32_t( 0b101'000 ) << 26},
      {"SDC1"sv, std::uint32_t( 0b111'101 ) << 26},
      {"SELEQZ"sv, 0b110'101},
      {"SELNEZ"sv, 0b110'111},
      {"SH"sv, std::uint32_t( 0b101'001 ) << 26},
      {"SIGRIE"sv, 1 << 26 | 0b10'111 << 16},
      {"SLL"sv, 0},
      {"SLLV"sv, 0b100},
      {"SLT"sv, 0b101'010},
      {"SLTI"sv, 0b001'010 << 26},
      {"SLTIU"sv, 0b001'011 << 26},
      {"SLTU"sv, 0b101'011},
      {"SRA"sv, 0b000'011},
      {"SRAV"sv, 0b000'111},
      {"SRL"sv, 0b10},
      {"SRLV"sv, 0b110},
      {"SUB"sv, 0b100'010},
      {"SUBU"sv, 0b100'011},
      {"SW"sv, std::uint32_t( 0b101'011 ) << 26},
      {"SWC1"sv, std::uint32_t( 0b111'001 ) << 26},
      {"SYSCALL"sv, 0b001'100},
      {"TEQ"sv, 0b110'100},
      {"TGE"sv, 0b110'000},
      {"TGEU"sv, 0b110'001},
      {"TLT"sv, 0b110'010},
      {"TLTU"sv, 0b110'011},
      {"TNE"sv, 0b110'110},
      {"XOR"sv, 0b100'110},
      {"XORI"sv, 0b001'110 << 26},
  };

  std::string_view _sv( s, n );
  for ( auto const &ins : opcode_table )
    if ( ins.name == _sv )
      return ins.opcode;

  return 0; // NOP
}
constexpr std::uint32_t operator""_rs( unsigned long long int v ) noexcept
{
  return std::uint32_t( v & 0x1F ) << 21;
}
constexpr std::uint32_t operator""_rt( unsigned long long int v ) noexcept
{
  return std::uint32_t( v & 0x1F ) << 16;
}
constexpr std::uint32_t operator""_rd( unsigned long long int v ) noexcept
{
  return std::uint32_t( v & 0x1F ) << 11;
}
constexpr std::uint32_t operator""_shamt( unsigned long long int v ) noexcept
{
  return std::uint32_t( v & 0x1F ) << 6;
}
constexpr std::uint32_t operator""_imm16( unsigned long long int v ) noexcept
{
  return std::uint32_t( v ) & 0xFFFF;
}
constexpr std::uint32_t operator""_imm19( unsigned long long int v ) noexcept
{
  return std::uint32_t( v ) & 0xF'FFFF;
}
constexpr std::uint32_t operator""_imm26( unsigned long long int v ) noexcept
{
  return std::uint32_t( v ) & 0x7FF'FFFF;
}