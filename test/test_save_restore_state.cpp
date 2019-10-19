#include <catch.hpp>

#include <mips32/machine_inspector.hpp>
#include "../src/cpu.hpp"

#include <algorithm>
#include <cstring>

using namespace mips32;
using namespace mips32::literals;

constexpr char const state_name[] = { "test_save_restore_state" };

TEST_CASE( "A functional machine exists" )
{
  RAM ram{ 192_KB };

  CPU cpu{ ram };

  MachineInspector inspector;

  inspector
    .inspect( ram )
    .inspect( cpu );

  cpu.hard_reset();

  SECTION( "I save and restore CP0" )
  {
    auto & cp0 = inspector.access_CP0();

    CP0 cp0_copy = cp0;

    REQUIRE_FALSE( std::memcmp( &cp0, &cp0_copy, sizeof( CP0 ) ) );

    auto _s = inspector.save_state( MachineInspector::Component::CP0, state_name );
    REQUIRE_FALSE( _s );

    // Modifying some data to check if the restore succeed
    cp0.bad_instr = 1;
    cp0.bad_vaddr = 2;
    cp0.cause = 3;
    cp0.config[0] = 4;
    cp0.config[1] = 5;
    cp0.config[2] = 6;
    cp0.config[3] = 7;

    auto _r = inspector.restore_state( MachineInspector::Component::CP0, state_name );
    REQUIRE_FALSE( _r );
    REQUIRE_FALSE( std::memcmp( &cp0, &cp0_copy, sizeof( CP0 ) ) );
  }

  SECTION( "I save and restore CP1" )
  {
    CP1 cp1_copy;
    cp1_copy.reset();
    MachineInspector copy_inspector;
    copy_inspector.inspect( cp1_copy );

    auto _begin = inspector.CP1_fpr_begin();
    auto const _end = inspector.CP1_fpr_end();
    auto _begin_copy = copy_inspector.CP1_fpr_begin();
    while ( _begin != _end )
    {
      _begin_copy->i64 = _begin->i64;
      ++_begin;
      ++_begin_copy;
    }

    auto _s = inspector.save_state( MachineInspector::Component::CP1, state_name );
    REQUIRE_FALSE( _s );

    for ( auto reg = inspector.CP1_fpr_begin(); reg != inspector.CP1_fpr_end(); ++reg )
    {
      reg->i64 = _end - reg;
    }

    auto _r = inspector.restore_state( MachineInspector::Component::CP1, state_name );
    REQUIRE_FALSE( _r );

    REQUIRE( copy_inspector.CP1_fcsr() == inspector.CP1_fcsr() );
    REQUIRE( copy_inspector.CP1_fir() == inspector.CP1_fir() );

    _begin = inspector.CP1_fpr_begin();
    _begin_copy = copy_inspector.CP1_fpr_begin();
    while ( _begin != _end )
    {
      REQUIRE( _begin_copy->i64 == _begin->i64 );
      ++_begin;
      ++_begin_copy;
    }
  }

  SECTION( "I save and restore RAM" )
  {
    ram[0x0000'0000] = 0x0000'0000;
    ram[0x0004'0000] = 0x0004'0000;
    ram[0x0500'0000] = 0x0500'0000;

    for ( int i = 0; i < RAM::block_size; i += 4 )
      ram[0x8000'0000 + i] = i;

    auto const ram_info = inspector.RAM_info();

    REQUIRE( ram_info.allocated_blocks_no == 3 );
    REQUIRE( ram_info.swapped_blocks_no == 1 );

    auto _s = inspector.save_state( MachineInspector::Component::RAM, state_name );
    REQUIRE_FALSE( _s );

    ram[0x0000'0000] = 0xFFFF'FFFF;
    ram[0x0004'0000] = 0xFFFF'FFFF;
    ram[0x0500'0000] = 0xFFFF'FFFF;

    for ( int i = 0; i < RAM::block_size; i += 4 )
      ram[0x8000'0000 + i] = 0xFFFF'FFFF;

    auto _r = inspector.restore_state( MachineInspector::Component::RAM, state_name );

    auto new_info = inspector.RAM_info();

    REQUIRE_FALSE( _r );

    REQUIRE( ram_info.alloc_limit == new_info.alloc_limit );

    REQUIRE( ram_info.allocated_blocks_no == new_info.allocated_blocks_no );

    REQUIRE( ram_info.swapped_blocks_no == new_info.swapped_blocks_no );

    REQUIRE( std::equal( ram_info.allocated_addresses.cbegin(),
                         ram_info.allocated_addresses.cend(),
                         new_info.allocated_addresses.cbegin() ) );

    REQUIRE( std::equal( ram_info.swapped_addresses.cbegin(),
                         ram_info.swapped_addresses.cend(),
                         new_info.swapped_addresses.cbegin() ) );

    REQUIRE( ram[0x0000'0000] == 0x0000'0000 );
    REQUIRE( ram[0x0004'0000] == 0x0004'0000 );
    REQUIRE( ram[0x0500'0000] == 0x0500'0000 );

    for ( int i = 0; i < RAM::block_size; i += 4 )
      REQUIRE( ram[0x8000'0000 + i] == i );
  }

  SECTION( "I save and restore CPU" )
  {
    MachineInspector copy_inspector;
    CPU copy_cpu{ ram };
    copy_cpu.hard_reset();

    copy_inspector.inspect( copy_cpu );

    std::copy( inspector.CPU_gpr_begin(),
               inspector.CPU_gpr_end(),
               copy_inspector.CPU_gpr_begin() );

    copy_inspector.CPU_pc() = inspector.CPU_pc();

    REQUIRE_FALSE( inspector.save_state( MachineInspector::Component::CPU, state_name ) );

    auto begin = inspector.CPU_gpr_begin();
    auto end = inspector.CPU_gpr_end();
    while ( begin != end )
      *begin++ = 42;

    inspector.CPU_pc() = 0xAABB'CCDD;
    inspector.CPU_write_exit_code( 142 );

    REQUIRE_FALSE( inspector.restore_state( MachineInspector::Component::CPU, state_name ) );

    REQUIRE( std::equal( inspector.CPU_gpr_begin(),
                         inspector.CPU_gpr_end(),
                         copy_inspector.CPU_gpr_begin() ) );

    REQUIRE( inspector.CPU_pc() == copy_inspector.CPU_pc() );

    REQUIRE_FALSE( inspector.CPU_read_exit_code() );
  }

  SECTION( "I save and restore the entire Machine" )
  {
    // Just to increase the coverage
    inspector.save_state( MachineInspector::Component::ALL, state_name );
    inspector.restore_state( MachineInspector::Component::ALL, state_name );
  }
}
