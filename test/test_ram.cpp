#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <RAM/ram.hpp>
#include <machine_inspector.hpp>

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

SCENARIO( "RAM can be inspected" )
{
  GIVEN( "A RAM with 4GB of capacity and 256MB of allocation limit" )
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
  }
}