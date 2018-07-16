#include <catch.hpp>

#include <mips32/cp1.hpp>
#include <mips32/machine_inspector.hpp>

using namespace mips32;

SCENARIO( "A Coprocessor 1 object exists and it's resetted and inspected" )
{
  CP1 cp1;
  cp1.reset();

  MachineInspector inspector;

  inspector.inspect( cp1 );

  WHEN( "I read the registers on their default state" )
  {
    std::uint32_t regs[4] = {
        cp1.read( 0 ),  // FIR
        cp1.read( 31 ), // FCSR
        cp1.read( 26 ), // FEXR
        cp1.read( 28 )  // FENR
    };

    THEN( "They should be equal to those values" )
    {
      REQUIRE( regs[0] == 0x00C3'0000 );
      REQUIRE( regs[1] == 0x010C'0000 );
      REQUIRE( regs[2] == ( regs[1] & 0x0003'F07C ) );
      REQUIRE( regs[3] == ( regs[1] & 0x0000'0F87 ) );
    }
  }

  WHEN( "I write to the registers" )
  {
    std::uint32_t before[2] = {
        cp1.read( 0 ),  // FIR
        cp1.read( 31 ), // FCSR
    };

    cp1.write( 0, 0x1234'5678 ); // FIR
    cp1.write( 31, 0 );          // FCSR
    cp1.write( 26, 0 );          // FEXR
    cp1.write( 28, 0 );          // FENR

    std::uint32_t after[2] = {
        cp1.read( 0 ),  // FIR
        cp1.read( 31 ), // FCSR
    };

    THEN( "Read only fields are unchanged" )
    {
      REQUIRE( before[0] == after[0] );
      REQUIRE( ( before[1] & ~0x0103'FFFF ) == ( after[1] & ~0x0103'FFFF ) );
    }
  }

  WHEN( "I read the FPRs" )
  {
    THEN( "All of them are 0" )
    {
      auto begin = inspector.CP1_fpr_begin();
      auto end   = inspector.CP1_fpr_end();
      while ( begin != end ) {
        REQUIRE( begin.f == 0.0f );
        REQUIRE( begin.d == 0.0 );
        REQUIRE( begin.i32 == 0 );
        REQUIRE( begin.i64 == 0 );
      }
    }
  }
}