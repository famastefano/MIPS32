#include <catch.hpp>

#include <mips32/machine_inspector.hpp>
#include "../src/ram.hpp"

using namespace mips32;
using namespace mips32::literals;

TEST_CASE( "A RAM Object exists with 256 MB of allocation limit" )
{
  MachineInspector inspector;

  RAM ram{ 256_MB };
  inspector.inspect( ram );

  SECTION( "I ask for RAM's infos" )
  {
    auto info = inspector.RAM_info();

    REQUIRE( info.alloc_limit == 256_MB );
    REQUIRE( info.block_size == RAM::block_size );
    REQUIRE( info.allocated_blocks_no == 0 );
    REQUIRE( info.swapped_blocks_no == 0 );
    REQUIRE( info.allocated_addresses.size() == 0 );
    REQUIRE( info.swapped_addresses.size() == 0 );
  }

  SECTION( "I access a word" )
  {
    ram[0];

    REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
    REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 0 ) );
  }

  SECTION( "I write to a word" )
  {
    ram[0] = 0xABCD'0123;

    REQUIRE( ram[0] == 0xABCD'0123 );
  }

  SECTION( "I write to a sequence of words in the same block" )
  {
    for ( std::uint32_t i = 0; i < 256 * 4; i += 4 )
      ram[i] = i;

    for ( std::uint32_t i = 0; i < 256 * 4; i += 4 )
      REQUIRE( ram[i] == i );
  }
}

TEST_CASE( "A RAM object exists and can allocate 1 block only" )
{
  MachineInspector inspector;

  RAM ram{ 64_KB };

  inspector.inspect( ram );

  SECTION( "I access 2 blocks" )
  {
    ram[0];
    ram[RAM::block_size];

    REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
    REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 1 ) );
  }

  SECTION( "I access more blocks after the first allocation" )
  {
    for ( std::uint32_t i = 0; i < RAM::block_size * 10; i += RAM::block_size )
      ram[i];

    REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
    REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 9 ) );
  }

  SECTION( "I access a swapped block" )
  {
    ram[0];
    ram[RAM::block_size];

    REQUIRE( inspector.RAM_swapped_addresses().back() == std::uint32_t( 0 ) );

    ram[0];

    REQUIRE( inspector.RAM_swapped_addresses()[0] == RAM::block_size );
    REQUIRE( inspector.RAM_allocated_addresses()[0] == std::uint32_t( 0 ) );
  }
}
