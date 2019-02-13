#include <catch.hpp>

#include "../src/cpu.hpp"
#include <mips32/machine_inspector.hpp>

#include "helpers/test_cpu_instructions.hpp"
#include "helpers/Terminal.hpp"
#include "helpers/FileManager.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

// TODO: test for reserved(word) path
// TODO: test BNEZALC
// TODO: test for [L|S][B|H|W] with address overflow and $zero as destination

using namespace mips32;
using namespace mips32::literals;

using ui32 = std::uint32_t;

#define R(n) inspector.CPU_gpr_begin() + n
#define FP(n) inspector.CP1_fpr_begin() + n
#define PC() inspector.CPU_pc()
#define $start ram[0xBFC0'0000]
#define CAUSE() cp0.cause
#define ClearExCause() CAUSE() = CAUSE() & ~0x7C
#define ExCause() (CAUSE() >> 2 & 0x1F)
#define HasTrapped() (ExCause() == 13)
#define HasOverflowed() (ExCause() == 12)

// SYSCALL helpers

constexpr int _v0 = 2;
constexpr int _v1 = 3;

constexpr int _a0 = 4;
constexpr int _a1 = 5;
constexpr int _a2 = 6;

enum SYSCALL
{
  PRINT_INT = 1,
  PRINT_FLOAT,
  PRINT_DOUBLE,
  PRINT_STRING,
  READ_INT,
  READ_FLOAT,
  READ_DOUBLE,
  READ_STRING,
  SBRK,
  EXIT,
  PRINT_CHAR,
  READ_CHAR,
  OPEN,
  READ,
  WRITE,
  CLOSE,
  EXIT2,
};

