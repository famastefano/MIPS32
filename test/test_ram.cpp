#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <mips32/machine_inspector.hpp>
#include <mips32/ram.hpp>

/*
struct RAMInfo
  {
    word              capacity;
    word              alloc_limit;
    word              block_size;
    word              allocated_blocks_no;
    word              swapped_blocks_no;
    std::vector<word> allocated_addresses;
    std::vector<word> swapped_addresses;
  };
*/

using namespace mips32;
using namespace mips32::literals;

SCENARIO( "A RAM Object exists" )
{
  GIVEN( "A RAM with 256MB of allocation limit" )
  {
    MachineInspector inspector;

    RAM ram{256_MB};
    inspector.inspect( ram );

    WHEN( "I ask for RAM's infos" )
    {
      auto info = inspector.RAM_info();

      THEN( "The inspector returns the correct informations." )
      {
        REQUIRE( info.alloc_limit == 256_MB );
        REQUIRE( info.block_size == RAM::block_size );
        REQUIRE( info.allocated_blocks_no == 0 );
        REQUIRE( info.swapped_blocks_no == 0 );
        REQUIRE( info.allocated_addresses.size() == 0 );
        REQUIRE( info.swapped_addresses.size() == 0 );
      }
    }

    WHEN( "I access a word" )
    {
      ram[0];

      THEN( "It's block shall be allocated" )
      {
        REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
        REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 0 ) );
      }
    }

    WHEN( "I write to a word" )
    {
      ram[0] = 0xABCD'0123;

      THEN( "Its value shall be stored" ) { REQUIRE( ram[0] == 0xABCD'0123 ); }
    }

    WHEN( "I write to a sequence of words in the same block" )
    {
      for ( std::uint32_t i = 0; i < 256 * 4; i += 4 ) ram[i] = i;

      THEN( "I shall retrieve those values by reading them back" )
      {
        for ( std::uint32_t i = 0; i < 256 * 4; i += 4 ) REQUIRE( ram[i] == i );
      }
    }
  }

  GIVEN( "A little RAM that can allocate 1 block only" )
  {
    MachineInspector inspector;

    RAM ram{RAM::block_size};

    inspector.inspect( ram );

    WHEN( "I access 2 blocks" )
    {
      ram[0];
      ram[RAM::block_size];

      THEN( "Only 1 block shall be allocated and 1 swapped" )
      {
        REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
        REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 1 ) );
      }
    }

    WHEN( "I access more blocks after the first allocation" )
    {
      for ( std::uint32_t i = 0; i < RAM::block_size * 10;
            i += RAM::block_size )
        ram[i];

      THEN( "Only 1 block shall be allocated and the rest are swapped" )
      {
        REQUIRE( inspector.RAM_allocated_blocks_no() == std::uint32_t( 1 ) );
        REQUIRE( inspector.RAM_swapped_blocks_no() == std::uint32_t( 9 ) );
      }
    }

    WHEN( "I access a swapped block" )
    {
      ram[0];
      ram[RAM::block_size];

      REQUIRE( inspector.RAM_swapped_addresses().back() == std::uint32_t( 0 ) );

      ram[0];

      THEN( "That block shall be present in memory" )
      {
        REQUIRE( inspector.RAM_swapped_addresses()[0] == RAM::block_size );
        REQUIRE( inspector.RAM_allocated_addresses()[0] == std::uint32_t( 0 ) );
      }
    }
  }
}
