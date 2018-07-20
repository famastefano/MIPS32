#include <catch.hpp>

#include <mips32/cp1.hpp>
#include <mips32/machine_inspector.hpp>

#include <array>
#include <cfenv>
#include <cmath>
#include <limits>
#include <string_view>

using namespace mips32;

// [OPCODE][FMT][R1][R2][R3][FUNCTION]
// [COP1][S|D|W|L][0-31][0-31][0-31][0-63]

constexpr std::uint32_t operator""_inst( char const *c, std::size_t ) noexcept;
constexpr std::uint32_t operator""_r1( unsigned long long n ) noexcept;
constexpr std::uint32_t operator""_r2( unsigned long long n ) noexcept;
constexpr std::uint32_t operator""_r3( unsigned long long n ) noexcept;

constexpr std::uint32_t FMT_S{0x10 << 21};
constexpr std::uint32_t FMT_D{0x11 << 21};
constexpr std::uint32_t FMT_W{0x14 << 21};
constexpr std::uint32_t FMT_L{0x15 << 21};

template <typename T>
struct QNAN;

template <typename T>
struct SNAN;

template <typename T>
struct DENORM;

template <typename T>
struct INF;

template <>
struct QNAN<float>
{
  constexpr operator float() noexcept { return std::numeric_limits<float>::quiet_NaN(); }
};

template <>
struct QNAN<double>
{
  constexpr operator double() noexcept { return std::numeric_limits<double>::quiet_NaN(); }
};

template <>
struct SNAN<float>
{
  constexpr operator float() noexcept { return std::numeric_limits<float>::signaling_NaN(); }
};

template <>
struct SNAN<double>
{
  constexpr operator double() noexcept { return std::numeric_limits<double>::signaling_NaN(); }
};

template <>
struct DENORM<float>
{
  constexpr operator float() noexcept { return std::numeric_limits<float>::denorm_min(); }
};

template <>
struct DENORM<double>
{
  constexpr operator double() noexcept { return std::numeric_limits<double>::denorm_min(); }
};

template <>
struct INF<float>
{
  constexpr operator float() noexcept { return std::numeric_limits<float>::infinity(); }
};

template <>
struct INF<double>
{
  constexpr operator double() noexcept { return std::numeric_limits<double>::infinity(); }
};

