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
}

#undef PC
#undef R