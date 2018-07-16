#include <catch.hpp>

#include <mips32/cp1.hpp>
#include <mips32/machine_inspector.hpp>

#include <cfenv>

using namespace mips32;

// TODO: change rounding mode
// TODO: provoke FPU exception
// TODO: test every instruction for every format

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
        REQUIRE( begin.read_single() == 0.0f );
        REQUIRE( begin.read_double() == 0.0 );
        REQUIRE( begin.single_binary() == 0 );
        REQUIRE( begin.double_binary() == 0 );

        ++begin;
      }
    }
  }

  WHEN( "I write a float to a register through the inspector" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end   = inspector.CP1_fpr_end();
    while ( begin != end ) {
      begin = 42.f;
      ++begin;
    }

    THEN( "If I read them again, I should see the changes" )
    {
      auto _begin = inspector.CP1_fpr_begin();
      auto _end   = inspector.CP1_fpr_end();
      while ( _begin != _end ) {
        REQUIRE( _begin.read_single() == 42.f );
        ++_begin;
      }
    }
  }

  WHEN( "I write a double to a register through the inspector" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end   = inspector.CP1_fpr_end();
    while ( begin != end ) {
      begin = 6657.0;
      ++begin;
    }

    THEN( "If I read them again, I should see the changes" )
    {
      auto _begin = inspector.CP1_fpr_begin();
      auto _end   = inspector.CP1_fpr_end();
      while ( _begin != _end ) {
        REQUIRE( _begin.read_double() == 6657.0 );
        ++_begin;
      }
    }
  }

  WHEN( "I write an i32 to a register through the inspector" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end   = inspector.CP1_fpr_end();
    while ( begin != end ) {
      begin = (std::uint32_t)0xABCD'7531;
      ++begin;
    }

    THEN( "If I read them again, I should see the changes" )
    {
      auto _begin = inspector.CP1_fpr_begin();
      auto _end   = inspector.CP1_fpr_end();
      while ( _begin != _end ) {
        REQUIRE( _begin.single_binary() == 0xABCD'7531 );
        ++_begin;
      }
    }
  }

  WHEN( "I write i64 to a register through the inspector" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end   = inspector.CP1_fpr_end();
    while ( begin != end ) {
      begin = (std::uint64_t)0x8518'FBBB'9871'412C;
      ++begin;
    }

    THEN( "If I read them again, I should see the changes" )
    {
      auto _begin = inspector.CP1_fpr_begin();
      auto _end   = inspector.CP1_fpr_end();
      while ( _begin != _end ) {
        REQUIRE( _begin.double_binary() == 0x8518'FBBB'9871'412C );
        ++_begin;
      }
    }
  }

  WHEN( "I change the rounding in the FCSR" )
  {
    /*
    FCSR is register 28
    0 RN - Round to Nearest
    1 RZ - Round Toward Zero
    2 RP - Round Towards Plus Infinity
    3 RM - Round Towards Minus Infinity
    */

    THEN( "The FP ENV should reflect that change as well" )
    {
      cp1.write( 28, 1 );
      REQUIRE( std::fegetround() == FE_TOWARDZERO );

      cp1.write( 28, 2 );
      REQUIRE( std::fegetround() == FE_UPWARD );

      cp1.write( 28, 3 );
      REQUIRE( std::fegetround() == FE_DOWNWARD );

      cp1.write( 28, 0 );
      REQUIRE( std::fegetround() == FE_TONEAREST );
    }
  }
}