// TODO: provoke FPU exception
// TODO: test instruction related to the W and L format

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
      REQUIRE( regs[0] == 0x00F3'0000 );
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

  /* * * * * * * * *
   *               *
   *  INSTRUCTIONS *
   *               *
   * * * * * * * * */

  // In alphabetically order, because that's how the manual order them

  WHEN( "ABS.S $f0, $f2 and ABS.D $f14, $f8 is executed" )
  {
    auto abs_s = "ABS"_inst | FMT_S | 0_r1 | 2_r2;
    auto abs_d = "ABS"_inst | FMT_D | 14_r1 | 8_r2;

    inspector.CP1_fpr( 2 ) = -1.f;   // $f2 = -1.f
    inspector.CP1_fpr( 8 ) = -897.0; // $f8 == -897.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( abs_s );
      auto rd = cp1.execute( abs_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 0 ).read_single() == 1.f );
      REQUIRE( inspector.CP1_fpr( 14 ).read_double() == 897.0 );
    }
  }

  WHEN( "ADD.S $f31, $f7, $f21 and ADD.D $f18, $f17, $f16 is executed" )
  {
    auto add_s = "ADD"_inst | FMT_S | 31_r1 | 7_r2 | 21_r3;
    auto add_d = "ADD"_inst | FMT_D | 18_r1 | 17_r2 | 16_r3;

    inspector.CP1_fpr( 7 )  = 38.f;
    inspector.CP1_fpr( 21 ) = 1285.f;

    inspector.CP1_fpr( 17 ) = 429.0;
    inspector.CP1_fpr( 16 ) = -2943.0;

    auto res_s = 38.0f + 1285.0f;
    auto res_d = 429.0 + ( -2943.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( add_s );
      auto rd = cp1.execute( add_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 31 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 18 ).read_double() == res_d );
    }
  }

  // TODO: cmp.condn.fmt

  /*
  The mask has 10 bits as follows.
  Bits 0 and 1 indicate NaN values:
  - signaling NaN (bit 0) 
  - and quiet NaN (bit 1).

  Bits 2, 3, 4, 5 classify negative values:
  - infinity (bit 2),
  - normal (bit 3),
  - subnormal (bit 4),
  - and zero (bit 5).

  Bits 6, 7, 8, 9 classify positive values:
  - infinity (bit 6),
  - normal (bit 7),
  - subnormal (bit 8),
  - and zero (bit 9).
  */
  WHEN( "CLASS.S $f0, $f2 and CLASS.D $f31, $f31 are executed" )
  {
    auto class_s = "CLASS"_inst | FMT_S | 0_r1 | 2_r2;
    auto class_d = "CLASS"_inst | FMT_D | 31_r1 | 31_r2;

    // Quiet NaN
    THEN( "Quiet NaN should be classified correctly" )
    {
      inspector.CP1_fpr( 2 )  = QNAN<float>{};
      inspector.CP1_fpr( 31 ) = QNAN<double>{};

      auto res_s = std::uint32_t( 1 << 1 );
      auto res_d = std::uint64_t( 1 << 1 );

      auto rs = cp1.execute( class_s );
      auto rd = cp1.execute( class_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 31 ).double_binary() == res_d );
    }

    // [NEGATIVE] Infinity + Normal + Denormal + Zero
    AND_THEN( "[NEGATIVE] Infinity + Normal + Denormal + Zero" )
    {
      cp1.write( 31, 0x000C'0000 ); // remove the flushing of denormalized numbers to zero

      float f_value_to_test[] = {
          -INF<float>{},
          -152.0f,
          -DENORM<float>{},
          -0.0f,
      };

      double d_value_to_test[] = {
          -INF<double>{},
          -62'342.0,
          -DENORM<double>{},
          -0.0,
      };

      std::uint32_t res_s[] = {
          1 << 2, // inf
          1 << 3, // normal
          1 << 4, // den
          1 << 5, // zero
      };

      std::uint64_t res_d[] = {
          1 << 2, // inf
          1 << 3, // normal
          1 << 4, // den
          1 << 5, // zero
      };

      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 2 )  = f_value_to_test[i];
        inspector.CP1_fpr( 31 ) = d_value_to_test[i];

        auto rs = cp1.execute( class_s );
        auto rd = cp1.execute( class_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == res_s[i] );
        REQUIRE( inspector.CP1_fpr( 31 ).double_binary() == res_d[i] );
      }
    }

    // [POSITIVE] Infinity + Normal + Denormal + Zero
    AND_THEN( "[POSITIVE] Infinity + Normal + Denormal + Zero" )
    {
      cp1.write( 31, 0x000C'0000 ); // remove the flushing of denormalized numbers to zero

      float f_value_to_test[] = {
          INF<float>{},
          14'000.0f,
          DENORM<float>{},
          0.0f,
      };

      double d_value_to_test[] = {
          INF<double>{},
          1.0,
          DENORM<double>{},
          0.0,
      };

      std::uint32_t res_s[] = {
          1 << 6, // inf
          1 << 7, // normal
          1 << 8, // den
          1 << 9, // zero
      };

      std::uint64_t res_d[] = {
          1 << 6, // inf
          1 << 7, // normal
          1 << 8, // den
          1 << 9, // zero
      };

      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 2 )  = f_value_to_test[i];
        inspector.CP1_fpr( 31 ) = d_value_to_test[i];

        auto rs = cp1.execute( class_s );
        auto rd = cp1.execute( class_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == res_s[i] );
        REQUIRE( inspector.CP1_fpr( 31 ).double_binary() == res_d[i] );
      }
    }
  }

  WHEN( "CVT.D.S $f19, $f15 is executed" )
  {
    auto cvt_d = "CVT_D"_inst | FMT_S | 19_r1 | 15_r2;

    inspector.CP1_fpr( 15 ) = 1509.f;

    auto res = 1509.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( cvt_d );

      REQUIRE( rs == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 19 ).read_double() == res );
    }
  }

  WHEN( "CVT.S.D $f3, $f9 is executed" )
  {
    auto cvt_s = "CVT_S"_inst | FMT_D | 3_r1 | 9_r2;

    inspector.CP1_fpr( 9 ) = 8374.0;

    auto res = 8374.0f;

    THEN( "The result must be correct" )
    {
      auto rd = cp1.execute( cvt_s );

      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 3 ).read_single() == res );
    }
  }

  WHEN( "DIV.S $f1, $f2, $f3 and DIV.D $f4, $f5, $f6 are executed" )
  {
    auto div_s = "DIV"_inst | FMT_S | 1_r1 | 2_r2 | 3_r3;
    auto div_d = "DIV"_inst | FMT_D | 4_r1 | 5_r2 | 6_r3;

    inspector.CP1_fpr( 2 ) = 121.0f;
    inspector.CP1_fpr( 3 ) = 11.0f;

    inspector.CP1_fpr( 5 ) = 240.0;
    inspector.CP1_fpr( 6 ) = 2.0;

    auto res_s = 121.0f / 11.0f;
    auto res_d = 240.0f / 2.0f;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( div_s );
      auto rd = cp1.execute( div_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 1 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 4 ).read_double() == res_d );
    }
  }

  WHEN( "MADDF.S $f10, $f11, $f12 and MADDF.D $f20, $f20, $f20 are executed" )
  {
    auto maddf_s = "MADDF"_inst | FMT_S | 10_r1 | 11_r2 | 12_r3;
    auto maddf_d = "MADDF"_inst | FMT_D | 20_r1 | 20_r2 | 20_r3;

    inspector.CP1_fpr( 10 ) = 31.0f;
    inspector.CP1_fpr( 11 ) = 89.0f;
    inspector.CP1_fpr( 12 ) = 61000.0f;

    inspector.CP1_fpr( 20 ) = 14912.0;

    auto res_s = std::fma( 89.0f, 61000.0f, 31.0f );
    auto res_d = std::fma( 14912.0, 14912.0, 14912.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( maddf_s );
      auto rd = cp1.execute( maddf_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 10 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 20 ).read_double() == res_d );
    }
  }

  WHEN( "MSUBF.S $f9, $f21, $f13 and MSUBF.D $f1, $f1, $f1 are executed" )
  {
    auto msubf_s = "MSUBF"_inst | FMT_S | 9_r1 | 21_r2 | 13_r3;
    auto msubf_d = "MSUBF"_inst | FMT_D | 1_r1 | 1_r2 | 1_r3;

    inspector.CP1_fpr( 9 )  = 9.0f;
    inspector.CP1_fpr( 21 ) = 40'000.0f;
    inspector.CP1_fpr( 13 ) = 2.0f;

    inspector.CP1_fpr( 1 ) = 7202.0;

    auto res_s = 9.0f - ( 40'000.0f / 2.0f );
    auto res_d = 7202.0 - ( 7202.0 / 7202.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( msubf_s );
      auto rd = cp1.execute( msubf_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 9 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 1 ).read_double() == res_d );
    }
  }

  WHEN( "MAX.S $f5, $f8, $f0 and MAX.D $f0, $f11, $f11 are executed" )
  {
    auto max_s = "MAX"_inst | FMT_S | 5_r1 | 8_r2 | 0_r3;
    auto max_d = "MAX"_inst | FMT_D | 0_r1 | 11_r2 | 11_r3;

    inspector.CP1_fpr( 8 ) = 60.0f;
    inspector.CP1_fpr( 0 ) = 59.0f;

    inspector.CP1_fpr( 11 ) = 239'457.0;

    auto res_s = 60.0f;
    auto res_d = 239'457.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( max_s );
      auto rd = cp1.execute( max_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 5 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 0 ).read_double() == res_d );
    }
  }

  WHEN( "MIN.S $f18, $f30, $f27 and MIN.D $f4, $f25, $f22 are executed" )
  {
    auto max_s = "MIN"_inst | FMT_S | 18_r1 | 30_r2 | 27_r3;
    auto max_d = "MIN"_inst | FMT_D | 4_r1 | 25_r2 | 22_r3;

    inspector.CP1_fpr( 30 ) = 8345.0f;
    inspector.CP1_fpr( 27 ) = 34'897.0f;

    inspector.CP1_fpr( 25 ) = -98'345.0;
    inspector.CP1_fpr( 22 ) = 0.0;

    auto res_s = 8345.0f;
    auto res_d = -98'345.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( max_s );
      auto rd = cp1.execute( max_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 18 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 4 ).read_double() == res_d );
    }
  }

  WHEN( "MAXA.S $f26, $f12, $f29 and MAXA.D $f0, $f1, $f6 are executed" )
  {
    auto maxa_s = "MAXA"_inst | FMT_S | 26_r1 | 12_r2 | 29_r3;
    auto maxa_d = "MAXA"_inst | FMT_D | 0_r1 | 1_r2 | 6_r3;

    inspector.CP1_fpr( 12 ) = -3984.0f;
    inspector.CP1_fpr( 29 ) = -6230.0f;

    inspector.CP1_fpr( 1 ) = 923.0;
    inspector.CP1_fpr( 6 ) = -18'000.0;

    auto res_s = 6230.0f;
    auto res_d = 18'000.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( maxa_s );
      auto rd = cp1.execute( maxa_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 26 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 0 ).read_double() == res_d );
    }
  }

  WHEN( "MINA.S $f31, $f30, $f29 and MINA.D $f2, $f4, $f8 are executed" )
  {
    auto mina_s = "MINA"_inst | FMT_S | 31_r1 | 30_r2 | 29_r3;
    auto mina_d = "MINA"_inst | FMT_D | 2_r1 | 4_r2 | 8_r3;

    inspector.CP1_fpr( 30 ) = -245.0f;
    inspector.CP1_fpr( 29 ) = -988.0f;

    inspector.CP1_fpr( 4 ) = 586.0;
    inspector.CP1_fpr( 8 ) = -6000.0;

    auto res_s = 245.0f;
    auto res_d = 586.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( mina_s );
      auto rd = cp1.execute( mina_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 31 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 2 ).read_double() == res_d );
    }
  }

  WHEN( "MOV.S $f10, $f18 and MOV.D $f21, $f3 are executed" )
  {
    auto mov_s = "MOV"_inst | FMT_S | 10_r1 | 18_r2;
    auto mov_d = "MOV"_inst | FMT_D | 21_r1 | 3_r2;

    inspector.CP1_fpr( 18 ) = 681.0f;

    inspector.CP1_fpr( 3 ) = -50'000.0;

    auto res_s = 681.0f;
    auto res_d = -50'000.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( mov_s );
      auto rd = cp1.execute( mov_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 10 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 21 ).read_double() == res_d );
    }
  }

  WHEN( "MUL.S $f15, $f9, $f2 and MUL.D $f13, $f0, $f7 are executed" )
  {
    auto mul_s = "MUL"_inst | FMT_S | 15_r1 | 9_r2 | 2_r3;
    auto mul_d = "MUL"_inst | FMT_D | 13_r1 | 0_r2 | 7_r3;

    inspector.CP1_fpr( 9 ) = 2.0f;
    inspector.CP1_fpr( 2 ) = -1024.0f;

    inspector.CP1_fpr( 0 ) = 88.0;
    inspector.CP1_fpr( 7 ) = 621.0;

    auto res_s = 2.0f * -1024.0f;
    auto res_d = 88.0 * 621.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( mul_s );
      auto rd = cp1.execute( mul_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 15 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 13 ).read_double() == res_d );
    }
  }

  WHEN( "NEG.S $f5, $f6 and NEG.D $f7, $f8 are executed" )
  {
    auto neg_s = "NEG"_inst | FMT_S | 5_r1 | 6_r2;
    auto neg_d = "NEG"_inst | FMT_D | 7_r1 | 8_r2;

    inspector.CP1_fpr( 6 ) = 1234.0f;

    inspector.CP1_fpr( 8 ) = 0.0;

    auto res_s = -1234.0f;
    auto res_d = -0.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( neg_s );
      auto rd = cp1.execute( neg_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 5 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 7 ).read_double() == res_d );
    }
  }

  WHEN( "RECIP.S $f9, $f8 and RECIP.D $f31, $f0 are executed" )
  {
    auto recip_s = "RECIP"_inst | FMT_S | 9_r1 | 8_r2;
    auto recip_d = "RECIP"_inst | FMT_D | 31_r1 | 0_r2;

    inspector.CP1_fpr( 8 ) = 67.0f;

    inspector.CP1_fpr( 0 ) = -851.0;

    auto res_s = 1.0f / 67.0f;
    auto res_d = 1.0 / -851.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( recip_s );
      auto rd = cp1.execute( recip_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 9 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 31 ).read_double() == res_d );
    }
  }

  WHEN( "RINT.S $f1, $f18 and RINT.D $f4, $f26 are executed" )
  {
    auto rint_s = "RINT"_inst | FMT_S | 1_r1 | 18_r2;
    auto rint_d = "RINT"_inst | FMT_D | 4_r1 | 26_r2;

    inspector.CP1_fpr( 18 ) = 29'842.0f;

    inspector.CP1_fpr( 26 ) = -87'431.0;

    auto res_s = std::uint32_t( std::llrint( 29'842.0f ) );
    auto res_d = std::uint64_t( std::llrint( -87'431.0 ) );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( rint_s );
      auto rd = cp1.execute( rint_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 1 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 4 ).double_binary() == res_d );
    }
  }

  WHEN( "RSQRT.S $f17, $f21 and RSQRT.D $f24, $f30 are executed" )
  {
    auto rsqrt_s = "RSQRT"_inst | FMT_S | 17_r1 | 21_r2;
    auto rsqrt_d = "RSQRT"_inst | FMT_D | 24_r1 | 30_r2;

    inspector.CP1_fpr( 21 ) = 25.0f;

    inspector.CP1_fpr( 30 ) = 256.0;

    auto res_s = 1.0f / std::sqrt( 25.0f );
    auto res_d = 1.0 / std::sqrt( 256.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( rsqrt_s );
      auto rd = cp1.execute( rsqrt_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 17 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 24 ).read_double() == res_d );
    }
  }

  WHEN( "SEL.S $f8, $f11, $f12 and SEL.D $f0, $f1, $f3 are executed" )
  {
    auto sel_s = "SEL"_inst | FMT_S | 8_r1 | 11_r2 | 12_r3;
    auto sel_d = "SEL"_inst | FMT_D | 0_r1 | 1_r2 | 3_r3;

    inspector.CP1_fpr( 8 )  = std::uint32_t( 0 );
    inspector.CP1_fpr( 11 ) = 48.0f;
    inspector.CP1_fpr( 12 ) = 18'000.0f;

    inspector.CP1_fpr( 0 ) = std::uint64_t( 1 );
    inspector.CP1_fpr( 1 ) = -8888.0;
    inspector.CP1_fpr( 3 ) = -141.0;

    auto res_s = 48.0f;
    auto res_d = -141.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( sel_s );
      auto rd = cp1.execute( sel_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 8 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 0 ).read_double() == res_d );
    }
  }

  WHEN( "SELEQZ.S $f9, $f8, $f7 and SELEQZ.D $f27, $f26, $f25 are executed" )
  {
    auto seleqz_s = "SELEQZ"_inst | FMT_S | 9_r1 | 8_r2 | 7_r3;
    auto seleqz_d = "SELEQZ"_inst | FMT_D | 27_r1 | 26_r2 | 25_r3;

    inspector.CP1_fpr( 9 ) = 92'837.0f;
    inspector.CP1_fpr( 8 ) = 564.0f;
    inspector.CP1_fpr( 7 ) = std::uint32_t( 1 );

    inspector.CP1_fpr( 27 ) = 39'847.0;
    inspector.CP1_fpr( 26 ) = 987.0;
    inspector.CP1_fpr( 25 ) = std::uint64_t( 0 );

    auto res_s = 0.0f;
    auto res_d = 987.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( seleqz_s );
      auto rd = cp1.execute( seleqz_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 9 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 27 ).read_double() == res_d );
    }
  }

  WHEN( "SELNEZ.S $f22, $f3, $f4 and SELNEZ.D $f18, $f22, $f14 are executed" )
  {
    auto selnez_s = "SELNEZ"_inst | FMT_S | 22_r1 | 3_r2 | 4_r3;
    auto selnez_d = "SELNEZ"_inst | FMT_D | 18_r1 | 22_r2 | 14_r3;

    inspector.CP1_fpr( 3 ) = 17.0f;
    inspector.CP1_fpr( 4 ) = std::uint32_t( 1 );

    inspector.CP1_fpr( 22 ) = -41'000.0;
    inspector.CP1_fpr( 14 ) = std::uint32_t( 0 );

    auto res_s = 17.0f;
    auto res_d = 0.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( selnez_s );
      auto rd = cp1.execute( selnez_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 22 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 18 ).read_double() == res_d );
    }
  }

  WHEN( "SQRT.S $f13, $f7 and SQRT.D $f14, $f8 are executed" )
  {
    auto sqrt_s = "SQRT"_inst | FMT_S | 13_r1 | 7_r2;
    auto sqrt_d = "SQRT"_inst | FMT_D | 14_r1 | 8_r2;

    inspector.CP1_fpr( 7 ) = 1024.0f;

    inspector.CP1_fpr( 8 ) = 16'000.0;

    auto res_s = std::sqrt( 1024.0f );
    auto res_d = std::sqrt( 16'000.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( sqrt_s );
      auto rd = cp1.execute( sqrt_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 13 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 14 ).read_double() == res_d );
    }
  }

  WHEN( "SUB.S $f10, $f9, $f10 and SUB.D $f1, $f1, $f1 are executed" )
  {
    auto sub_s = "SUB"_inst | FMT_S | 10_r1 | 9_r2 | 10_r3;
    auto sub_d = "SUB"_inst | FMT_D | 1_r1 | 1_r2 | 1_r3;

    inspector.CP1_fpr( 9 )  = 15.0f;
    inspector.CP1_fpr( 10 ) = -61'000.0f;

    inspector.CP1_fpr( 1 ) = -1985.0;

    auto res_s = 15.0f - ( -61'000.0f );
    auto res_d = -1985.0 - ( -1985.0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( sub_s );
      auto rd = cp1.execute( sub_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 10 ).read_single() == res_s );
      REQUIRE( inspector.CP1_fpr( 1 ).read_double() == res_d );
    }
  }
}