TEST_CASE( "A CPU object exists" )
{
  std::unique_ptr<Terminal> terminal = std::make_unique<Terminal>();
  std::unique_ptr<FileManager> filehandler = std::make_unique<FileManager>();

  MachineInspector inspector;

  RAM ram{ 192_KB }; // 3 Blocks
  CPU cpu{ ram };

  inspector
    .inspect( ram )
    .inspect( cpu );

  cpu.hard_reset();

  cpu.attach_iodevice( terminal.get() );
  cpu.attach_file_handler( filehandler.get() );
  filehandler->reset();

  auto & cp0 = inspector.access_CP0();

  auto const pc = PC();

  ClearExCause();

  SECTION( "ADD $1, $2, $3 is executed" )
  {
    auto const _add = "ADD"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -48;
    *$3 = 21;

    ui32 const res = -48 + 21;

    $start = _add;
    cpu.single_step();
    REQUIRE( *$1 == res );
  }

  SECTION( "ADDIU $21, $3, 32'000 is executed" )
  {
    auto const _addiu = "ADDIU"_cpu | 21_rt | 3_rs | 32000_imm16;

    auto $21 = R( 21 );
    auto $3 = R( 3 );

    *$3 = 123'098;

    ui32 const res = 123'098 + 32'000;

    $start = _addiu;
    cpu.single_step();
    REQUIRE( *$21 == res );
  }

  SECTION( "ADDIUPC $30, 16 is executed" )
  {
    auto const _addiupc = "ADDIUPC"_cpu | 30_rs | 16_imm19;

    auto $30 = R( 30 );

    ui32 const res = pc + ( 16 << 2 );

    $start = _addiupc;
    cpu.single_step();
    REQUIRE( *$30 == res );
  }

  SECTION( "ADDU $6, $8, $10 is executed" )
  {
    auto const _addu = "ADDU"_cpu | 6_rd | 8_rs | 10_rt;

    auto $6 = R( 6 );
    auto $8 = R( 8 );
    auto $10 = R( 10 );

    *$8 = 305;
    *$10 = 3894;

    ui32 const res = 305 + 3894;

    $start = _addu;
    cpu.single_step();
    REQUIRE( *$6 == res );
  }

  SECTION( "ALUIPC $1, 256 is executed" )
  {
    auto const _aluipc = "ALUIPC"_cpu | 1_rs | 256;

    auto $1 = R( 1 );

    auto const res = 0xFFFF'0000 & ( pc + ( 256 << 16 ) );

    $start = _aluipc;
    cpu.single_step();
    REQUIRE( *$1 == res );
  }

  SECTION( "AND $5, $10, $15 is executed" )
  {
    auto const _and = "AND"_cpu | 5_rd | 10_rs | 15_rt;

    auto $5 = R( 5 );
    auto $10 = R( 10 );
    auto $15 = R( 15 );

    *$10 = 0xFFFF'FFFF;
    *$15 = 0b1010'1010'1010'1010'1010'1010'1010'1010;

    ui32 const res = *$10 & *$15;

    $start = _and;
    cpu.single_step();
    REQUIRE( *$5 == res );
  }

  SECTION( "ANDI $4, $18 is executed" )
  {
    auto const _andi = "ANDI"_cpu | 4_rt | 18_rs | 0xCEED_imm16;

    auto $4 = R( 4 );
    auto $18 = R( 18 );

    *$18 = 0xFFFF'FFFF;

    ui32 const res = 0xFFFF'FFFF & 0xCEED;

    $start = _andi;
    cpu.single_step();
    REQUIRE( *$4 == res );
  }

  SECTION( "AUI $8, $20 is executed" )
  {
    auto const _aui = "AUI"_cpu | 8_rt | 20_rs | 0xCCAA_imm16;

    auto $8 = R( 8 );
    auto $20 = R( 20 );

    *$20 = 0;

    ui32 const res = 0xCCAA'0000;

    $start = _aui;
    cpu.single_step();
    REQUIRE( *$8 == res );
  }

  SECTION( "AUIPC $2, 36 is executed" )
  {
    auto const _auipc = "AUIPC"_cpu | 2_rs | 36_imm16;

    auto $2 = R( 2 );

    ui32 const res = pc + ( 36 << 16 );

    $start = _auipc;
    cpu.single_step();
    REQUIRE( *$2 == res );
  }

  SECTION( "BAL 768 is executed" )
  {
    auto const _bal = "BAL"_cpu | 768_imm16;

    auto $31 = R( 31 );

    ui32 const res = pc + 4 + ( 768 << 2 );
    ui32 const ret = pc + 8;

    $start = _bal;
    cpu.single_step();
    REQUIRE( PC() == res );
    REQUIRE( *$31 == ret );
  }

  SECTION( "BALC 0x0123'4567 is executed" )
  {
    auto const _balc = "BALC"_cpu | 0x01234567_imm26;

    auto $31 = inspector.CPU_gpr_begin() + 31;

    ui32 const res = pc + 4 + ( 0x01234567 << 2 );
    ui32 const ret = pc + 4;

    $start = _balc;
    cpu.single_step();
    REQUIRE( PC() == res );
    REQUIRE( *$31 == ret );
  }

  SECTION( "BC 0x02BC'DEFF is executed" )
  {
    auto const _bc = "BC"_cpu | 0x02BCDEFF_imm26;

    ui32 const res = pc + 4 + 0xFAF3'7BFC;

    $start = _bc;
    cpu.single_step();
    REQUIRE( PC() == res );
  }

  SECTION( "BEQ $0, $0, 81 and BEQ $1, $2, 81 are executed" )
  {
    auto const _beq_jump = "BEQ"_cpu | 0_rs | 0_rt | 81_imm16;
    auto const _beq_no_jump = "BEQ"_cpu | 1_rs | 2_rt | 81_imm16;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$1 = 44;
    *$2 = -44;

    ui32 const res_jump = pc + 4 + ( 81 << 2 );
    ui32 const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _beq_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _beq_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGEZ $30, 128 and BGEZ $28, 128 are executed" )
  {
    auto const _bgez_jump = "BGEZ"_cpu | 30_rs | 128_imm16;
    auto const _bgez_no_jump = "BGEZ"_cpu | 28_rs | 128_imm16;

    auto $30 = R( 30 );

    auto $28 = R( 28 );

    *$30 = 0;
    *$28 = -1;

    ui32 const res_jump = pc + 4 + ( 128 << 2 );
    ui32 const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgez_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgez_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BLEZALC $9, 1 and BLEZALC $12, 256 are executed" )
  {
    auto const _blezalc_jump = "BLEZALC"_cpu | 9_rt | 1_imm16;
    auto const _blezalc_no_jump = "BLEZALC"_cpu | 12_rt | 256_imm16;

    auto $9 = R( 9 );

    auto $12 = R( 12 );

    auto $31 = R( 31 );

    *$9 = 0;
    *$12 = 24;

    auto const res_jump = pc + 4 + ( 1 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _blezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _blezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BGEZALC $13, 14 and BGEZALC $19, 964 are executed" )
  {
    auto const _bgezalc_jump = "BGEZALC"_cpu | 13_rs | 13_rt | 14_imm16;
    auto const _bgezalc_no_jump = "BGEZALC"_cpu | 19_rs | 19_rt | 964_imm16;

    auto $13 = R( 13 );
    auto $19 = R( 19 );

    auto $31 = R( 31 );

    *$13 = 534;

    *$19 = -3498;

    auto const res_jump = pc + 4 + ( 14 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BGTZALC $8, 97 and BGTZALC $1, 1000 are executed" )
  {
    auto const _bgtzalc_jump = "BGTZALC"_cpu | 8_rt | 97_imm16;
    auto const _bgtzalc_no_jump = "BGTZALC"_cpu | 1_rt | 1000_imm16;

    auto $8 = R( 8 );

    auto $1 = R( 1 );

    auto $31 = R( 31 );

    *$8 = 534;
    *$1 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 97 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgtzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgtzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BLTZALC $17, 77 and BLTZALC $20, 93 are executed" )
  {
    auto const _bltzalc_jump = "BLTZALC"_cpu | 17_rs | 17_rt | 77_imm16;
    auto const _bltzalc_no_jump = "BLTZALC"_cpu | 20_rs | 20_rt | 93_imm16;

    auto $17 = R( 17 );
    auto $20 = R( 20 );

    auto $31 = R( 31 );

    *$17 = -8947;

    *$20 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 77 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bltzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bltzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BEQZALC $24, 816 and BEQZALC $11, 789 are executed" )
  {
    auto const _beqzalc_jump = "BEQZALC"_cpu | 24_rt | 816_imm16;
    auto const _beqzalc_no_jump = "BEQZALC"_cpu | 11_rt | 789_imm16;

    auto $24 = R( 24 );
    auto $11 = R( 11 );

    auto $31 = R( 31 );

    *$24 = 0;

    *$11 = 1;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 816 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _beqzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }

    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _beqzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BNEZALC $17, 1598 and BNEZALC $20, 795 are executed" )
  {
    auto const _bnezalc_jump = "BNEZALC"_cpu | 17_rt | 1598_imm16;
    auto const _bnezalc_no_jump = "BNEZALC"_cpu | 20_rt | 795_imm16;

    auto $17 = R( 17 );
    auto $20 = R( 20 );

    auto $31 = R( 31 );

    *$17 = 0;

    *$20 = 1;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1598 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bnezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bnezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  SECTION( "BLEZC $18, 8 and BLEZC $4, 6 are executed" )
  {
    auto const _blezc_jump = "BLEZC"_cpu | 18_rt | 8_imm16;
    auto const _blezc_no_jump = "BLEZC"_cpu | 4_rt | 6_imm16;

    auto $18 = R( 18 );

    auto $4 = R( 4 );

    *$18 = 0;

    *$4 = 12;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 8 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _blezc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _blezc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGEZC $15, 99 and BGEZC $1, 17 are executed" )
  {
    auto const _bgezc_jump = "BGEZC"_cpu | 15_rs | 15_rt | 99_imm16;
    auto const _bgezc_no_jump = "BGEZC"_cpu | 1_rs | 1_rt | 17_imm16;

    auto $15 = R( 15 );

    auto $1 = R( 1 );

    *$15 = 5234;
    *$1 = -564;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 99 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgezc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgezc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGEC $31, $3, 905 and BGEC $30, $20, 54 are executed" )
  {
    auto const _bgec_jump = "BGEC"_cpu | 31_rs | 3_rt | 905_imm16;
    auto const _bgec_no_jump = "BGEC"_cpu | 30_rs | 20_rt | 54_imm16;

    auto $31 = R( 31 );
    auto $3 = R( 3 );

    auto $30 = R( 30 );
    auto $20 = R( 20 );

    *$31 = 32;
    *$3 = -923;

    *$30 = -98'734;
    *$20 = -98'000;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 905 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BLEC $31, $3, 905 and BLEC $30, $20, 54 are executed" )
  {
    auto const _blec_jump = "BLEC"_cpu | 31_rt | 3_rs | 905_imm16;
    auto const _blec_no_jump = "BLEC"_cpu | 30_rt | 20_rs | 54_imm16;

    auto $31 = R( 31 );
    auto $3 = R( 3 );

    auto $30 = R( 30 );
    auto $20 = R( 20 );

    *$31 = -923;
    *$3 = 32;

    *$30 = -98'000;
    *$20 = -98'734;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 905 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _blec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _blec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGTZC $20, 1111 and BGTZC $6, 25'896 are executed" )
  {
    auto const _bgtzc_jump = "BGTZC"_cpu | 20_rt | 1111_imm16;
    auto const _bgtzc_no_jump = "BGTZC"_cpu | 6_rt | 25896_imm16;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 65;

    *$6 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgtzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgtzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }
  SECTION( "BLTZC $20, 1111 and BLTZC $6, 25'896 are executed" )
  {
    auto const _bltzc_jump = "BLTZC"_cpu | 20_rs | 20_rt | 1111_imm16;
    auto const _bltzc_no_jump = "BLTZC"_cpu | 6_rs | 6_rt | 25896_imm16;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = -20;

    *$6 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bltzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bltzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BLTC $20, $21, 1111 and BLTC $6, $7, 25'896 are executed" )
  {
    auto const _bltc_jump = "BLTC"_cpu | 20_rs | 21_rt | 1111_imm16;
    auto const _bltc_no_jump = "BLTC"_cpu | 6_rs | 7_rt | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = -3490;
    *$21 = 854;

    *$6 = -65;
    *$7 = -65;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bltc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bltc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGTC $20, $21, 1111 and BGTC $6, $7, 25'896 are executed" )
  {
    auto const _bgtc_jump = "BGTC"_cpu | 20_rt | 21_rs | 1111_imm16;
    auto const _bgtc_no_jump = "BGTC"_cpu | 6_rt | 7_rs | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = 0;
    *$21 = -1;

    *$6 = 11;
    *$7 = 12;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgtc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgtc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGEUC $20, $21, 1111 and BGEUC $6, $7, 25'896 are executed" )
  {
    auto const _bgeuc_jump = "BGEUC"_cpu | 20_rs | 21_rt | 1111_imm16;
    auto const _bgeuc_no_jump = "BGEUC"_cpu | 6_rs | 7_rt | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = -1;
    *$21 = -1;

    *$6 = 11;
    *$7 = 12;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgeuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgeuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BLEUC $20, $21, 1111 and BLEUC $6, $7, 25'896 are executed" )
  {
    auto const _bleuc_jump = "BLEUC"_cpu | 20_rt | 21_rs | 1111_imm16;
    auto const _bleuc_no_jump = "BLEUC"_cpu | 6_rt | 7_rs | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = 0;
    *$21 = -1;

    *$6 = 1;
    *$7 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bleuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bleuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BLTUC $20, $21, 1111 and BLTUC $6, $7, 25'896 are executed" )
  {
    auto const _bltuc_jump = "BLTUC"_cpu | 20_rs | 21_rt | 1111_imm16;
    auto const _bltuc_no_jump = "BLTUC"_cpu | 6_rs | 7_rt | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = 0;
    *$21 = -1;

    *$6 = 11;
    *$7 = 11;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bltuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bltuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BGTUC $20, $21, 1111 and BGTUC $6, $7, 25'896 are executed" )
  {
    auto const _bgtuc_jump = "BGTUC"_cpu | 20_rt | 21_rs | 1111_imm16;
    auto const _bgtuc_no_jump = "BGTUC"_cpu | 6_rt | 7_rs | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = -1;
    *$21 = 0;

    *$6 = 11;
    *$7 = 11;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bgtuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bgtuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BEQC $20, $21, 1111 and BEQC $6, $7, 25'896 are executed" )
  {
    auto const _beqc_jump = "BEQC"_cpu | 20_rs | 21_rt | 1111_imm16;
    auto const _beqc_no_jump = "BEQC"_cpu | 6_rs | 7_rt | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = 1;
    *$21 = 1;

    *$6 = 0;
    *$7 = 11;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _beqc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _beqc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BNEC $20, $21, 1111 and BNEC $6, $7, 25'896 are executed" )
  {
    auto const _bnec_jump = "BNEC"_cpu | 20_rs | 21_rt | 1111_imm16;
    auto const _bnec_no_jump = "BNEC"_cpu | 6_rs | 7_rt | 25896_imm16;

    auto $20 = R( 20 );
    auto $21 = R( 21 );

    auto $6 = R( 6 );
    auto $7 = R( 7 );

    *$20 = 0;
    *$21 = -1;

    *$6 = 4123;
    *$7 = 4123;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bnec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bnec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BEQZC $20, 0xF'FFFF and BEQZC $6, 0xA'BCDE are executed" )
  {
    auto const _beqzc_jump = "BEQZC"_cpu | 20_rs | 0xF'FFFF;
    auto const _beqzc_no_jump = "BEQZC"_cpu | 6_rs | 0xA'BCDE;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 0;

    *$6 = -423;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 0xF'FFFF << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _beqzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _beqzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BNEZC $20, 0xA'BCDE and BNEZC $6, 21 are executed" )
  {
    auto const _beqzc_jump = "BNEZC"_cpu | 20_rs | 0xA'BCDE;
    auto const _beqzc_no_jump = "BNEZC"_cpu | 6_rs | 21;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 1;

    *$6 = 0;

    PC() = pc;

    auto const res_jump = pc + 4 + ( 0xA'BCDE << 2 );
    auto const res_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _beqzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _beqzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  SECTION( "BOVC $2, $1, 256 and BOVC $4, $3, 1024 are executed" )
  {
    auto const _bovc_jump = "BOVC"_cpu | 2_rs | 1_rt | 256;
    auto const _bovc_no_jump = "BOVC"_cpu | 4_rs | 3_rt | 1024;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = 0x8000'0000;
    *$2 = 0x8000'0000;

    *$3 = 1;
    *$4 = 1;

    PC() = pc;

    auto const pc_jump = pc + 4 + ( 256 << 2 );
    auto const pc_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bovc_jump;
      cpu.single_step();

      REQUIRE( PC() == pc_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bovc_no_jump;
      cpu.single_step();

      REQUIRE( PC() == pc_no_jump );
    }
  }

  SECTION( "BNVC $2, $1, 256 and BNVC $4, $3, 1024 are executed" )
  {
    auto const _bnvc_jump = "BNVC"_cpu | 2_rs | 1_rt | 256;
    auto const _bnvc_no_jump = "BNVC"_cpu | 4_rs | 3_rt | 1024;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = 1;
    *$2 = 1;

    *$3 = 0x8000'0000;
    *$4 = 0x8000'0000;

    PC() = pc;

    auto const pc_jump = pc + 4 + ( 256 << 2 );
    auto const pc_no_jump = pc + 4;

    SECTION( "It shall jump in the 1st case" )
    {
      $start = _bnvc_jump;
      cpu.single_step();

      REQUIRE( PC() == pc_jump );
    }
    SECTION( "It shall not jump in the 2nd case" )
    {
      $start = _bnvc_no_jump;
      cpu.single_step();

      REQUIRE( PC() == pc_no_jump );
    }
  }

  SECTION( "BREAK is executed" )
  {
    $start = "BREAK"_cpu;

    REQUIRE( cpu.single_step() == 3 );
  }

  SECTION( "CLO $10, $15 and CLO $21, $0 are executed" )
  {
    auto const _clo_15 = "CLO"_cpu | 10_rd | 15_rs;
    auto const _clo_0 = "CLO"_cpu | 21_rd | 0_rs;

    auto $10 = R( 10 );
    auto $15 = R( 15 );

    auto $21 = R( 21 );

    PC() = pc;

    auto const res_EBF8_9D0A = 3;
    auto const res_FFFF_FFFF = 32;
    auto const res_0000_0000 = 0;

    SECTION( "0xEBF8'9D0A shall return 3" )
    {
      $start = _clo_15;
      *$15 = 0xEBF8'9D0A;
      cpu.single_step();
      REQUIRE( *$10 == res_EBF8_9D0A );
    }
    SECTION( "0xFFFF'FFFF shall return 32" )
    {
      $start = _clo_15;
      *$15 = 0xFFFF'FFFF;
      cpu.single_step();
      REQUIRE( *$10 == res_FFFF_FFFF );
    }
    SECTION( "0x0000'0000 shall return 0" )
    {
      $start = _clo_0;
      cpu.single_step();
      REQUIRE( *$21 == res_0000_0000 );
    }
  }

  SECTION( "CLZ $10, $15 and CLZ $21, $0 are executed" )
  {
    auto const _clo_15 = "CLZ"_cpu | 10_rd | 15_rs;
    auto const _clo_0 = "CLZ"_cpu | 21_rd | 0_rs;

    auto $10 = R( 10 );
    auto $15 = R( 15 );

    auto $21 = R( 21 );

    PC() = pc;

    auto const res_0604_7FEB = 5;
    auto const res_FFFF_FFFF = 0;
    auto const res_0000_0000 = 32;

    SECTION( "0x0604'7FEB shall return 5" )
    {
      $start = _clo_15;
      *$15 = 0x0604'7FEB;
      cpu.single_step();
      REQUIRE( *$10 == res_0604_7FEB );
    }
    SECTION( "0xFFFF'FFFF shall return 0" )
    {
      $start = _clo_15;
      *$15 = 0xFFFF'FFFF;
      cpu.single_step();
      REQUIRE( *$10 == res_FFFF_FFFF );
    }
    SECTION( "0x0000'0000 shall return 32" )
    {
      $start = _clo_0;
      cpu.single_step();
      REQUIRE( *$21 == res_0000_0000 );
    }
  }

  SECTION( "DI is executed" )
  {
    cp0.status |= 1;
    $start = "DI"_cpu;
    cpu.single_step();

    auto const int_disabled = cp0.status & 1;
    REQUIRE( int_disabled == 0 );
  }

  SECTION( "EI is executed" )
  {
    cp0.status &= ~1;

    $start = "EI"_cpu;

    cpu.single_step();

    auto const int_enabled = cp0.status & 1;
    REQUIRE( int_enabled == 1 );
  }

  SECTION( "DIV $1, $2, $3 is executed" )
  {
    auto const _div = "DIV"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -10;
    *$3 = 5;

    ui32 const res = -10 / 5;

    $start = _div;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "DIVU $1, $2, $3 is executed" )
  {
    auto const _divu = "DIVU"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 17;
    *$3 = -4;

    ui32 const res = 17 / ( ui32 )-4;

    $start = _divu;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "MOD $1, $2, $3 is executed" )
  {
    auto const _mod = "MOD"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 241;
    *$3 = -25;

    auto const res = 241 % -25;

    $start = _mod;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "MODU $1, $2, $3 is executed" )
  {
    auto const _mod = "MODU"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 3498;
    *$3 = -95;

    auto const res = 3498 % ( ui32 )-95;

    $start = _mod;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "ERET is executed to return from an interrupt/exception" )
  {
    $start = "SIGRIE"_cpu;
    ram[0x8000'0180] = "ERET"_cpu;

    cpu.single_step(); // sigrie
    REQUIRE( PC() == 0x8000'0180 );
    cpu.single_step(); // eret
    REQUIRE( PC() == 0xBFC0'0000 );
  }

  SECTION( "EXT $1, $2, 14, 8 is executed" )
  {
    auto const _ext = "EXT"_cpu | 1_rt | 2_rs | 14_shamt | 7_rd;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0x001F'C000;

    auto const res = 0x7F;

    $start = _ext;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "INS $1, $2, 19, 4 is executed" )
  {
    auto const _ins = "INS"_cpu | 1_rt | 2_rs | 19_shamt | 22_rd;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$1 = 0x0000'0000;
    *$2 = 0xF;

    auto const res = 0x0078'0000;

    $start = _ins;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "J 0x0279'DB24 is executed" )
  {
    auto const _j = "J"_cpu | 0x0279'DB24;

    auto const res = PC() & 0xF000'0000 | 0x0279'DB24 << 2;

    $start = _j;

    cpu.single_step();

    REQUIRE( PC() == res );
  }

  SECTION( "JAL 2389 is executed" )
  {
    auto const _jal = "JAL"_cpu | 2389;

    auto $31 = R( 31 );

    PC() = pc;
    auto const res = pc & 0xF000'0000 | 2389 << 2;

    $start = _jal;

    cpu.single_step();

    REQUIRE( PC() == res );
    REQUIRE( *$31 == pc + 8 );
  }

  SECTION( "JALR $1 and JALR $1, $2 are executed" )
  {
    auto const _jalr_31 = "JALR"_cpu | 1_rs | 31_rd;
    auto const _jalr_2 = "JALR"_cpu | 1_rs | 2_rd;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $31 = R( 31 );

    *$1 = 0x8000'0000;

    PC() = pc;
    auto const ret = pc + 8;

    SECTION( "It shall store pc into $31 in the 1st case" )
    {
      $start = _jalr_31;
      cpu.single_step();

      REQUIRE( PC() == 0x8000'0000 );
      REQUIRE( *$31 == ret );
    }
    SECTION( "It shall store pc into $2 in the 2nd case" )
    {
      $start = _jalr_2;
      cpu.single_step();

      REQUIRE( PC() == 0x8000'0000 );
      REQUIRE( *$2 == ret );
    }
  }

  SECTION( "JIALC $4, 21 is executed" )
  {
    auto const _jialc = "JIALC"_cpu | 4_rt | 21;

    auto $4 = R( 4 );
    auto $31 = R( 31 );

    *$4 = 0x8000'0000;

    auto const ret = pc + 4;
    auto const new_pc = 0x8000'0000 + 21;

    $start = _jialc;
    cpu.single_step();

    REQUIRE( PC() == new_pc );
    REQUIRE( *$31 == ret );
  }

  SECTION( "JIC $16, 9345 is executed" )
  {
    auto const _jialc = "JIALC"_cpu | 4_rt | 9345;

    auto $4 = R( 4 );
    auto $31 = R( 31 );

    *$4 = 0xABCD'0000;

    auto const ret = pc + 4;
    auto const new_pc = 0xABCD'0000 + 9345;

    $start = _jialc;
    cpu.single_step();

    REQUIRE( PC() == new_pc );
    REQUIRE( *$31 == ret );
  }

  SECTION( "JIC $1, 51 is executed" )
  {
    auto const _jic = "JIC"_cpu | 1_rt | 51;

    auto $1 = R( 1 );

    *$1 = 0xAE00'0000;

    auto const res = 0xAE00'0000 + 51;

    $start = _jic;
    cpu.single_step();

    REQUIRE( PC() == 0xAE00'0033 );
  }

  SECTION( "JR $31 is executed" )
  {
    auto const _jr = "JR"_cpu | 31_rs;

    auto $31 = R( 31 );

    *$31 = 0x0024'3798;

    $start = _jr;
    cpu.single_step();

    REQUIRE( PC() == 0x0024'3798 );
  }

  //// Signed Load

  SECTION( "LB $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _lb_0 = "LB"_cpu | 1_rt | 0 | 2_rs;
    auto const _lb_1 = "LB"_cpu | 1_rt | 1 | 2_rs;
    auto const _lb_2 = "LB"_cpu | 1_rt | 2 | 2_rs;
    auto const _lb_3 = "LB"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$2 = 0x8000'0000;
    ram[0x8000'0000] = 0xABCD'EF12;

    // Remember that LB sign extends before writing to rt

    SECTION( "0($2) should load 0x12" )
    {
      $start = _lb_0;
      cpu.single_step();
      REQUIRE( *$1 == 0x12 );
    }
    SECTION( "1($2) should load 0xEF" )
    {
      $start = _lb_1;
      cpu.single_step();
      REQUIRE( *$1 == 0xFF'FF'FF'EF );
    }
    SECTION( "2($2) should load 0xCD" )
    {
      $start = _lb_2;
      cpu.single_step();
      REQUIRE( *$1 == 0xFF'FF'FF'CD );
    }
    SECTION( "3($2) should load 0xAB" )
    {
      $start = _lb_3;
      cpu.single_step();
      REQUIRE( *$1 == 0xFF'FF'FF'AB );
    }
  }

  SECTION( "LH $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _lh_0 = "LH"_cpu | 1_rt | 0 | 2_rs;
    auto const _lh_1 = "LH"_cpu | 1_rt | 1 | 2_rs;
    auto const _lh_2 = "LH"_cpu | 1_rt | 2 | 2_rs;
    auto const _lh_3 = "LH"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$2 = 0x8000'0000;
    ram[0x8000'0000] = 0xABCD'EF12;
    ram[0x8000'0004] = 0x3456'7890;

    // Remember that LH sign extends before writing to rt

    SECTION( "0($2) should load 0xFFFF'EF12" )
    {
      $start = _lh_0;
      cpu.single_step();
      REQUIRE( *$1 == 0xFFFF'EF12 );
    }
    SECTION( "1($2) should load 0xFFFF'CDEF" )
    {
      $start = _lh_1;
      cpu.single_step();
      REQUIRE( *$1 == 0xFFFF'CDEF );
    }
    SECTION( "2($2) should load 0xFFFF'ABCD" )
    {
      $start = _lh_2;
      cpu.single_step();
      REQUIRE( *$1 == 0xFFFF'ABCD );
    }
    SECTION( "3($2) should load 0xFFFF'90AB" )
    {
      $start = _lh_3;
      cpu.single_step();
      REQUIRE( *$1 == 0xFFFF'90AB );
    }
  }

  SECTION( "LW $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _lw_0 = "LW"_cpu | 1_rt | 0 | 2_rs;
    auto const _lw_1 = "LW"_cpu | 1_rt | 1 | 2_rs;
    auto const _lw_2 = "LW"_cpu | 1_rt | 2 | 2_rs;
    auto const _lw_3 = "LW"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$2 = 0x8000'0000;
    ram[0x8000'0000] = 0xABCD'EF12;
    ram[0x8000'0004] = 0x3456'7890;

    SECTION( "0($2) should load 0xABCD'EF12" )
    {
      $start = _lw_0;
      cpu.single_step();
      REQUIRE( *$1 == 0xABCD'EF12 );
    }
    SECTION( "1($2) should load 0x90AB'CDEF" )
    {
      $start = _lw_1;
      cpu.single_step();
      REQUIRE( *$1 == 0x90AB'CDEF );
    }
    SECTION( "2($2) should load 0x7890'ABCD" )
    {
      $start = _lw_2;
      cpu.single_step();
      REQUIRE( *$1 == 0x7890'ABCD );
    }
    SECTION( "3($2) should load 0x3456'7890" )
    {
      $start = _lw_3;
      cpu.single_step();
      REQUIRE( *$1 == 0x5678'90AB );
    }
  }

  SECTION( "LWC1 $f0, n($1) is executed with n = {0,1,2,3}" )
  {
    auto const _lwc1_0 = "LWC1"_cpu | 0_rt | 1_rs | 0;
    auto const _lwc1_1 = "LWC1"_cpu | 0_rt | 1_rs | 1;
    auto const _lwc1_2 = "LWC1"_cpu | 0_rt | 1_rs | 2;
    auto const _lwc1_3 = "LWC1"_cpu | 0_rt | 1_rs | 3;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    *$1 = 0x8000'0000;

    ram[0x8000'0000] = 0xDDDD'EEEE;
    ram[0x8000'0004] = 0xAAAA'BBBB;

    PC() = pc;

    SECTION( "0($1) should load 0xDDDD'EEEE" )
    {
      $start = _lwc1_0;
      cpu.single_step();

      REQUIRE( $f0->i32 == 0xDDDD'EEEE );
    }
    SECTION( "1($1) should load 0xBBDD'DDEE" )
    {
      $start = _lwc1_1;
      cpu.single_step();

      REQUIRE( $f0->i32 == 0xBBDD'DDEE );
    }
    SECTION( "2($1) should load 0xBBBB'DDDD" )
    {
      $start = _lwc1_2;
      cpu.single_step();

      REQUIRE( $f0->i32 == 0xBBBB'DDDD );
    }
    SECTION( "3($1) should load 0xAABB'BBDD" )
    {
      $start = _lwc1_3;
      cpu.single_step();

      REQUIRE( $f0->i32 == 0xAABB'BBDD );
    }
  }

  SECTION( "LWPC $1, 6000 is executed" )
  {
    auto const _lwpc = "LWPC"_cpu | 1_rs | 6000;

    auto $1 = R( 1 );

    *$1 = 0xCCCC'CCCC;

    ram[0xBFC0'0000 + ( 6000 << 2 )] = 0xAAAA'BBBB;

    $start = _lwpc;
    cpu.single_step();

    REQUIRE( *$1 == 0xAAAA'BBBB );
  }

  SECTION( "LWUPC $1, 6000 is executed" )
  {
    auto const _lwupc = "LWUPC"_cpu | 1_rs | 6000;

    auto $1 = R( 1 );

    *$1 = 0xCCCC'CCCC;

    ram[0xBFC0'0000 + ( 6000 << 2 )] = 0xAAAA'BBBB;

    $start = _lwupc;
    cpu.single_step();

    REQUIRE( *$1 == 0xAAAA'BBBB );
  }

  //// Unsigned Load

  SECTION( "LBU $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _lbu_0 = "LBU"_cpu | 1_rt | 0 | 2_rs;
    auto const _lbu_1 = "LBU"_cpu | 1_rt | 1 | 2_rs;
    auto const _lbu_2 = "LBU"_cpu | 1_rt | 2 | 2_rs;
    auto const _lbu_3 = "LBU"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$2 = 0x8000'0000;
    ram[0x8000'0000] = 0xAABB'DDEE;

    SECTION( "0($2) should load 0xEE" )
    {
      $start = _lbu_0;
      cpu.single_step();
      REQUIRE( *$1 == 0xEE );
    }
    SECTION( "1($2) should load 0xDD" )
    {
      $start = _lbu_1;
      cpu.single_step();
      REQUIRE( *$1 == 0xDD );
    }
    SECTION( "2($2) should load 0xBB" )
    {
      $start = _lbu_2;
      cpu.single_step();
      REQUIRE( *$1 == 0xBB );
    }
    SECTION( "3($2) should load 0xAA" )
    {
      $start = _lbu_3;
      cpu.single_step();
      REQUIRE( *$1 == 0xAA );
    }
  }

  SECTION( "LHU $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _lhu_0 = "LHU"_cpu | 1_rt | 0 | 2_rs;
    auto const _lhu_1 = "LHU"_cpu | 1_rt | 1 | 2_rs;
    auto const _lhu_2 = "LHU"_cpu | 1_rt | 2 | 2_rs;
    auto const _lhu_3 = "LHU"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$2 = 0x8000'0000;
    ram[0x8000'0000] = 0xDDDD'EEEE;
    ram[0x8000'0004] = 0xAAAA'BBBB;

    // Remember that LH sign extends before writing to rt

    SECTION( "0($2) should load 0xEEEE" )
    {
      $start = _lhu_0;
      cpu.single_step();
      REQUIRE( *$1 == 0xEEEE );
    }
    SECTION( "1($2) should load 0xDDEE" )
    {
      $start = _lhu_1;
      cpu.single_step();
      REQUIRE( *$1 == 0xDDEE );
    }
    SECTION( "2($2) should load 0xDDDD" )
    {
      $start = _lhu_2;
      cpu.single_step();
      REQUIRE( *$1 == 0xDDDD );
    }
    SECTION( "3($2) should load 0xBBDD" )
    {
      $start = _lhu_3;
      cpu.single_step();
      REQUIRE( *$1 == 0xBBDD );
    }
  }

  SECTION( "LDC1 $f0, n($1) is executed with n = {0,1,2,3}" )
  {
    auto const _ldc1_0 = "LDC1"_cpu | 0_rt | 0 | 1_rs;
    auto const _ldc1_1 = "LDC1"_cpu | 0_rt | 1 | 1_rs;
    auto const _ldc1_2 = "LDC1"_cpu | 0_rt | 2 | 1_rs;
    auto const _ldc1_3 = "LDC1"_cpu | 0_rt | 3 | 1_rs;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    PC() = pc;

    $f0->i64 = -1;

    *$1 = 0x8000'0000;

    ram[0x8000'0000] = 0xAAAA'AAAA;
    ram[0x8000'0004] = 0xBBBB'BBBB;
    ram[0x8000'0008] = 0xDDDD'DDDD;

    SECTION( "0($1) should load 0xBBBB'BBBB'AAAA'AAAA" )
    {
      $start = _ldc1_0;
      cpu.single_step();
      REQUIRE( $f0->i64 == 0xBBBB'BBBB'AAAA'AAAAull );
    }
    SECTION( "1($1) should load 0xDDBB'BBBB'BBAA'AAAA" )
    {
      $start = _ldc1_1;
      cpu.single_step();
      REQUIRE( $f0->i64 == 0xDDBB'BBBB'BBAA'AAAAull );
    }
    SECTION( "2($1) should load 0xDDDD'BBBB'BBBB'AAAA" )
    {
      $start = _ldc1_2;
      cpu.single_step();
      REQUIRE( $f0->i64 == 0xDDDD'BBBB'BBBB'AAAAull );
    }
    SECTION( "3($1) should load 0xDDDD'DDBB'BBBB'BBAA" )
    {
      $start = _ldc1_3;
      cpu.single_step();
      REQUIRE( $f0->i64 == 0xDDDD'DDBB'BBBB'BBAAull );
    }
  }

  SECTION( "SB $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _sb_0 = "SB"_cpu | 1_rt | 0 | 2_rs;
    auto const _sb_1 = "SB"_cpu | 1_rt | 1 | 2_rs;
    auto const _sb_2 = "SB"_cpu | 1_rt | 2 | 2_rs;
    auto const _sb_3 = "SB"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$1 = 0x33; // data
    *$2 = 0x8000'0000; // address

    ram[0x8000'0000] = 0xCCCC'CCCC;

    SECTION( "0($2) should store 0xCCCC'CC33" )
    {
      $start = _sb_0;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xCCCC'CC33 );
    }
    SECTION( "1($2) should store 0xCCCC'33CC" )
    {
      $start = _sb_1;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xCCCC'33CC );
    }
    SECTION( "2($2) should store 0xCC33'CCCC" )
    {
      $start = _sb_2;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xCC33'CCCC );
    }
    SECTION( "3($2) should store 0x33CC'CCCC" )
    {
      $start = _sb_3;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x33CC'CCCC );
    }
  }

  SECTION( "SH $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _sh_0 = "SH"_cpu | 1_rt | 0 | 2_rs;
    auto const _sh_1 = "SH"_cpu | 1_rt | 1 | 2_rs;
    auto const _sh_2 = "SH"_cpu | 1_rt | 2 | 2_rs;
    auto const _sh_3 = "SH"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$1 = 0x3333; // data
    *$2 = 0x8000'0000; // address

    ram[0x8000'0000] = 0xCCCC'CCCC;
    ram[0x8000'0004] = 0xCCCC'CCCC;

    SECTION( "0($2) should store 0xCCCC'3333" )
    {
      $start = _sh_0;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xCCCC'3333 );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCCC );
    }
    SECTION( "1($2) should store 0xCC33'33CC" )
    {
      $start = _sh_1;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xCC33'33CC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCCC );
    }
    SECTION( "2($2) should store 0x3333'CCCC" )
    {
      $start = _sh_2;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x3333'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCCC );
    }
    SECTION( "3($2) should store 0x33CC'CCCC and 0xCCCC'CC33" )
    {
      $start = _sh_3;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x33CC'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CC33 );
    }
  }

  SECTION( "SW $1, n($2) is executed with n = {0,1,2,3}" )
  {
    auto const _sw_0 = "SW"_cpu | 1_rt | 0 | 2_rs;
    auto const _sw_1 = "SW"_cpu | 1_rt | 1 | 2_rs;
    auto const _sw_2 = "SW"_cpu | 1_rt | 2 | 2_rs;
    auto const _sw_3 = "SW"_cpu | 1_rt | 3 | 2_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    PC() = pc;

    *$1 = 0x3333'3333; // data
    *$2 = 0x8000'0000; // address

    ram[0x8000'0000] = 0xCCCC'CCCC;
    ram[0x8000'0004] = 0xCCCC'CCCC;

    SECTION( "0($2) should store 0x3333'3333" )
    {
      $start = _sw_0;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x3333'3333 );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCCC );
    }
    SECTION( "1($2) should store 0x3333'33CC and 0xCCCC'CC33" )
    {
      $start = _sw_1;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x3333'33CC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CC33 );
    }
    SECTION( "2($2) should store 0x3333'CCCC and 0xCCCC'3333" )
    {
      $start = _sw_2;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x3333'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'3333 );
    }
    SECTION( "3($2) should store 0x33CC'CCCC and 0xCC33'3333" )
    {
      $start = _sw_3;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0x33CC'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCC33'3333 );
    }
  }

  SECTION( "SWC1 $f0, n($1) is executed with n = {0,1,2,3}" )
  {
    auto const _swc1_0 = "SWC1"_cpu | 0_rt | 1_rs | 0;
    auto const _swc1_1 = "SWC1"_cpu | 0_rt | 1_rs | 1;
    auto const _swc1_2 = "SWC1"_cpu | 0_rt | 1_rs | 2;
    auto const _swc1_3 = "SWC1"_cpu | 0_rt | 1_rs | 3;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    *$1 = 0x8000'0000;
    $f0->i32 = 0xAAAA'BBBB;

    ram[0x8000'0000] = 0xCCCC'CCCC;
    ram[0x8000'0004] = 0xCCCC'CCCC;

    PC() = pc;

    SECTION( "0($1) should store 0xAAAA'BBBB" )
    {
      $start = _swc1_0;
      cpu.single_step();

      REQUIRE( ram[0x8000'0000] == 0xAAAA'BBBB );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCCC );
    }
    SECTION( "1($1) should load 0xAABB'BBCC and 0xCCCC'CCAA" )
    {
      $start = _swc1_1;
      cpu.single_step();

      REQUIRE( ram[0x8000'0000] == 0xAABB'BBCC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'CCAA );
    }
    SECTION( "2($1) should load 0xBBBB'CCCC and 0xCCCC'AAAA" )
    {
      $start = _swc1_2;
      cpu.single_step();

      REQUIRE( ram[0x8000'0000] == 0xBBBB'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCCCC'AAAA );
    }
    SECTION( "3($1) should load 0xBBCC'CCCC and 0xCCAA'AABB" )
    {
      $start = _swc1_3;
      cpu.single_step();

      REQUIRE( ram[0x8000'0000] == 0xBBCC'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xCCAA'AABB );
    }
  }

  SECTION( "SDC1 $f0, n($1) is executed with n = {0,1,2,3}" )
  {
    auto const _sdc1_0 = "SDC1"_cpu | 0_rt | 0 | 1_rs;
    auto const _sdc1_1 = "SDC1"_cpu | 0_rt | 1 | 1_rs;
    auto const _sdc1_2 = "SDC1"_cpu | 0_rt | 2 | 1_rs;
    auto const _sdc1_3 = "SDC1"_cpu | 0_rt | 3 | 1_rs;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    PC() = pc;

    $f0->i64 = 0xAAAA'BBBB'DDDD'EEEEull;

    *$1 = 0x8000'0000;

    ram[0x8000'0000] = 0xCCCC'CCCC;
    ram[0x8000'0004] = 0xCCCC'CCCC;
    ram[0x8000'0008] = 0xCCCC'CCCC;

    SECTION( "0($1) should store 0xDDDD'EEEE and 0xAAAA'BBBB" )
    {
      $start = _sdc1_0;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xDDDD'EEEE );
      REQUIRE( ram[0x8000'0004] == 0xAAAA'BBBB );
      REQUIRE( ram[0x8000'0008] == 0xCCCC'CCCC );
    }
    SECTION( "1($1) should store 0xDDEE'EECC and 0xAABB'BBDD and 0xCCCC'CCAA" )
    {
      $start = _sdc1_1;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xDDEE'EECC );
      REQUIRE( ram[0x8000'0004] == 0xAABB'BBDD );
      REQUIRE( ram[0x8000'0008] == 0xCCCC'CCAA );
    }
    SECTION( "2($1) should store 0xEEEE'CCCC and 0xBBBB'DDDD and 0xCCCC'AAAA" )
    {
      $start = _sdc1_2;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xEEEE'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xBBBB'DDDD );
      REQUIRE( ram[0x8000'0008] == 0xCCCC'AAAA );
    }
    SECTION( "3($1) should store 0xEECC'CCCC and 0xBBDD'DDEE and 0xCCAA'AABB" )
    {
      $start = _sdc1_3;
      cpu.single_step();
      REQUIRE( ram[0x8000'0000] == 0xEECC'CCCC );
      REQUIRE( ram[0x8000'0004] == 0xBBDD'DDEE );
      REQUIRE( ram[0x8000'0008] == 0xCCAA'AABB );
    }
  }

  SECTION( "LSA $1, $2, $3, n is executed with n = {0,1,2,3}" )
  {
    auto const _lsa_0 = "LSA"_cpu | 1_rd | 2_rs | 3_rt | 0_shamt;
    auto const _lsa_1 = "LSA"_cpu | 1_rd | 2_rs | 3_rt | 1_shamt;
    auto const _lsa_2 = "LSA"_cpu | 1_rd | 2_rs | 3_rt | 2_shamt;
    auto const _lsa_3 = "LSA"_cpu | 1_rd | 2_rs | 3_rt | 3_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    PC() = pc;

    *$2 = 0x8000;
    *$3 = 512;

    SECTION( "The result shall be correct in the 1st case" )
    {
      $start = _lsa_0;
      cpu.single_step();

      auto const res = ( 0x8000 << 1 ) + 512;
      REQUIRE( *$1 == res );
    }
    SECTION( "The result shall be correct in the 2nd case" )
    {
      $start = _lsa_1;
      cpu.single_step();

      auto const res = ( 0x8000 << 2 ) + 512;
      REQUIRE( *$1 == res );
    }
    SECTION( "The result shall be correct in the 3rd case" )
    {
      $start = _lsa_2;
      cpu.single_step();

      auto const res = ( 0x8000 << 3 ) + 512;
      REQUIRE( *$1 == res );
    }
    SECTION( "The result shall be correct in the 4th case" )
    {
      $start = _lsa_3;
      cpu.single_step();

      auto const res = ( 0x8000 << 4 ) + 512;
      REQUIRE( *$1 == res );
    }
  }

  SECTION( "LUI $29, 0xABCD is executed" )
  {
    auto const _lui = "LUI"_cpu | 29_rt | 0xABCD;

    auto $29 = R( 29 );

    $start = _lui;
    cpu.single_step();

    REQUIRE( *$29 == 0xABCD'0000 );
  }

  SECTION( "MUL $1, $2, $3 is executed" )
  {
    auto const _mul = "MUL"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 53'897;
    *$3 = -9043;

    ui32 const res = 53'897 * -9043;

    $start = _mul;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "MUH $4, $5, $6 is executed" )
  {
    auto const _mul = "MUH"_cpu | 4_rd | 5_rs | 6_rt;

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$5 = 0xFFFF'FFFF;
    *$6 = 0xABCD'0000;

    ui32 const res = ( std::uint64_t( 0xFFFF'FFFF ) * std::uint64_t( 0xABCD'0000 ) ) >> 32;

    $start = _mul;
    cpu.single_step();

    REQUIRE( *$4 == res );
  }

  SECTION( "MULU $1, $2, $3 is executed" )
  {
    auto const _mul = "MULU"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 53'897;
    *$3 = -9043;

    ui32 const res = 53'897 * -9043;

    $start = _mul;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "MUHU $4, $5, $6 is executed" )
  {
    auto const _mul = "MUHU"_cpu | 4_rd | 5_rs | 6_rt;

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$5 = 0xFFFF'FFFF;
    *$6 = 0xABCD'0000;

    ui32 const res = ( std::uint64_t( 0xFFFF'FFFF ) * std::uint64_t( 0xABCD'0000 ) ) >> 32;

    $start = _mul;
    cpu.single_step();

    REQUIRE( *$4 == res );
  }

  SECTION( "NAL is executed" )
  {
    auto $31 = R( 31 );

    auto const res = PC() + 8;

    $start = "NAL"_cpu;
    cpu.single_step();

    REQUIRE( *$31 == res );
  }

  SECTION( "NOP is executed" )
  {
    auto begin = inspector.CPU_gpr_begin();
    auto const end = inspector.CPU_gpr_end();

    while ( begin != end )
      *begin++ = 0;

    auto const pc = PC() + 4;

    $start = "NOP"_cpu;
    cpu.single_step();

    begin = inspector.CPU_gpr_begin();

    REQUIRE( pc == PC() );

    while ( begin != end )
      REQUIRE( *begin++ == 0 );
  }

  SECTION( "NOR $1, $2, $3 is executed" )
  {
    auto const _nor = "NOR"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 0x0000'ABCD;
    *$3 = 0xDCBA'0000;

    auto const res = ~( 0x0000'ABCD | 0xDCBA'0000 );

    $start = _nor;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "OR $1, $2, $3 is executed" )
  {
    auto const _nor = "OR"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 0x0000'ABCD;
    *$3 = 0xDCBA'0000;

    auto const res = 0x0000'ABCD | 0xDCBA'0000;

    $start = _nor;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "ORI $1, $2, 0x1234 is executed" )
  {
    auto const _nor = "ORI"_cpu | 1_rt | 2_rs | 0x1234;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0x0000'ABCD;

    auto const res = 0x0000'ABCD | 0x1234;

    $start = _nor;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "ROTR $1, $2, 5 is executed" )
  {
    auto const _rotr = "ROTR"_cpu | 1_rd | 2_rt | 8_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0xABCD'1234;

    auto const res = 0xABCD'1234 >> 8 | 0xABCD'1234 << ( 32 - 8 );

    $start = _rotr;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "ROTRV $1, $2, $3 is executed" )
  {
    auto const _rotrv = "ROTRV"_cpu | 1_rd | 2_rt | 3_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 0xABCD'1234;
    *$3 = 8;

    auto const res = 0xABCD'1234 >> 8 | 0xABCD'1234 << ( 32 - 8 );

    $start = _rotrv;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SELEQZ $1, $2, $3 and SELEQZ $4, $5, $6 are executed" )
  {
    auto const _seleqz_select = "SELEQZ"_cpu | 1_rd | 2_rs | 3_rt;
    auto const _seleqz_no_select = "SELEQZ"_cpu | 4_rd | 5_rs | 6_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$1 = 0;
    *$3 = 0;
    *$2 = 523;

    *$4 = 6000;
    *$6 = 1;
    *$5 = 9;

    SECTION( "It shall change $1 to $2 in the 1st case" )
    {
      $start = _seleqz_select;
      cpu.single_step();

      REQUIRE( *$1 == *$2 );
    }
    SECTION( "It shall change $4 to 0 in the 2nd case" )
    {
      $start = _seleqz_no_select;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  SECTION( "SELNEZ $1, $2, $3 and SELNEZ $4, $5, $6 are executed" )
  {
    auto const _selnez_select = "SELNEZ"_cpu | 1_rd | 2_rs | 3_rt;
    auto const _selnez_no_select = "SELNEZ"_cpu | 4_rd | 5_rs | 6_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$1 = 0;
    *$3 = 1;
    *$2 = 523;

    *$4 = 6000;
    *$6 = 0;
    *$5 = 9;

    SECTION( "It shall change $1 to $2 in the 1st case" )
    {
      $start = _selnez_select;
      cpu.single_step();

      REQUIRE( *$1 == *$2 );
    }
    SECTION( "It shall change $4 to 0 in the 2nd case" )
    {
      $start = _selnez_no_select;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  SECTION( "SLL $1, $2, 18 is executed" )
  {
    auto const _sll = "SLL"_cpu | 1_rd | 2_rt | 18_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0xABCD'1234;

    auto const res = 0xABCD'1234 << 18;

    $start = _sll;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SLLV $1, $2, $3 is executed" )
  {
    auto const _sllv = "SLLV"_cpu | 1_rd | 2_rt | 3_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 0xABCD'1234;
    *$3 = 18;

    auto const res = 0xABCD'1234 << 18;

    $start = _sllv;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SLT $1, $2, $3 and SLT $4, $5, $6 are executed" )
  {
    auto const _slt_set = "SLT"_cpu | 1_rd | 2_rs | 3_rt;
    auto const _slt_clear = "SLT"_cpu | 4_rd | 5_rs | 6_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$2 = -124;
    *$3 = 0;

    *$5 = 523;
    *$6 = 235;

    SECTION( "It should be set in the 1st case" )
    {
      $start = _slt_set;
      cpu.single_step();

      REQUIRE( *$1 == 1 );
    }
    SECTION( "It should be clear in the 2nd case" )
    {
      $start = _slt_clear;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  SECTION( "SLTI $2, $3, 29 and SLTI $5, $6, 68 are executed" )
  {
    auto const _slti_set = "SLTI"_cpu | 2_rt | 3_rs | 29;
    auto const _slti_clear = "SLTI"_cpu | 5_rt | 6_rs | 68;

    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$3 = 0;

    *$6 = 235;

    SECTION( "It should be set in the 1st case" )
    {
      $start = _slti_set;
      cpu.single_step();

      REQUIRE( *$2 == 1 );
    }
    SECTION( "It should be clear in the 2nd case" )
    {
      $start = _slti_clear;
      cpu.single_step();

      REQUIRE( *$5 == 0 );
    }
  }

  SECTION( "SLTU $1, $2, $3 and SLTU $4, $5, $6 are executed" )
  {
    auto const _sltu_set = "SLTU"_cpu | 1_rd | 2_rs | 3_rt;
    auto const _sltu_clear = "SLTU"_cpu | 4_rd | 5_rs | 6_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $4 = R( 4 );
    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$2 = 0;
    *$3 = -124;

    *$5 = -523;
    *$6 = 235;

    SECTION( "It should be set in the 1st case" )
    {
      $start = _sltu_set;
      cpu.single_step();

      REQUIRE( *$1 == 1 );
    }
    SECTION( "It should be clear in the 2nd case" )
    {
      $start = _sltu_clear;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  SECTION( "SLTIU $2, $3, 29 and SLTIU $5, $6, 68 are executed" )
  {
    auto const _sltiu_set = "SLTIU"_cpu | 2_rt | 3_rs | 29;
    auto const _sltiu_clear = "SLTIU"_cpu | 5_rt | 6_rs | 68;

    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$3 = 14;

    *$6 = 235;

    SECTION( "It should be set in the 1st case" )
    {
      $start = _sltiu_set;
      cpu.single_step();

      REQUIRE( *$2 == 1 );
    }
    SECTION( "It should be clear in the 2nd case" )
    {
      $start = _sltiu_clear;
      cpu.single_step();

      REQUIRE( *$5 == 0 );
    }
  }

  SECTION( "SRA $1, $2, 17 is executed" )
  {
    auto const _sra = "SRA"_cpu | 1_rd | 2_rt | 17_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = -47;

    ui32 const res = -1;

    $start = _sra;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SRAV $1, $2, $3 is executed" )
  {
    auto const _srav = "SRAV"_cpu | 1_rd | 2_rt | 3_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -988;
    *$3 = 4;

    ui32 const res = -62;

    $start = _srav;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SRL $1, $2, 18 is executed" )
  {
    auto const _srl = "SRL"_cpu | 1_rd | 2_rt | 18_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = -988;

    ui32 const res = ( ui32 )-988 >> 18;

    $start = _srl;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SRLV $1, $2, $3 is executed" )
  {
    auto const _srlv = "SRLV"_cpu | 1_rd | 2_rt | 3_rs;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -988;
    *$3 = 4;

    ui32 const res = ( ui32 )-988 >> 4;

    $start = _srlv;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SUB $1, $2, $3 is executed" )
  {
    auto const _sub = "SUB"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -253;
    *$3 = 6;

    ui32 const res = ( ui32 )-253 - ( ui32 )6;

    $start = _sub;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "SUBU $1, $2, $3 is executed" )
  {
    auto const _subu = "SUBU"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -598;
    *$3 = 978;

    ui32 const res = ( ui32 )-598 - ( ui32 )978;

    $start = _subu;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "TEQ $1, $2 and TEQ $3, $4 are executed" )
  {
    auto const _teq_trap = "TEQ"_cpu | 1_rs | 2_rt;
    auto const _teq_no_trap = "TEQ"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = *$2;

    *$3 = *$4 + 20;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _teq_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _teq_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "TGE $1, $2 and TGE $3, $4 are executed" )
  {
    auto const _tge_trap = "TGE"_cpu | 1_rs | 2_rt;
    auto const _tge_no_trap = "TGE"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = *$2 + 1;

    *$3 = *$4 - 20;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _tge_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _tge_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "TGEU $1, $2 and TGEU $3, $4 are executed" )
  {
    auto const _tgeu_trap = "TGEU"_cpu | 1_rs | 2_rt;
    auto const _tgeu_no_trap = "TGEU"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = -112;
    *$2 = 113;

    *$3 = 0;
    *$4 = -1;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _tgeu_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _tgeu_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "TLT $1, $2 and TLT $3, $4 are executed" )
  {
    auto const _tlt_trap = "TLT"_cpu | 1_rs | 2_rt;
    auto const _tlt_no_trap = "TLT"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = -1;
    *$2 = 0;

    *$3 = 0;
    *$4 = -1;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _tlt_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _tlt_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "TLTU $1, $2 and TLTU $3, $4 are executed" )
  {
    auto const _tltu_trap = "TLTU"_cpu | 1_rs | 2_rt;
    auto const _tltu_no_trap = "TLTU"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = 0;
    *$2 = -1;

    *$3 = 412;
    *$4 = 23;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _tltu_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _tltu_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "TNE $1, $2 and TNE $3, $4 are executed" )
  {
    auto const _tne_trap = "TNE"_cpu | 1_rs | 2_rt;
    auto const _tne_no_trap = "TNE"_cpu | 3_rs | 4_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    auto $3 = R( 3 );
    auto $4 = R( 4 );

    *$1 = 0;
    *$2 = 1;

    *$3 = 0;
    *$4 = 0;

    SECTION( "It shall trap in the 1st case" )
    {
      $start = _tne_trap;
      cpu.single_step();

      REQUIRE( HasTrapped() );
    }
    SECTION( "It shall not trap in the 2nd case" )
    {
      $start = _tne_no_trap;
      cpu.single_step();

      REQUIRE( !HasTrapped() );
    }
  }

  SECTION( "XOR $1, $2, $3 is executed" )
  {
    auto const _xor = "XOR"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = -253;
    *$3 = 6;

    ui32 const res = ( ui32 )-253 ^ ( ui32 )6;

    $start = _xor;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  SECTION( "XORI $1, $2, 0xABC is executed" )
  {
    auto const _xori = "XORI"_cpu | 1_rt | 2_rs | 0xABC;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = -253;

    ui32 const res = ( ui32 )-253 ^ ( ui32 )0xABC;

    $start = _xori;
    cpu.single_step();

    REQUIRE( *$1 == res );
  }

  /* * * * * * * * * *
   *                 *
   * COPROCESSOR 0|1 *
   *                 *
   * * * * * * * * * */

  SECTION( "[MFC0] is executed with multiple selections (all of them write to $1)" )
  {
    auto const _user_local = "MFC0"_cpu | 1_rt | 4_rd | 2;
    auto const _hwrena = "MFC0"_cpu | 1_rt | 7_rd | 0;
    auto const _badvaddr = "MFC0"_cpu | 1_rt | 8_rd | 0;
    auto const _badinstr = "MFC0"_cpu | 1_rt | 8_rd | 1;
    auto const _status = "MFC0"_cpu | 1_rt | 12_rd | 0;
    auto const _intctl = "MFC0"_cpu | 1_rt | 12_rd | 1;
    auto const _srsctl = "MFC0"_cpu | 1_rt | 12_rd | 2;
    auto const _cause = "MFC0"_cpu | 1_rt | 13_rd | 0;
    auto const _epc = "MFC0"_cpu | 1_rt | 14_rd | 0;
    auto const _prid = "MFC0"_cpu | 1_rt | 15_rd | 0;
    auto const _ebase = "MFC0"_cpu | 1_rt | 15_rd | 1;
    ui32 const _config[6] = {
      "MFC0"_cpu | 1_rt | 16_rd | 0,
      "MFC0"_cpu | 1_rt | 16_rd | 1,
      "MFC0"_cpu | 1_rt | 16_rd | 2,
      "MFC0"_cpu | 1_rt | 16_rd | 3,
      "MFC0"_cpu | 1_rt | 16_rd | 4,
    };
    auto const _errorepc = "MFC0"_cpu | 1_rt | 30_rd | 0;
    ui32 const _kscratch[6] = {
      "MFC0"_cpu | 1_rt | 31_rd | 2,
      "MFC0"_cpu | 1_rt | 31_rd | 3,
      "MFC0"_cpu | 1_rt | 31_rd | 4,
      "MFC0"_cpu | 1_rt | 31_rd | 5,
    };

    auto $1 = R( 1 );

    SECTION( "I shall able to read user_local" )
    {
      $start = _user_local;
      cpu.single_step();

      REQUIRE( *$1 == cp0.user_local );
    }
    SECTION( "I shall able to read HWREna" )
    {
      $start = _hwrena;
      cpu.single_step();

      REQUIRE( *$1 == cp0.hwr_ena );
    }
    SECTION( "I shall able to read BadVAddr" )
    {
      $start = _badvaddr;
      cpu.single_step();

      REQUIRE( *$1 == cp0.bad_vaddr );
    }
    SECTION( "I shall able to read BadInstr" )
    {
      $start = _badinstr;
      cpu.single_step();

      REQUIRE( *$1 == cp0.bad_instr );
    }
    SECTION( "I shall able to read Status" )
    {
      $start = _status;
      cpu.single_step();

      REQUIRE( *$1 == cp0.status );
    }
    SECTION( "I shall able to read IntCtl" )
    {
      $start = _intctl;
      cpu.single_step();

      REQUIRE( *$1 == cp0.int_ctl );
    }
    SECTION( "I shall able to read SRSCtl" )
    {
      $start = _srsctl;
      cpu.single_step();

      REQUIRE( *$1 == cp0.srs_ctl );
    }
    SECTION( "I shall able to read Cause" )
    {
      $start = _cause;
      cpu.single_step();

      REQUIRE( *$1 == cp0.cause );
    }
    SECTION( "I shall able to read EPC" )
    {
      $start = _epc;
      cpu.single_step();

      REQUIRE( *$1 == cp0.epc );
    }
    SECTION( "I shall able to read PRId" )
    {
      $start = _prid;
      cpu.single_step();

      REQUIRE( *$1 == cp0.pr_id );
    }
    SECTION( "I shall able to read EBase" )
    {
      $start = _ebase;
      cpu.single_step();

      REQUIRE( *$1 == cp0.e_base );
    }
    SECTION( "I shall able to read Config registers" )
    {
      for ( int i = 0; i < 5; ++i )
      {
        PC() = pc;
        $start = _config[i];
        cpu.single_step();

        REQUIRE( *$1 == cp0.config[i] );
      }
    }
    SECTION( "I shall able to read ErrorEPC" )
    {
      $start = _errorepc;
      cpu.single_step();

      REQUIRE( *$1 == cp0.error_epc );
    }
    SECTION( "I shall able to read KScratch registers" )
    {
      for ( int i = 0; i < 5; ++i )
      {
        PC() = pc;
        $start = _kscratch[i];
        cpu.single_step();

        REQUIRE( *$1 == cp0.k_scratch[i + 2] );
      }
    }
  }

  SECTION( "MFHC0 is executed" )
  {
    auto $1 = R( 1 );

    *$1 = 21;

    $start = "MFHC0"_cpu | 1_rt;

    cpu.single_step();

    REQUIRE( *$1 == 0 );
  }

  SECTION( "[MTC0] is executed with multiple selections (all of them read from $1)" )
  {
    auto const _user_local = "MTC0"_cpu | 1_rt | 4_rd | 2;
    auto const _hwrena = "MTC0"_cpu | 1_rt | 7_rd | 0;
    auto const _badvaddr = "MTC0"_cpu | 1_rt | 8_rd | 0;
    auto const _badinstr = "MTC0"_cpu | 1_rt | 8_rd | 1;
    auto const _status = "MTC0"_cpu | 1_rt | 12_rd | 0;
    auto const _intctl = "MTC0"_cpu | 1_rt | 12_rd | 1;
    auto const _srsctl = "MTC0"_cpu | 1_rt | 12_rd | 2;
    auto const _cause = "MTC0"_cpu | 1_rt | 13_rd | 0;
    auto const _epc = "MTC0"_cpu | 1_rt | 14_rd | 0;
    auto const _prid = "MTC0"_cpu | 1_rt | 15_rd | 0;
    auto const _ebase = "MTC0"_cpu | 1_rt | 15_rd | 1;
    ui32 const _config[6] = {
      "MTC0"_cpu | 1_rt | 16_rd | 0,
      "MTC0"_cpu | 1_rt | 16_rd | 1,
      "MTC0"_cpu | 1_rt | 16_rd | 2,
      "MTC0"_cpu | 1_rt | 16_rd | 3,
      "MTC0"_cpu | 1_rt | 16_rd | 4,
      "MTC0"_cpu | 1_rt | 16_rd | 5,
    };
    auto const _errorepc = "MTC0"_cpu | 1_rt | 30_rd | 0;
    ui32 const _kscratch[6] = {
      "MTC0"_cpu | 1_rt | 31_rd | 2,
      "MTC0"_cpu | 1_rt | 31_rd | 3,
      "MTC0"_cpu | 1_rt | 31_rd | 4,
      "MTC0"_cpu | 1_rt | 31_rd | 5,
      "MTC0"_cpu | 1_rt | 31_rd | 6,
      "MTC0"_cpu | 1_rt | 31_rd | 7,
    };

    auto $1 = R( 1 );

    *$1 = 0xFFFF'FFFF;

    SECTION( "I shall able to write user_local" )
    {
      $start = _user_local;
      cpu.single_step();

      REQUIRE( *$1 == cp0.user_local );
    }
    SECTION( "I shall not able to write HWREna" )
    {
      auto const _prev_hwr_ena = cp0.hwr_ena;

      $start = _hwrena;
      cpu.single_step();

      REQUIRE( _prev_hwr_ena == cp0.hwr_ena );
    }
    SECTION( "I shall not able to write BadVAddr" )
    {
      auto const _prev_bad_vaddr = cp0.bad_vaddr;

      $start = _badvaddr;
      cpu.single_step();

      REQUIRE( _prev_bad_vaddr == cp0.bad_vaddr );
    }
    SECTION( "I shall not able to write BadInstr" )
    {
      auto const _prev_bad_instr = cp0.bad_instr;

      $start = _badinstr;
      cpu.single_step();

      REQUIRE( _prev_bad_instr == cp0.bad_instr );
    }
    SECTION( "I shall able to write Status" )
    {
      ui32 const expected_status = 0xFFFF'FFFF & 0x1000'FF13 | cp0.status;

      $start = _status;
      cpu.single_step();

      REQUIRE( expected_status == cp0.status );
    }
    SECTION( "I shall not able to write IntCtl" )
    {
      auto const _prev_int_ctl = cp0.int_ctl;

      $start = _intctl;
      cpu.single_step();

      REQUIRE( _prev_int_ctl == cp0.int_ctl );
    }
    SECTION( "I shall not able to write SRSCtl" )
    {
      auto const _prev_srs_ctl = cp0.srs_ctl;

      $start = _srsctl;
      cpu.single_step();

      REQUIRE( _prev_srs_ctl == cp0.srs_ctl );
    }
    SECTION( "I shall not able to write Cause" )
    {
      auto const _prev_cause = cp0.cause;

      $start = _cause;
      cpu.single_step();

      REQUIRE( _prev_cause == cp0.cause );
    }
    SECTION( "I shall able to write EPC" )
    {
      $start = _epc;
      cpu.single_step();

      REQUIRE( *$1 == cp0.epc );
    }
    SECTION( "I shall not able to write PRId" )
    {
      auto const _prev_pr_id = cp0.pr_id;

      $start = _prid;
      cpu.single_step();

      REQUIRE( _prev_pr_id == cp0.pr_id );
    }
    SECTION( "I shall able to write EBase (WG disabled)" )
    {
      *$1 &= ~( 1 << 11 ); // otherwise we could enable the WG

      $start = _ebase;
      cpu.single_step();

      *$1 = 0xFFFF'FFFF;

      REQUIRE( 0x3FFF'F000 == cp0.e_base );
    }
    SECTION( "I shall able to write EBase (WG enabled)" )
    {
      cp0.e_base |= 1 << 11; // enable WG

      $start = _ebase;
      cpu.single_step();

      REQUIRE( 0xFFFF'F800 == cp0.e_base ); // 31..30 and WG are set
    }
    SECTION( "I shall not able to write Config registers" )
    {
      for ( int i = 0; i < 5; ++i )
      {
        auto const _prev_config = cp0.config[i];

        PC() = pc;
        $start = _config[i];
        cpu.single_step();

        REQUIRE( _prev_config == cp0.config[i] );
      }
    }
    SECTION( "I shall able to write ErrorEPC" )
    {
      $start = _errorepc;
      cpu.single_step();

      REQUIRE( *$1 == cp0.error_epc );
    }
    SECTION( "I shall able to write KScratch registers" )
    {
      for ( int i = 0; i < 5; ++i )
      {
        PC() = pc;
        $start = _kscratch[i];
        cpu.single_step();

        REQUIRE( *$1 == cp0.k_scratch[i + 2] );
      }
    }
  }

  // This test is used just to increase the coverage, MTHC0 behave like a NOP
  SECTION( "MTHC0 is executed" )
  {
    $start = "MTHC0"_cpu;
    cpu.single_step();
  }

  SECTION( "MFC1 $1, $f0 is executed" )
  {
    auto const _mfc1 = "MFC1"_cpu | 1_rt | 0_rd;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    $f0->i64 = 0xAAAA'BBBB'DDDD'EEEEull;

    $start = _mfc1;
    cpu.single_step();

    REQUIRE( *$1 == 0xDDDD'EEEE );
  }

  SECTION( "MFHC1 $1, $f0 is executed" )
  {
    auto const _mfhc1 = "MFHC1"_cpu | 1_rt | 0_rd;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    $f0->i64 = 0xAAAA'BBBB'DDDD'EEEEull;

    $start = _mfhc1;
    cpu.single_step();

    REQUIRE( *$1 == 0xAAAA'BBBB );
  }

  SECTION( "MTC1 $1, $f0 is executed" )
  {
    auto const _mtc1 = "MTC1"_cpu | 1_rt | 0_rd;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    *$1 = 0xAAAA'BBBB;
    $f0->i64 = 0xCCCC'CCCC'CCCC'CCCCull;

    $start = _mtc1;
    cpu.single_step();

    REQUIRE( $f0->i64 == 0xCCCC'CCCC'AAAA'BBBBull );
  }

  SECTION( "MTHC1 $1, $f0 is executed" )
  {
    auto const _mthc1 = "MTHC1"_cpu | 1_rt | 0_rd;

    auto $f0 = FP( 0 );
    auto $1 = R( 1 );

    *$1 = 0xDDDD'EEEE;
    $f0->i64 = 0xCCCC'CCCC'CCCC'CCCCull;

    $start = _mthc1;
    cpu.single_step();

    REQUIRE( $f0->i64 == 0xDDDD'EEEE'CCCC'CCCCull );
  }

  SECTION( "Swapping 2 FPRs registers using GPRS" )
  {
    // $f0 into $1, $2
    auto const _mfc1_0 = "MFC1"_cpu | 1_rt | 0_rd;
    auto const _mfhc1_0 = "MFHC1"_cpu | 2_rt | 0_rd;

    // $f1 into $3, $4
    auto const _mfc1_1 = "MFC1"_cpu | 3_rt | 1_rd;
    auto const _mfhc1_1 = "MFHC1"_cpu | 4_rt | 1_rd;

    // $1, $2 ($f0) into $f1
    auto const _mtc1_1 = "MTC1"_cpu | 1_rt | 1_rd;
    auto const _mthc1_1 = "MTHC1"_cpu | 2_rt | 1_rd;

    // $3, $4 ($f1) into $f0
    auto const _mtc1_0 = "MTC1"_cpu | 3_rt | 0_rd;
    auto const _mthc1_0 = "MTHC1"_cpu | 4_rt | 0_rd;

    // Copying $f0/$f1 into GPRs
    ram[0xBFC0'0000] = _mfc1_0;  // mfc1 $1, $f0
    ram[0xBFC0'0004] = _mfhc1_0; // mfhc1 $2, $f0

    ram[0xBFC0'0008] = _mfc1_1;  // mfc1 $3, $f1
    ram[0xBFC0'000C] = _mfhc1_1; // mfhc1 $4, $f1

    // Swapping FPRs
    ram[0xBFC0'0010] = _mtc1_1;  // mtc1 $3, $f1
    ram[0xBFC0'0014] = _mthc1_1; // mthc1 $4, $f1

    ram[0xBFC0'0018] = _mtc1_0;  // mfhc1 $1, $f0
    ram[0xBFC0'001C] = _mthc1_0; // mfhc1 $2, $f0

    ram[0xBFC0'0020] = "BREAK"_cpu;

    auto $f0 = FP( 0 );
    auto $f1 = FP( 1 );

    $f0->i64 = 0XBBBB'BBBB'AAAA'AAAAull;
    $f1->i64 = 0xEEEE'EEEE'DDDD'DDDDull;

    cpu.start();

    REQUIRE( $f0->i64 == 0xEEEE'EEEE'DDDD'DDDDull );
    REQUIRE( $f1->i64 == 0XBBBB'BBBB'AAAA'AAAAull );
  }

  /* * * * * * * *
   *             *
   * EXCEPTIONS  *
   *             *
   * * * * * * * */

  //// Integer Overflow

  SECTION( "ADD $1, $2, $3 is executed" )
  {
    auto const _add = "ADD"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$1 = 0;
    *$2 = 0xF000'0000;
    *$3 = 0xF000'0000;

    $start = _add;
    cpu.single_step();

    REQUIRE( *$1 == 0 );
    REQUIRE( HasOverflowed() );
  }

  SECTION( "SUB $1, $2, $3 is executed" )
  {
    auto const _sub = "SUB"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$1 = 0;
    *$2 = 0xFFFF'0000;
    *$3 = 0xFFFF'FFFF;

    $start = _sub;
    cpu.single_step();

    REQUIRE( *$1 == 0 );
    REQUIRE( HasOverflowed() );
  }

  //// Instruction Fetch

  SECTION( "An instruction is fetched with a misaligned PC" )
  {
    PC() |= 1;
    cpu.single_step();

    REQUIRE( ExCause() == 4 );
  }

  /* * * * * *
   *         *
   * SYSCALL *
   *         *
   * * * * * */

  SECTION( "[SYSCALL] print_int is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = PRINT_INT;
    *$a0 = 19940915;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( terminal->out_int == 19940915 );
  }

  SECTION( "[SYSCALL] print_float is executed" )
  {
    auto $v0 = R( _v0 );
    auto $f12 = FP( 12 );

    *$v0 = PRINT_FLOAT;
    $f12->f = 1200.53f;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( terminal->out_float == 1200.53f );
  }

  SECTION( "[SYSCALL] print_double is executed" )
  {
    auto $v0 = R( _v0 );
    auto $f12 = FP( 12 );

    *$v0 = PRINT_DOUBLE;
    $f12->d = 987654.23;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( terminal->out_double == 987654.23 );
  }

  SECTION( "[SYSCALL] print_string is executed (same block in memory)" )
  {
    auto is_in_memory = [] ( std::vector<std::uint32_t> const& addresses, std::uint32_t address ) -> bool
    {
      for ( auto const & val : addresses )
        if ( address == val )
          return true;

      return false;
    };

    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    *$a0 = 0x0000'0000;

    terminal->in_string = "[SYSCALL] print_string";
    terminal->in_string += '\0'; // we must add it manually

    // We place the string into RAM through a syscall
    *$v0 = READ_STRING;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    // We then perform the print
    *$v0 = PRINT_STRING;
    *$a0 = 0x0000'0000;

    $start = "SYSCALL"_cpu;

    // Check that the block is in memory
    REQUIRE( is_in_memory( inspector.RAM_allocated_addresses(), 0 ) );

    PC() = pc;
    cpu.single_step();

    REQUIRE( terminal->out_string == "[SYSCALL] print_string" );
  }

  SECTION( "[SYSCALL] print_string is executed (different blocks in memory)" )
  {
    auto is_in_memory = [] ( std::vector<std::uint32_t> const& addresses, std::uint32_t address ) -> bool
    {
      for ( auto const & val : addresses )
        if ( address == val )
          return true;

      return false;
    };

    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    *$a0 = 0x0001'0000 - 0xF;

    terminal->in_string = "[SYSCALL] print_string";
    terminal->in_string += '\0'; // we must add it manually

    // We place the string into RAM through a syscall
    *$v0 = READ_STRING;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    // We then perform the print
    *$v0 = PRINT_STRING;

    $start = "SYSCALL"_cpu;

    // Check that the blocks are in memory
    REQUIRE( is_in_memory( inspector.RAM_allocated_addresses(), 0x0000'0000 ) );
    REQUIRE( is_in_memory( inspector.RAM_allocated_addresses(), 0x0001'0000 ) );

    PC() = pc;
    cpu.single_step();

    REQUIRE( terminal->out_string == "[SYSCALL] print_string" );
  }

  SECTION( "[SYSCALL] print_string is executed (same block swapped)" )
  {
    auto is_swapped = [] ( std::vector<std::uint32_t> const& addresses, std::uint32_t address ) -> bool
    {
      for ( auto const & val : addresses )
        if ( address == val )
          return true;

      return false;
    };

    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    *$a0 = 0x1000'0000;

    terminal->in_string = "[SYSCALL] print_string";
    terminal->in_string += '\0'; // we must add it manually

    // We place the string into RAM through a syscall
    *$v0 = READ_STRING;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    // We need to force a swap of the block at address 0x1000'0000
    for ( int i = 0; i < 10; ++i )
    {
      std::uint32_t volatile _dummy;
      _dummy = ram[0x2000'0000];
      _dummy = ram[0x3000'0000];
    }

    // We then perform the print
    *$v0 = PRINT_STRING;

    $start = "SYSCALL"_cpu;

    // Check that the block is swapped
    REQUIRE( is_swapped( inspector.RAM_swapped_addresses(), 0x1000'0000 ) );

    PC() = pc;
    cpu.single_step();

    REQUIRE( terminal->out_string == "[SYSCALL] print_string" );
  }

  SECTION( "[SYSCALL] print_string is executed (different blocks swapped)" )
  {
    auto is_swapped = [] ( std::vector<std::uint32_t> const& addresses, std::uint32_t address ) -> bool
    {
      for ( auto const & val : addresses )
        if ( address == val )
          return true;

      return false;
    };

    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    *$a0 = 0x3001'0000 - 0xD;

    terminal->in_string = "[SYSCALL] print_string";
    terminal->in_string += '\0'; // we must add it manually

    // We place the string into RAM through a syscall
    *$v0 = READ_STRING;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    // We then perform the print
    *$v0 = PRINT_STRING;

    // We need to force a swap of the blocks at address 0x3000'0000 and 0x3001'0000
    for ( int i = 0; i < 10; ++i )
    {
      volatile std::uint32_t _dummy;
      _dummy = ram[0x1000'0000];
      _dummy = ram[0x2000'0000];
    }

    $start = "SYSCALL"_cpu;

    // Check that the blocks are in memory
    REQUIRE( is_swapped( inspector.RAM_swapped_addresses(), 0x3000'0000 ) );
    REQUIRE( is_swapped( inspector.RAM_swapped_addresses(), 0x3001'0000 ) );

    PC() = pc;
    cpu.single_step();

    REQUIRE( terminal->out_string == "[SYSCALL] print_string" );
  }

  SECTION( "[SYSCALL] print_string is executed (block doesn't exist)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = PRINT_STRING;
    *$a0 = 0x1000'0000;

    $start = "SYSCALL"_cpu;
    PC() = pc;
    cpu.single_step();

    REQUIRE( terminal->out_string.empty() );
  }

  SECTION( "[SYSCALL] read_int is executed" )
  {
    auto $v0 = R( _v0 );

    *$v0 = READ_INT;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( *$v0 == ( std::uint32_t )terminal->in_int );
  }

  SECTION( "[SYSCALL] read_float is executed" )
  {
    auto $v0 = R( _v0 );
    auto $f0 = FP( 0 );

    *$v0 = READ_FLOAT;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( $f0->f == terminal->in_float );
  }

  SECTION( "[SYSCALL] read_double is executed" )
  {
    auto $v0 = R( _v0 );
    auto $f0 = FP( 0 );

    *$v0 = READ_DOUBLE;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( $f0->d == terminal->in_double );
  }

  SECTION( "[SYSCALL] read_string is executed (same block in memory)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    terminal->in_string = "[SYSCALL] read_string";

    *$v0 = READ_STRING;
    *$a0 = 0x0000'0000;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    auto str = inspector.RAM_read( 0x0000'0000, terminal->in_string.size() );

    REQUIRE( str.size() == terminal->in_string.size() );
    REQUIRE( std::memcmp( str.data(), terminal->in_string.data(), terminal->in_string.size() ) == 0 );
  }

  SECTION( "[SYSCALL] read_string is executed (different blocks in memory)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    terminal->in_string = "[SYSCALL] read_string";

    auto volatile _dummy_0 = ram[0x0001'0000 - 0xB];
    auto volatile _dummy_1 = ram[0x0001'0000];

    *$v0 = READ_STRING;
    *$a0 = 0x0001'0000 - 0xB;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    auto str = inspector.RAM_read( 0x0001'0000 - 0xB, terminal->in_string.size() );

    REQUIRE( str.size() == terminal->in_string.size() );
    REQUIRE( std::memcmp( str.data(), terminal->in_string.data(), terminal->in_string.size() ) == 0 );
  }

  SECTION( "[SYSCALL] read_string is executed (same block swapped)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    auto volatile _dummy_0 = ram[0x0000'0400];

    // We increase the access_count of 2 blocks, so the swapped one will be the one we need
    for ( int i = 0; i < 10; ++i )
    {
      std::uint32_t volatile _dummy_0;
      _dummy_0 = ram[RAM::block_size];
      _dummy_0 = ram[RAM::block_size * 2];
    }

    auto volatile _dummy_1 = ram[RAM::block_size * 3];

    // Check that is swapped
    REQUIRE( inspector.RAM_swapped_blocks_no() == 1 );
    REQUIRE( inspector.RAM_swapped_addresses()[0] == 0x0000'0000 );

    terminal->in_string = "[SYSCALL] read_string";

    *$v0 = READ_STRING;
    *$a0 = 0x0000'0400;
    *$a1 = terminal->in_string.size();

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    auto str = inspector.RAM_read( 0x400, terminal->in_string.size() );

    REQUIRE( str.size() == terminal->in_string.size() );
    REQUIRE( std::memcmp( str.data(), terminal->in_string.data(), terminal->in_string.size() ) == 0 );
  }

  SECTION( "[SYSCALL] read_string is executed (same non existent block)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    terminal->in_string = "[SYSCALL] read_string";

    *$v0 = READ_STRING;
    *$a0 = 0x1000'0000;
    *$a1 = terminal->in_string.size();

    // Check that the block doesn't exist
    auto const info = inspector.RAM_info();

    for ( auto const & addr : info.allocated_addresses )
      REQUIRE( addr != 0x1000'0000 );

    for ( auto const & addr : info.swapped_addresses )
      REQUIRE( addr != 0x1000'0000 );

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    auto str = inspector.RAM_read( 0x1000'0000, terminal->in_string.size() );

    REQUIRE( str.size() == terminal->in_string.size() );
    REQUIRE( std::memcmp( str.data(), terminal->in_string.data(), terminal->in_string.size() ) == 0 );
  }

  SECTION( "[SYSCALL] read_string is executed (different non existent block)" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    terminal->in_string = "[SYSCALL] read_string";

    *$v0 = READ_STRING;
    *$a0 = 0x1001'0000 - 0xE;
    *$a1 = terminal->in_string.size();

    // Check that the block doesn't exist
    auto const info = inspector.RAM_info();

    for ( auto const & addr : info.allocated_addresses )
    {
      REQUIRE( addr != 0x1000'0000 );
      REQUIRE( addr != 0x1001'0000 );
    }

    for ( auto const & addr : info.swapped_addresses )
    {
      REQUIRE( addr != 0x1000'0000 );
      REQUIRE( addr != 0x1001'0000 );
    }

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    auto alloc_addr = inspector.RAM_allocated_addresses();

    auto str = inspector.RAM_read( 0x1001'0000 - 0xE, terminal->in_string.size() );

    REQUIRE( str.size() == terminal->in_string.size() );
    REQUIRE( std::memcmp( str.data(), terminal->in_string.data(), terminal->in_string.size() ) == 0 );
  }

  SECTION( "[SYSCALL] sbrk is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = SBRK;
    *$a0 = 0;

    ram[0xBFC0'0000] = "EI"_cpu;
    ram[0xBFC0'0004] = "SYSCALL"_cpu;
    cpu.single_step();
    cpu.single_step();

    REQUIRE( PC() == 0x8000'0180 );
  }

  SECTION( "[SYSCALL] exit is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = EXIT;
    *$a0 = 0;

    $start = "SYSCALL"_cpu;

    REQUIRE( cpu.single_step() == 4 );
    REQUIRE( *$a0 == 0 );
  }

  SECTION( "[SYSCALL] print_char is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = PRINT_CHAR;
    *$a0 = 'n';

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( terminal->out_string == "n" );
  }

  SECTION( "[SYSCALL] read_char is executed" )
  {
    auto $v0 = R( _v0 );

    *$v0 = READ_CHAR;

    terminal->in_string = "_ABC";

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( ( char )*$v0 == '_' );
  }

  SECTION( "[SYSCALL] open is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );

    *$v0 = OPEN;
    *$a0 = 0x8000'0000;
    *$a1 = 0x00'62'2B'72; // r+b

    char const _name[] = "Donald_Duck.dat";

    auto * _ram_name = ( char* )std::addressof( ram[0x8000'0000] );

    for ( int i = 0; i < 16; ++i )
      _ram_name[i] = _name[i];

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( filehandler->param.name == _name );
    REQUIRE( filehandler->param.flags == "r+b" );
    REQUIRE( *$v0 == filehandler->fd_value );
  }

  SECTION( "[SYSCALL] read is executed" )
  {
    auto $v0 = R( _v0 );

    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );
    auto $a2 = R( _a2 );

    *$v0 = READ;

    *$a0 = 0xDDDD'EEEE;
    *$a1 = 0x8877'6655;
    *$a2 = 235;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( filehandler->param.fd == 0xDDDD'EEEE );
    REQUIRE( filehandler->param.dst != nullptr );
    REQUIRE( filehandler->param.count == 235 );
    REQUIRE( *$v0 == filehandler->read_count );
  }

  SECTION( "[SYSCALL] write is executed" )
  {
    auto $v0 = R( _v0 );

    auto $a0 = R( _a0 );
    auto $a1 = R( _a1 );
    auto $a2 = R( _a2 );

    *$v0 = WRITE;

    *$a0 = 0xAABB'EEDD;
    *$a1 = 0x3322'1100;
    *$a2 = 897;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( filehandler->param.fd == 0xAABB'EEDD );
    REQUIRE( filehandler->param.src == nullptr );
    REQUIRE( filehandler->param.count == 897 );
    REQUIRE( *$v0 == filehandler->write_count );
  }

  SECTION( "[SYSCALL] close is executed" )
  {
    auto $v0 = R( _v0 );

    auto $a0 = R( _a0 );

    *$v0 = CLOSE;

    *$a0 = 0xDDDD'EEEE;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( filehandler->param.fd == 0xDDDD'EEEE );
  }

  SECTION( "[SYSCALL] exit2 is executed" )
  {
    auto $v0 = R( _v0 );
    auto $a0 = R( _a0 );

    *$v0 = EXIT2;
    *$a0 = 2537;

    $start = "SYSCALL"_cpu;

    REQUIRE( cpu.single_step() == 4 );
    REQUIRE( *$a0 == 2537 );
  }

  SECTION( "SYSCALL is executed with an incorrect value in $v0" )
  {
    auto $v0 = R( _v0 );

    *$v0 = -1;

    $start = "SYSCALL"_cpu;
    cpu.single_step();

    REQUIRE( ExCause() == 8 );
  }

  SECTION( "The CPU fetches an instruction from an address that doesn't have access to" )
  {
     // forcing User Mode
    cp0.status &= ~0x1E;
    cp0.status |= 0x10;

    $start = "SIGRIE"_cpu; // this will never be executed

    cpu.single_step();

    REQUIRE( ExCause() == 4 );
    REQUIRE( PC() == 0x8000'0180 );
  }

  SECTION( "I read or write to the exit code" )
  {
    inspector.CPU_write_exit_code( -5423 );

    REQUIRE( ui32( -5423 ) == inspector.CPU_read_exit_code() );
  }
}

#undef HasOverflowed
#undef HasTrapped
#undef ExCause
#undef ClearExCause
#undef CAUSE
#undef PC
#undef FP
#undef R
#undef $start
