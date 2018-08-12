#include <catch.hpp>

#include <mips32/cpu.hpp>
#include <mips32/machine_inspector.hpp>

#include "helpers/test_cpu_instructions.hpp"

using namespace mips32;

using ui32 = std::uint32_t;

// TODO: test for $zero that always reads/writes as 0.
// TODO: test for exceptions.
// TODO: test for simple executions, like an hello world program.

#define R(n) inspector.CPU_gpr_begin() + n
#define PC() inspector.CPU_pc()

SCENARIO( "A CPU object exists" )
{
  MachineInspector inspector;

  RAM ram{ RAM::block_size };
  CPU cpu{ ram };

  auto &$start = ram[0xBFC0'0000];

  inspector
    .inspect( ram )
    .inspect( cpu );

  cpu.hard_reset();

  WHEN( "ADD $1, $2, $3 is executed" )
  {
    auto const _add = "ADD"_cpu | 1_rd | 2_rs | 3_rt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $3 = R( 3 );

    *$2 = 48;
    *$3 = -21;

    ui32 const res = 48 + -21;

    $start = _add;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "ADDIU $21, $3, 32'000 is executed" )
  {
    auto const _addiu = "ADDIU"_cpu | 21_rt | 3_rs | 32000_imm16;

    auto $21 = R( 21 );
    auto $3 = R( 3 );

    *$3 = 123'098;

    ui32 const res = 123'098 + 32'000;

    $start = _addiu;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$21 == res );
    }
  }

  WHEN( "ADDIUPC $30, 16 is executed" )
  {
    auto const _addiupc = "ADDIUPC"_cpu | 30_rs | 16_imm19;

    auto $30 = R( 30 );

    ui32 const res = PC() + ( 16 << 2 );

    $start = _addiupc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$30 == res );
    }
  }

  WHEN( "ADDU $6, $8, $10 is executed" )
  {
    auto const _addu = "ADDU"_cpu | 6_rd | 8_rs | 10_rt;

    auto $6 = R( 6 );
    auto $8 = R( 8 );
    auto $10 = R( 10 );

    *$8 = 305;
    *$10 = 3894;

    ui32 const res = 305 + 3894;

    $start = _addu;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$6 == res );
    }
  }

  // TODO: test ALUIPC

  WHEN( "AND $5, $10, $15 is executed" )
  {
    auto const _and = "AND"_cpu | 5_rd | 10_rs | 15_rt;

    auto $5 = R( 5 );
    auto $10 = R( 10 );
    auto $15 = R( 15 );

    *$10 = 0xFFFF'FFFF;
    *$15 = 0b1010'1010'1010'1010'1010'1010'1010'1010;

    ui32 const res = 0xFFFF'FFFF & 0b1010'1010'1010'1010'1010'1010'1010'1010;

    $start = _and;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$5 == res );
    }
  }

  WHEN( "ANDI $4, $18 is executed" )
  {
    auto const _andi = "ANDI"_cpu | 4_rt | 18_rs | 0xCEED_imm16;

    auto $4 = R( 4 );
    auto $18 = R( 18 );

    *$18 = 0xFFFF'FFFF;

    ui32 const res = 0xFFFF'FFFF & 0xCEED;

    $start = _andi;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$4 == res );
    }
  }

  WHEN( "AUI $8, $20 is executed" )
  {
    auto const _aui = "AUI"_cpu | 8_rt | 20_rs | 0xCCAA_imm16;

    auto $8 = R( 8 );
    auto $20 = R( 20 );

    *$20 = 0;

    ui32 const res = 0xCCAA'0000;

    $start = _aui;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$8 == res );
    }
  }

  WHEN( "AUIPC $2, 36 is executed" )
  {
    auto const _auipc = "AUIPC"_cpu | 2_rs | 36_imm16;

    auto $2 = R( 2 );

    ui32 const res = PC() + ( 36 << 16 );

    $start = _auipc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( *$2 == res );
    }
  }

  WHEN( "BAL 768 is executed" )
  {
    auto const _bal = "BAL"_cpu | 768_imm16;

    auto $31 = R( 31 );

    ui32 const res = PC() + 4 + ( 768 << 2 );
    ui32 const ret = PC() + 8;

    $start = _bal;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( PC() == res );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "BALC 0x0123'4567 is executed" )
  {
    auto const _balc = "BALC"_cpu | 0x01234567_imm26;

    auto $31 = inspector.CPU_gpr_begin() + 31;

    ui32 const res = PC() + 4 + ( 0x01234567 << 2 );
    ui32 const ret = PC() + 4;

    $start = _balc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( PC() == res );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "BC 0x02BC'DEFF is executed" )
  {
    auto const _bc = "BC"_cpu | 0x02BCDEFF_imm26;

    ui32 const res = PC() + 4 + 0xFAF3'7BFC;
    ui32 const ret = PC() + 4;

    $start = _bc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( PC() == res );
    }
  }

  WHEN( "BEQ $0, $0, 81 and BEQ $1, $2, 81 are executed" )
  {
    auto const _beq_jump = "BEQ"_cpu | 0_rs | 0_rt | 81_imm16;
    auto const _beq_no_jump = "BEQ"_cpu | 1_rs | 2_rt | 81_imm16;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$1 = 44;
    *$2 = -44;

    ui32 const pc = PC();

    ui32 const res_jump = pc + 4 + ( 81 << 2 );
    ui32 const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _beq_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _beq_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGEZ $30, 128 and BGEZ $28, 128 are executed" )
  {
    auto const _bgez_jump = "BGEZ"_cpu | 30_rs | 128_imm16;
    auto const _bgez_no_jump = "BGEZ"_cpu | 28_rs | 128_imm16;

    auto $28 = R( 28 );

    *$28 = -1;

    ui32 const pc = PC();

    ui32 const res_jump = pc + 4 + ( 128 << 2 );
    ui32 const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgez_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgez_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BLEZALC $9, 1 and BLEZALC $12, 256 are executed" )
  {
    auto const _blezalc_jump = "BLEZALC"_cpu | 9_rt | 1_imm16;
    auto const _blezalc_no_jump = "BLEZALC"_cpu | 12_rt | 256_imm16;

    auto $12 = R( 12 );

    auto $31 = R( 31 );

    *$12 = 24;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _blezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _blezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  WHEN( "BGEZALC $13, 14 and BGEZALC $19, 964 are executed" )
  {
    auto const _bgezalc_jump = "BGEZALC"_cpu | 13_rs | 13_rt | 14_imm16;
    auto const _bgezalc_no_jump = "BGEZALC"_cpu | 19_rs | 19_rt | 964_imm16;

    auto $13 = R( 13 );
    auto $19 = R( 19 );

    auto $31 = R( 31 );

    *$13 = 534;

    *$19 = -3498;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 14 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  WHEN( "BGTZALC $8, 97 and BGTZALC $1, 1000 are executed" )
  {
    auto const _bgtzalc_jump = "BGTZALC"_cpu | 8_rt | 97_imm16;
    auto const _bgtzalc_no_jump = "BGTZALC"_cpu | 1_rt | 1000_imm16;

    auto $8 = R( 8 );

    auto $1 = R( 1 );

    auto $31 = R( 31 );

    *$8 = 534;
    *$1 = 0;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 97 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgtzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgtzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  WHEN( "BLTZALC $17, 77 and BLTZALC $20, 93 are executed" )
  {
    auto const _bltzalc_jump = "BLTZALC"_cpu | 17_rs | 17_rt | 77_imm16;
    auto const _bltzalc_no_jump = "BLTZALC"_cpu | 20_rs | 20_rt | 93_imm16;

    auto $17 = R( 17 );
    auto $20 = R( 20 );

    auto $31 = R( 31 );

    *$17 = -8947;

    *$20 = 0;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 77 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bltzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bltzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  WHEN( "BEQZALC $24, 816 and BEQZALC $11, 789 are executed" )
  {
    auto const _beqzalc_jump = "BEQZALC"_cpu | 24_rt | 816_imm16;
    auto const _beqzalc_no_jump = "BEQZALC"_cpu | 11_rt | 789_imm16;

    auto $24 = R( 24 );
    auto $11 = R( 11 );

    auto $31 = R( 31 );

    *$24 = 0;

    *$11 = 1;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 816 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _beqzalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _beqzalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }

  WHEN( "BNEZALC $17, 1598 and BNEZALC $20, 795 are executed" )
  {
    auto const _bnezalc_jump = "BNEZALC"_cpu | 17_rt | 1598_imm16;
    auto const _bnezalc_no_jump = "BNEZALC"_cpu | 20_rt | 795_imm16;

    auto $17 = R( 17 );
    auto $20 = R( 20 );

    auto $31 = R( 31 );

    *$17 = 0;

    *$20 = 1;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1598 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bnezalc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
      REQUIRE( *$31 == pc + 4 );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bnezalc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
      REQUIRE( *$31 == pc + 4 );
    }
  }
}

#undef PC
#undef R