constexpr std::uint32_t operator""_r1( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 6 );
}
constexpr std::uint32_t operator""_r2( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 11 );
}
constexpr std::uint32_t operator""_r3( unsigned long long n ) noexcept
{
  return std::uint32_t( n << 16 );
}

constexpr std::uint32_t operator""_inst( char const *c, std::size_t ) noexcept
{
  using namespace std::literals;

  constexpr std::uint32_t opcode{0b010001 << 26};

  std::string_view view( c );

  constexpr std::array<std::string_view, 64> encode_table{
      "ADD"sv,
      "SUB"sv,
      "MUL"sv,
      "DIV"sv,
      "SQRT"sv,
      "ABS"sv,
      "MOV"sv,
      "NEG"sv,
      "ROUND_L"sv,
      "TRUNC_L"sv,
      "CEIL_L"sv,
      "FLOOR_L"sv,
      "ROUND_W"sv,
      "TRUNC_W"sv,
      "CEIL_W"sv,
      "FLOOR_W"sv,
      "SEL"sv,
      "MOVCF"sv,
      "MOVZ"sv,
      "MOVN"sv,
      "SELEQZ"sv,
      "RECIP"sv,
      "RSQRT"sv,
      "SELNEZ"sv,
      "MADDF"sv,
      "MSUBF"sv,
      "RINT"sv,
      "CLASS"sv,
      "MIN"sv,
      "MAX"sv,
      "MINA"sv,
      "MAXA"sv,
      "CVT_S"sv,
      "CVT_D"sv,
      "_"sv,  // reserved
      "__"sv, // reserved
      "CVT_L"sv,
      "CVT_W"sv,
      "CVT_PS"sv,
      "___"sv, // reserved
      "CABS_AF"sv,
      "CABS_UN"sv,
      "CABS_EQ"sv,
      "CABS_UEQ"sv,
      "CABS_LT"sv,
      "CABS_ULT"sv,
      "CABS_LE"sv,
      "CABS_ULE"sv,
      "CABS_SAF"sv,
      "CABS_SUN"sv,
      "CABS_SEQ"sv,
      "CABS_SUEQ"sv,
      "CABS_SLT"sv,
      "CABS_SULT"sv,
      "CABS_SLE"sv,
      "CABS_SULE"sv,
  };

  std::uint32_t code{0};
  for ( auto const &in : encode_table ) {
    if ( in == view ) return code | opcode;
    ++code;
  }

  return std::uint32_t( -1 );
}
