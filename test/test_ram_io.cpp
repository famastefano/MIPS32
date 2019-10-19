#include <catch.hpp>

#include "../src/ram.hpp"
#include "../src/ram_io.hpp"
#include <mips32/machine_inspector.hpp>

#include <cstring>

using namespace mips32;

TEST_CASE( "A RAM instance is used to perform IO" )
{
  RAM ram{ 1_MB };
  RAMIO ram_io{ ram };

  MachineInspector inspector;
  inspector.inspect( ram );

  SECTION( "1 Block - at start" )
  {
    constexpr int size = 128;
    constexpr int addr = 0x0;

    unsigned char raw_data[size];

    for ( int i = 0; i < size; ++i )
      raw_data[i] = ( unsigned char )i;

    ram_io.write( addr, raw_data, size );

    auto info = inspector.RAM_info();

    REQUIRE( info.allocated_blocks_no == 1 );
    REQUIRE( info.allocated_addresses[0] == 0 );

    auto read = inspector.RAM_read( addr, size );

    REQUIRE_FALSE( read.empty() );
    REQUIRE( std::memcmp( read.data(), raw_data, size ) == 0 );
  }

  SECTION( "1 Block - at offset" )
  {
    constexpr int size = 256;
    constexpr int addr = 0x879;

    unsigned char raw_data[size];

    for ( int i = 0; i < size; ++i )
      raw_data[i] = ( unsigned char )i;

    ram_io.write( addr, raw_data, size );

    auto info = inspector.RAM_info();

    REQUIRE( info.allocated_blocks_no == 1 );
    REQUIRE( info.allocated_addresses[0] == 0 );

    auto read = inspector.RAM_read( addr, size );

    REQUIRE_FALSE( read.empty() );
    REQUIRE( std::memcmp( read.data(), raw_data, size ) == 0 );
  }

  SECTION( "Multiple Blocks - from start to finish" )
  {
    constexpr int size = ram.block_size * 3;
    std::vector<unsigned char> raw_data( size, 0xFA );

    ram_io.write( 0x0000'0000, raw_data.data(), size );

    auto info = inspector.RAM_info();
    
    REQUIRE( info.allocated_addresses.size() == 3 );
    REQUIRE( info.allocated_addresses[0] == 0x0000'0000 );
    REQUIRE( info.allocated_addresses[1] == 0x0001'0000 );
    REQUIRE( info.allocated_addresses[2] == 0x0002'0000 );

    REQUIRE( std::memcmp( raw_data.data(), &ram[0x0000'0000], ram.block_size ) == 0 );
    REQUIRE( std::memcmp( raw_data.data() + 0x1'0000, &ram[0x0001'0000], ram.block_size ) == 0 );
    REQUIRE( std::memcmp( raw_data.data() + 0x2'0000, &ram[0x0002'0000], ram.block_size ) == 0 );

    auto block_0 = ram_io.read( 0x0000'0000, ram.block_size );
    auto block_1 = ram_io.read( 0x0001'0000, ram.block_size );
    auto block_2 = ram_io.read( 0x0002'0000, ram.block_size );
    auto block_g = ram_io.read( 0x0000'0000, size );

    REQUIRE_FALSE( block_0.empty() );
    REQUIRE_FALSE( block_1.empty() );
    REQUIRE_FALSE( block_2.empty() );
    REQUIRE_FALSE( block_g.empty() );

    REQUIRE( std::memcmp( raw_data.data(), block_0.data(), block_0.size() ) == 0 );
    REQUIRE( std::memcmp( raw_data.data(), block_1.data(), block_1.size() ) == 0 );
    REQUIRE( std::memcmp( raw_data.data(), block_2.data(), block_2.size() ) == 0 );
    REQUIRE( std::memcmp( raw_data.data(), block_g.data(), block_g.size() ) == 0 );
  }
}
