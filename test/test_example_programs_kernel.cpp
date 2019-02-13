#include <catch.hpp>

#include <mips32/machine_inspector.hpp>
#include "../src/cpu.hpp"

#include "helpers/Terminal.hpp"
#include "helpers/FileManager.hpp"
#include "helpers/test_cpu_instructions.hpp"
#include "helpers/test_cp1_instructions.hpp"

#include <cstring>
#include <memory>
#include <iterator>

#define R(n) inspector.CPU_gpr_begin() + n
#define FP(n) inspector.CP1_fpr_begin() + n
#define PC() inspector.CPU_pc()
#define ExCause() ( inspector.access_CP0().cause >> 2 & 0x1F)

using namespace mips32;
using namespace mips32::literals;

TEST_CASE( "A CPU runs a simple Hello World program in Kernel Mode" )
{
  auto terminal = std::make_unique<Terminal>();
  auto filemanager = std::make_unique<FileManager>();

  MachineInspector inspector;

  RAM ram{ 1_MB };
  CPU cpu{ ram };

  inspector.inspect( ram );
  inspector.inspect( cpu );

  cpu.attach_iodevice( terminal.get() );
  cpu.attach_file_handler( filemanager.get() );

  cpu.hard_reset();

  constexpr std::uint32_t data_segment = 0x0000'0000;
  constexpr std::uint32_t text_segment = 0x8000'0000;

  /**
   * Reads an integer `n` and prints 'Hello World!' `n` times
   * 
   * -- C-like pseudocode --
   * int n;
   * getint(&n);
   * 
   * while(n--)
   *    printf("Hello World!\n");
   * 
   * return 0;
   * 
   * -- Assembly --
   * 
   * .data 0x0000'0000
   * n: .word 0                       # .data+0
   * str: .asciiz "Hello World!\n"    # .data+4
   * 
   * .text 0x8000'0000
   * _start: jal main                 # .text+0
   *         nop
   *         li $v0, 17               # exit2
   *         syscall
   * 
   * main: li $v0, 5                  # read_integer, .text+3
   *       syscall
   *       sw $v0, 0($zero)           # saves n
   *  
   *       li $t1, 1
   * 
   * while_head: lw $t0, 0($zero)     # reads n,         .text+7
   *             beqzc $t0, while_end # if n == 0 break, .text+8
   *             subu $t0, $t0, $t1   # --n
   *             sw $t0, 0($zero)     # saves n
   *  
   *             li $v0, 4            # print_string
   *             la $a0, str          # &str
   *             syscall
   *             break                # we use this to check every print and that it loops exactly n times
   *  
   *             j while_head         # loop, text+15
   *  
   * while_end:  xor $a0, $a0         # .text+16
   *             jr $ra               # return 0
   * 
   * 
   * Due to the use of pseudo instructions more instructions are generated
   **/
  constexpr std::uint32_t machine_code[] =
  {
    // _start
    "JAL"_cpu | 5,
    "NOP"_cpu,
    "XOR"_cpu | 2_rd | 2_rs | 2_rt,
    "ORI"_cpu | 2_rt | 2_rs | 17,
    "SYSCALL"_cpu,

    // main
    "XOR"_cpu | 2_rd | 2_rs | 2_rt,
    "ORI"_cpu | 2_rt | 2_rs | 5,
    "SYSCALL"_cpu,
    "SW"_cpu | 2_rt | 0 | 0_rs,

    "XOR"_cpu | 9_rd | 9_rs | 9_rt,
    "ORI"_cpu | 9_rt | 9_rs | 1,

    // while_head
    "LW"_cpu | 8_rt | 0 | 0_rs,
    "BEQZC"_cpu | 8_rs | 9, // branches to while_end
    "SUBU"_cpu | 8_rd | 8_rs | 9_rt,
    "SW"_cpu | 8_rt | 0 | 0_rs,

    "XOR"_cpu | 2_rd | 2_rs | 2_rt,
    "ORI"_cpu | 2_rt | 2_rs | 4,
    "XOR"_cpu | 4_rd | 4_rs | 4_rt,
    "ORI"_cpu | 4_rt | 4_rs | 4,
    "SYSCALL"_cpu,
    "BREAK"_cpu,

    "J"_cpu | 11, // jumps to while_head

    // while_end
    "XOR"_cpu | 4_rd | 4_rs | 4_rt,
    "JR"_cpu | 31_rs,
  };

  char const * _data_str = "Hello World!\n\0";
  constexpr int _data_str_len = 14;

  ram[data_segment] = 0;
  std::memcpy( &ram[data_segment + 4], _data_str, _data_str_len );

  
  for ( std::uint32_t i = 0; i < std::size( machine_code ); ++i )
  {
    ram[text_segment + i*4] = machine_code[i];
  }
  

  //inspector.RAM_write( data_segment, _data_str, _data_str_len );
  //inspector.RAM_write( text_segment, machine_code, std::size( machine_code ) * sizeof( std::uint32_t ) );

  terminal->out_string = "UNDEFINED";

  PC() = 0x8000'0000;

  SECTION( "[n = 0] It shouldn't print anything" )
  {
    terminal->in_int = 0;
    auto const ret = cpu.start();

    REQUIRE( ret == CPU::EXIT );
    REQUIRE( terminal->out_string == "UNDEFINED" );
  }

  SECTION( "[n = 15] It should print 15 times" )
  {
    terminal->in_int = 15;

    for ( int i = 0; i < 15; ++i )
    {
      auto const res = cpu.start();
      auto const ex = ExCause();

      REQUIRE( terminal->out_string == _data_str );
      REQUIRE( res == CPU::EXCEPTION );
      REQUIRE( ex == CPU::Bp );
    }

    auto const res = cpu.start();
    REQUIRE( res == CPU::EXIT );
  }

  SECTION( "[n = 10] It should print 10 times, n is modified manually" )
  {
    terminal->in_int = 142;

    auto const res = cpu.start();
    auto const ex = ExCause();

    REQUIRE( ram[data_segment] == 141 );
    REQUIRE( terminal->out_string == _data_str );
    REQUIRE( res == CPU::EXCEPTION );
    REQUIRE( ex == CPU::Bp );

    ram[data_segment] = 9;

    for ( int i = 0; i < 9; ++i )
    {
      auto const res = cpu.start();
      auto const ex = ExCause();

      REQUIRE( terminal->out_string == _data_str );
      REQUIRE( res == CPU::EXCEPTION );
      REQUIRE( ex == CPU::Bp );
    }

    auto const res_final = cpu.start();
    REQUIRE( res_final == CPU::EXIT );
  }
}

#undef ExCause
#undef R
#undef FP
#undef PC