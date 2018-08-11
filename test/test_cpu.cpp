#include <catch.hpp>

#include <mips32/cpu.hpp>
#include <mips32/machine_inspector.hpp>

#include "helpers/test_cpu_instructions.hpp"

using namespace mips32;

using ui32 = std::uint32_t;

// TODO: test for $zero that always reads/writes as 0.
// TODO: test for exceptions.
// TODO: test for simple executions, like an hello world program.


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

    auto $1 = inspector.CPU_gpr_begin() + 1;
    auto $2 = inspector.CPU_gpr_begin() + 2;
    auto $3 = inspector.CPU_gpr_begin() + 3;

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

    auto $21 = inspector.CPU_gpr_begin() + 21;
    auto $3 = inspector.CPU_gpr_begin() + 3;

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

    auto $30 = inspector.CPU_gpr_begin() + 30;

    ui32 const res = inspector.CPU_pc() + ( 16 << 2 );

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

    auto $6 = inspector.CPU_gpr_begin() + 6;
    auto $8 = inspector.CPU_gpr_begin() + 8;
    auto $10 = inspector.CPU_gpr_begin() + 10;

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

    auto $5 = inspector.CPU_gpr_begin() + 5;
    auto $10 = inspector.CPU_gpr_begin() + 10;
    auto $15 = inspector.CPU_gpr_begin() + 15;

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

    auto $4 = inspector.CPU_gpr_begin() + 4;
    auto $18 = inspector.CPU_gpr_begin() + 18;

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

    auto $8 = inspector.CPU_gpr_begin() + 8;
    auto $20 = inspector.CPU_gpr_begin() + 20;

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

    auto $2 = inspector.CPU_gpr_begin() + 2;

    ui32 const res = inspector.CPU_pc() + ( 36 << 16 );

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

    auto $31 = inspector.CPU_gpr_begin() + 31;

    ui32 const res = inspector.CPU_pc() + 4 + ( 768 << 2 );
    ui32 const ret = inspector.CPU_pc() + 8;

    $start = _bal;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( inspector.CPU_pc() == res );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "BALC 0x0123'4567 is executed" )
  {
    auto const _balc = "BALC"_cpu | 0x01234567_imm26;

    auto $31 = inspector.CPU_gpr_begin() + 31;

    ui32 const res = inspector.CPU_pc() + 4 + ( 0x01234567 << 2 );
    ui32 const ret = inspector.CPU_pc() + 4;

    $start = _balc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( inspector.CPU_pc() == res );
      REQUIRE( *$31 == ret );
    }
  }

  WHEN( "BC 0x02BC'DEFF is executed" )
  {
    auto const _bc = "BC"_cpu | 0x02BCDEFF_imm26;

    ui32 const res = inspector.CPU_pc() + 4 + 0xFAF3'7BFC;
    ui32 const ret = inspector.CPU_pc() + 4;

    $start = _bc;

    THEN( "The result must be correct" )
    {
      cpu.single_step();
      REQUIRE( inspector.CPU_pc() == res );
    }
  }

  WHEN( "BEQ $0, $0, 81 and BEQ $1, $2 are executed" )
  {
    auto const _beq_jump = "BEQ"_cpu | 0_rs | 0_rt | 81_imm16;
    auto const _beq_no_jump = "BEQ"_cpu | 1_rs | 2_rt | 81_imm16;

    auto $1 = inspector.CPU_gpr_begin() + 1;
    auto $2 = inspector.CPU_gpr_begin() + 2;

    *$1 = 44;
    *$2 = -44;

    ui32 const pc = inspector.CPU_pc();

    ui32 const res_jump = pc + 4 + ( 81 << 2 );
    ui32 const res_no_jump = pc + 4;

    THEN( "It shall jump in the 1st case" )
    {
      inspector.CPU_pc() = pc;
      $start = _beq_jump;
      cpu.single_step();
      REQUIRE( inspector.CPU_pc() == res_jump );
    }
    AND_THEN( "It shall not jump in the 2nd case" )
    {
      inspector.CPU_pc() = pc;
      $start = _beq_no_jump;
      cpu.single_step();
      REQUIRE( inspector.CPU_pc() == res_no_jump );
    }
  }
}
