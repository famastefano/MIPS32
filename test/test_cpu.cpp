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

    *$2 = -48;
    *$3 = 21;

    ui32 const res = -48 + 21;

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

    auto $30 = R( 30 );

    auto $28 = R( 28 );

    *$30 = 0;
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

    auto $9 = R( 9 );

    auto $12 = R( 12 );

    auto $31 = R( 31 );

    *$9 = 0;
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

  WHEN( "BLEZC $18, 8 and BLEZC $4, 6 are executed" )
  {
    auto const _blezc_jump = "BLEZC"_cpu | 18_rt | 8_imm16;
    auto const _blezc_no_jump = "BLEZC"_cpu | 4_rt | 6_imm16;

    auto $18 = R( 18 );

    auto $4 = R( 4 );

    *$18 = 0;

    *$4 = 12;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 8 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _blezc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _blezc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGEZC $15, 99 and BGEZC $1, 17 are executed" )
  {
    auto const _bgezc_jump = "BGEZC"_cpu | 15_rs | 15_rt | 99_imm16;
    auto const _bgezc_no_jump = "BGEZC"_cpu | 1_rs | 1_rt | 17_imm16;

    auto $15 = R( 15 );

    auto $1 = R( 1 );

    *$15 = 5234;
    *$1 = -564;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 99 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgezc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgezc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGEC $31, $3, 905 and BGEC $30, $20, 54 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 905 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BLEC $31, $3, 905 and BLEC $30, $20, 54 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 905 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _blec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _blec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGTZC $20, 1111 and BGTZC $6, 25'896 are executed" )
  {
    auto const _bgtzc_jump = "BGTZC"_cpu | 20_rt | 1111_imm16;
    auto const _bgtzc_no_jump = "BGTZC"_cpu | 6_rt | 25896_imm16;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 65;

    *$6 = 0;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgtzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgtzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }
  WHEN( "BLTZC $20, 1111 and BLTZC $6, 25'896 are executed" )
  {
    auto const _bltzc_jump = "BLTZC"_cpu | 20_rs | 20_rt | 1111_imm16;
    auto const _bltzc_no_jump = "BLTZC"_cpu | 6_rs | 6_rt | 25896_imm16;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = -20;

    *$6 = 0;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bltzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bltzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BLTC $20, $21, 1111 and BLTC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bltc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bltc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGTC $20, $21, 1111 and BGTC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgtc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgtc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGEUC $20, $21, 1111 and BGEUC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgeuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgeuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BLEUC $20, $21, 1111 and BLEUC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bleuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bleuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BLTUC $20, $21, 1111 and BLTUC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bltuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bltuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BGTUC $20, $21, 1111 and BGTUC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bgtuc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bgtuc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BEQC $20, $21, 1111 and BEQC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _beqc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _beqc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BNEC $20, $21, 1111 and BNEC $6, $7, 25'896 are executed" )
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

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 1111 << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _bnec_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _bnec_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BEQZC $20, 0xF'FFFF and BEQZC $6, 0xA'BCDE are executed" )
  {
    auto const _beqzc_jump = "BEQZC"_cpu | 20_rs | 0xF'FFFF;
    auto const _beqzc_no_jump = "BEQZC"_cpu | 6_rs | 0xA'BCDE;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 0;

    *$6 = -423;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 0xF'FFFF << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _beqzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _beqzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BNEZC $20, 0xA'BCDE and BNEZC $6, 21 are executed" )
  {
    auto const _beqzc_jump = "BNEZC"_cpu | 20_rs | 0xA'BCDE;
    auto const _beqzc_no_jump = "BNEZC"_cpu | 6_rs | 21;

    auto $20 = R( 20 );

    auto $6 = R( 6 );

    *$20 = 1;

    *$6 = 0;

    auto const pc = PC();

    auto const res_jump = pc + 4 + ( 0xA'BCDE << 2 );
    auto const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      PC() = pc;
      $start = _beqzc_jump;
      cpu.single_step();
      REQUIRE( PC() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      PC() = pc;
      $start = _beqzc_no_jump;
      cpu.single_step();
      REQUIRE( PC() == res_no_jump );
    }
  }

  WHEN( "BREAK is executed" )
  {
    $start = "BREAK"_cpu;

    THEN( "It shall return 'breakpoint' as exception" )
    {
      REQUIRE( cpu.single_step() == 3 );
    }
  }

  WHEN( "CLO $10, $15 and CLO $21, $0 are executed" )
  {
    auto const _clo_15 = "CLO"_cpu | 10_rd | 15_rs;
    auto const _clo_0 = "CLO"_cpu | 21_rd | 0_rs;

    auto $10 = R( 10 );
    auto $15 = R( 15 );

    auto $21 = R( 21 );

    auto const pc = PC();

    auto const res_EBF8_9D0A = 3;
    auto const res_FFFF_FFFF = 32;
    auto const res_0000_0000 = 0;

    THEN( "0xEBF8'9D0A shall return 3" )
    {
      PC() = pc;
      $start = _clo_15;
      *$15 = 0xEBF8'9D0A;
      cpu.single_step();
      REQUIRE( *$10 == res_EBF8_9D0A );
    }
    AND_THEN( "0xFFFF'FFFF shall return 32" )
    {
      PC() = pc;
      $start = _clo_15;
      *$15 = 0xFFFF'FFFF;
      cpu.single_step();
      REQUIRE( *$10 == res_FFFF_FFFF );
    }
    AND_THEN( "0x0000'0000 shall return 0" )
    {
      PC() = pc;
      $start = _clo_0;
      cpu.single_step();
      REQUIRE( *$21 == res_0000_0000 );
    }
  }

  WHEN( "CLZ $10, $15 and CLZ $21, $0 are executed" )
  {
    auto const _clo_15 = "CLZ"_cpu | 10_rd | 15_rs;
    auto const _clo_0 = "CLZ"_cpu | 21_rd | 0_rs;

    auto $10 = R( 10 );
    auto $15 = R( 15 );

    auto $21 = R( 21 );

    auto const pc = PC();

    auto const res_0604_7FEB = 5;
    auto const res_FFFF_FFFF = 0;
    auto const res_0000_0000 = 32;

    THEN( "0x0604'7FEB shall return 5" )
    {
      PC() = pc;
      $start = _clo_15;
      *$15 = 0x0604'7FEB;
      cpu.single_step();
      REQUIRE( *$10 == res_0604_7FEB );
    }
    AND_THEN( "0xFFFF'FFFF shall return 0" )
    {
      PC() = pc;
      $start = _clo_15;
      *$15 = 0xFFFF'FFFF;
      cpu.single_step();
      REQUIRE( *$10 == res_FFFF_FFFF );
    }
    AND_THEN( "0x0000'0000 shall return 32" )
    {
      PC() = pc;
      $start = _clo_0;
      cpu.single_step();
      REQUIRE( *$21 == res_0000_0000 );
    }
  }

  WHEN( "DI is executed" )
  {
    inspector.CP0_status() |= 1;
    $start = "DI"_cpu;
    cpu.single_step();

    THEN( "Interrupts shall be disabled" )
    {
      auto const int_disabled = inspector.CP0_status() & 1;
      REQUIRE( int_disabled == 0 );
    }
  }

  WHEN( "EI is executed" )
  {
    inspector.CP0_status() &= ~1;

    $start = "EI"_cpu;

    cpu.single_step();

    THEN( "Interrupts shall be enabled" )
    {
      auto const int_enabled = inspector.CP0_status() & 1;
      REQUIRE( int_enabled == 1 );
    }
  }

  WHEN( "DIV $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "DIVU $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "MOD $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "MODU $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  // TODO: test ERET

  // TODO: test EXT

  // TODO: test INS

  WHEN( "J 0x0279'DB24 is executed" )
  {
    auto const _j = "J"_cpu | 0x0279'DB24;

    auto const res = PC() & 0xF000'0000 | 0x0279'DB24 << 2;

    $start = _j;

    cpu.single_step();

    THEN( "The pc must hold the correct address" )
    {
      REQUIRE( PC() == res );
    }
  }

  WHEN( "JAL 2389 is executed" )
  {
    auto const _jal = "JAL"_cpu | 2389;

    auto $31 = R( 31 );

    auto const pc = PC();
    auto const res = pc & 0xF000'0000 | 2389 << 2;

    $start = _jal;

    cpu.single_step();

    THEN( "The pc must hold the correct address and $31 the previous value" )
    {
      REQUIRE( PC() == res );
      REQUIRE( *$31 == pc + 8 );
    }
  }

  WHEN( "JALR $1 and JALR $1, $2 are executed" )
  {
    auto const _jalr_31 = "JALR"_cpu | 1_rs | 31_rd;
    auto const _jalr_2 = "JALR"_cpu | 1_rs | 2_rd;

    auto $1 = R( 1 );
    auto $2 = R( 2 );
    auto $31 = R( 31 );

    *$1 = 0x8000'0000;

    auto const pc = PC();
    auto const ret = pc + 8;

    THEN( "It shall store pc into $31 in the 1st case" )
    {
      PC() = pc;
      $start = _jalr_31;
      cpu.single_step();

      REQUIRE( PC() == 0x8000'0000 );
      REQUIRE( *$31 == ret );
    }
    AND_THEN( "It shall store pc into $2 in the 2nd case" )
    {
      PC() = pc;
      $start = _jalr_2;
      cpu.single_step();

      REQUIRE( PC() == 0x8000'0000 );
      REQUIRE( *$2 == ret );
    }
  }

  WHEN( "JIALC $4, 21 is executed" )
  {
    auto const _jialc = "JIALC"_cpu | 4_rt | 21;

    auto $4 = R( 4 );
    auto $31 = R( 31 );

    *$4 = 0x8000'0000;

    auto const ret = PC() + 4;
    auto const new_pc = 0x8000'0000 + 21;

    $start = _jialc;
    cpu.single_step();

    THEN( "It shall jump correctly" )
    {
      REQUIRE( PC() == new_pc );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "JIC $16, 9345 is executed" )
  {
    auto const _jialc = "JIALC"_cpu | 4_rt | 9345;

    auto $4 = R( 4 );
    auto $31 = R( 31 );

    *$4 = 0xABCD'0000;

    auto const ret = PC() + 4;
    auto const new_pc = 0xABCD'0000 + 9345;

    $start = _jialc;
    cpu.single_step();

    THEN( "It shall jump correctly" )
    {
      REQUIRE( PC() == new_pc );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "JR $31 is executed" )
  {
    auto const _jr = "JR"_cpu | 31_rs;

    auto $31 = R( 31 );

    *$31 = 0x0024'3798;

    $start = _jr;
    cpu.single_step();

    THEN( "It shall jump correctly" )
    {
      REQUIRE( PC() == 0x0024'3798 );
    }
  }

  // TODO: test LB

  // TODO: test LH

  // TODO: test LW

  // TODO: test LDC1

  // TODO: test SB

  // TODO: test SH

  // TODO: test SW

  // TODO: test SDC1

  // TODO: test LSA

  WHEN( "LUI $29, 0xABCD is executed" )
  {
    auto const _lui = "LUI"_cpu | 29_rt | 0xABCD;

    auto $29 = R( 29 );

    $start = _lui;
    cpu.single_step();

    THEN( "The result must be correct" )
    {
      REQUIRE( *$29 == 0xABCD'0000 );
    }
  }

  WHEN( "MUL $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "MUH $4, $5, $6 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$4 == res );
    }
  }

  WHEN( "MULU $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "MUHU $4, $5, $6 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$4 == res );
    }
  }

  WHEN( "NAL is executed" )
  {
    auto $31 = R( 31 );

    auto const res = PC() + 8;

    $start = "NAL"_cpu;
    cpu.single_step();

    THEN( "The result must be correct" )
    {
      REQUIRE( *$31 == res );
    }
  }

  WHEN( "NOP is executed" )
  {
    auto const pc = PC() + 4;

    $start = "NOP"_cpu;
    cpu.single_step();

    THEN( "Nothing should happen" )
    {
      auto begin = inspector.CPU_gpr_begin();
      auto end = inspector.CPU_gpr_end();

      REQUIRE( pc == PC() );

      while ( begin != end )
        REQUIRE( *begin++ == 0 );
    }
  }

  WHEN( "NOR $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "OR $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "ORI $1, $2, 0x1234 is executed" )
  {
    auto const _nor = "ORI"_cpu | 1_rt | 2_rs | 0x1234;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0x0000'ABCD;

    auto const res = 0x0000'ABCD | 0x1234;

    $start = _nor;
    cpu.single_step();

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "ROTR $1, $2, 5 is executed" )
  {
    auto const _rotr = "ROTR"_cpu | 1_rd | 2_rt | 8_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0xABCD'1234;

    auto const res = 0xABCD'1234 >> 8 | 0xABCD'1234 << ( 32 - 8 );

    $start = _rotr;
    cpu.single_step();

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "ROTRV $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "SLL $1, $2, 18 is executed" )
  {
    auto const _sll = "SLL"_cpu | 1_rd | 2_rt | 18_shamt;

    auto $1 = R( 1 );
    auto $2 = R( 2 );

    *$2 = 0xABCD'1234;

    auto const res = 0xABCD'1234 << 18;

    $start = _sll;
    cpu.single_step();

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "SLLV $1, $2, $3 is executed" )
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

    THEN( "The result must be correct" )
    {
      REQUIRE( *$1 == res );
    }
  }

  WHEN( "SLT $1, $2, $3 and SLT $4, $5, $6 are executed" )
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

    auto const pc = PC();
    
    THEN( "It should be set in the 1st case" )
    {
      PC() = pc;
      $start = _slt_set;
      cpu.single_step();

      REQUIRE( *$1 == 1 );
    }
    AND_THEN( "It should be clear in the 2nd case" )
    {
      PC() = pc;
      $start = _slt_clear;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  WHEN( "SLTI $2, $3, 29 and SLTI $5, $6, 68 are executed" )
  {
    auto const _slti_set = "SLTI"_cpu | 2_rt | 3_rs | 29;
    auto const _slti_clear = "SLTI"_cpu | 5_rt | 6_rs | 68;

    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$3 = 0;

    *$6 = 235;

    auto const pc = PC();

    THEN( "It should be set in the 1st case" )
    {
      PC() = pc;
      $start = _slti_set;
      cpu.single_step();

      REQUIRE( *$2 == 1 );
    }
    AND_THEN( "It should be clear in the 2nd case" )
    {
      PC() = pc;
      $start = _slti_clear;
      cpu.single_step();

      REQUIRE( *$5 == 0 );
    }
  }

  WHEN( "SLTU $1, $2, $3 and SLTU $4, $5, $6 are executed" )
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

    auto const pc = PC();

    THEN( "It should be set in the 1st case" )
    {
      PC() = pc;
      $start = _sltu_set;
      cpu.single_step();

      REQUIRE( *$1 == 1 );
    }
    AND_THEN( "It should be clear in the 2nd case" )
    {
      PC() = pc;
      $start = _sltu_clear;
      cpu.single_step();

      REQUIRE( *$4 == 0 );
    }
  }

  WHEN( "SLTIU $2, $3, 29 and SLTIU $5, $6, 68 are executed" )
  {
    auto const _sltiu_set = "SLTIU"_cpu | 2_rt | 3_rs | 29;
    auto const _sltiu_clear = "SLTIU"_cpu | 5_rt | 6_rs | 68;

    auto $2 = R( 2 );
    auto $3 = R( 3 );

    auto $5 = R( 5 );
    auto $6 = R( 6 );

    *$3 = 14;

    *$6 = 235;

    auto const pc = PC();

    THEN( "It should be set in the 1st case" )
    {
      PC() = pc;
      $start = _sltiu_set;
      cpu.single_step();

      REQUIRE( *$2 == 1 );
    }
    AND_THEN( "It should be clear in the 2nd case" )
    {
      PC() = pc;
      $start = _sltiu_clear;
      cpu.single_step();

      REQUIRE( *$5 == 0 );
    }
  }
}

#undef PC
#undef R