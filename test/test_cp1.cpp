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

constexpr std::uint32_t CMP_FMT_S{0b10100 << 21};
constexpr std::uint32_t CMP_FMT_D{0b10101 << 21};

constexpr std::uint64_t CMP_FALSE( 0 );
constexpr std::uint64_t CMP_TRUE( 0xFFFF'FFFF'FFFF'FFFF );

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

  WHEN( "CMP.AF.S $f0, $f0, $f0 and CMP.AF.D $f4, $f5, $f4 are executed" )
  {
    auto cmp_af_s = "CMP_AF"_inst | CMP_FMT_S | 0_r1 | 0_r2 | 0_r3;
    auto cmp_af_d = "CMP_AF"_inst | CMP_FMT_D | 4_r1 | 5_r2 | 4_r3;

    inspector.CP1_fpr( 0 ) = 256.0f;
    inspector.CP1_fpr( 4 ) = 394.0;

    auto res_s = CMP_FALSE;
    auto res_d = CMP_FALSE;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( cmp_af_s );
      auto rd = cp1.execute( cmp_af_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 4 ).double_binary() == res_d );
    }
  }

  WHEN( "CMP.UN.S $f3, $f4, $f3 and CMP.AF.D $f18, $f11, $f0 are executed" )
  {
    auto cmp_un_s = "CMP_UN"_inst | CMP_FMT_S | 3_r1 | 4_r2 | 3_r3;
    auto cmp_un_d = "CMP_UN"_inst | CMP_FMT_D | 18_r1 | 11_r2 | 0_r3;

    constexpr float f_value_to_check[][2] = {
        {0.0f, -0.0f},
        {QNAN<float>{}, 298'375.0f},
        {-0.389f, QNAN<float>{}},
        {349.0f, -4959.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {0.0, -0.0},
        {QNAN<double>{}, 3409.0},
        {-0.0005, QNAN<double>{}},
        {7.0, -0.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_FALSE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_FALSE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 4 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 3 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 11 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 0 )  = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_un_s );
        auto rd = cp1.execute( cmp_un_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 3 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 18 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.EQ.S $f30, $f29, $f30 and CMP.EQ.D $f1, $f2, $f3 are executed" )
  {
    auto cmp_eq_s = "CMP_EQ"_inst | CMP_FMT_S | 30_r1 | 29_r2 | 30_r3;
    auto cmp_eq_d = "CMP_EQ"_inst | CMP_FMT_D | 1_r1 | 2_r2 | 3_r3;

    constexpr float f_value_to_check[][2] = {
        {1.0f, -0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {6'797'895.0f, 6'797'895.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {QNAN<double>{}, QNAN<double>{}},
        {QNAN<double>{}, 23.0},
        {-312.9999999, QNAN<double>{}},
        {+0.0, -0.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_FALSE,
        CMP_FALSE,
        CMP_FALSE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 29 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 30 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 2 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 3 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_eq_s );
        auto rd = cp1.execute( cmp_eq_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 30 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 1 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.UEQ.S $f10, $f9, $f27 and CMP.UEQ.D $f4, $f6, $f26 are executed" )
  {
    auto cmp_ueq_s = "CMP_UEQ"_inst | CMP_FMT_S | 10_r1 | 9_r2 | 27_r3;
    auto cmp_ueq_d = "CMP_UEQ"_inst | CMP_FMT_D | 4_r1 | 6_r2 | 26_r3;

    constexpr float f_value_to_check[][2] = {
        {0.0f, -0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {6'797'895.0f, 6'797'895.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {-0.0, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {QNAN<double>{}, 23.0},
        {-312.9999999, QNAN<double>{}},
    };

    constexpr std::uint64_t res[] = {
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 9 )  = f_value_to_check[i][0];
        inspector.CP1_fpr( 27 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 6 )  = d_value_to_check[i][0];
        inspector.CP1_fpr( 26 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_ueq_s );
        auto rd = cp1.execute( cmp_ueq_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 10 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 4 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.LT.S $f5, $f10, $f15 and CMP.LT.D $f6, $f12, $18 are executed" )
  {
    auto cmp_lt_s = "CMP_LT"_inst | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_lt_d = "CMP_LT"_inst | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    constexpr float f_value_to_check[][2] = {
        {-0.0f, 0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {-6'797'895.0f, 41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {-0.0, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {QNAN<double>{}, 23.0},
        {-312.9999999, 18.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_FALSE,
        CMP_FALSE,
        CMP_FALSE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 10 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 15 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 12 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 18 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_lt_s );
        auto rd = cp1.execute( cmp_lt_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 5 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 6 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.ULT.S $f5, $f10, $f15 and CMP.ULT.D $f6, $f12, $18 are executed" )
  {
    auto cmp_ult_s = "CMP_ULT"_inst | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_ult_d = "CMP_ULT"_inst | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    constexpr float f_value_to_check[][2] = {
        {-0.0f, 0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {-6'797'895.0f, 41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {-0.0, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {QNAN<double>{}, 23.0},
        {-312.9999999, 18.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_FALSE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 10 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 15 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 12 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 18 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_ult_s );
        auto rd = cp1.execute( cmp_ult_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 5 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 6 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.LE.S $f5, $f10, $f15 and CMP.LE.D $f6, $f12, $18 are executed" )
  {
    auto cmp_le_s = "CMP_LE"_inst | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_le_d = "CMP_LE"_inst | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    constexpr float f_value_to_check[][2] = {
        {-0.0f, 0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, -14.0f},
        {-6'797'895.0f, 41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {-0.0, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {23.0, 23.0},
        {-312.9999999, 18.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_TRUE,
        CMP_FALSE,
        CMP_TRUE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 10 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 15 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 12 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 18 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_le_s );
        auto rd = cp1.execute( cmp_le_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 5 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 6 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.ULE.S $f5, $f10, $f15 and CMP.ULE.D $f6, $f12, $18 are executed" )
  {
    auto cmp_ule_s = "CMP_ULE"_inst | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_ule_d = "CMP_ULE"_inst | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    constexpr float f_value_to_check[][2] = {
        {-0.0f, 0.0f},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {+6'797'895.0f, -41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {-0.0, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {23.0, 23.0},
        {+312.9999999, -18.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_FALSE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 10 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 15 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 12 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 18 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_ule_s );
        auto rd = cp1.execute( cmp_ule_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 5 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 6 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.OR.S $f0, $f3, $f6 and CMP.OR.D $f13, $f14, $f13 are executed" )
  {
    auto cmp_or_s = "CMP_OR"_inst | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_or_d = "CMP_OR"_inst | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    constexpr float f_value_to_check[][2] = {
        {QNAN<float>{}, QNAN<float>{}},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {+6'797'895.0f, -41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {QNAN<double>{}, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {23.0, QNAN<double>{}},
        {+312.9999999, -18.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_FALSE,
        CMP_FALSE,
        CMP_FALSE,
        CMP_TRUE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 3 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 6 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 14 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 13 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_or_s );
        auto rd = cp1.execute( cmp_or_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 13 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.UNE.S $f31, $f30, $f29 and CMP.UNE.D $f18, $f21, $f2 are executed" )
  {
    auto cmp_une_s = "CMP_UNE"_inst | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_une_d = "CMP_UNE"_inst | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    constexpr float f_value_to_check[][2] = {
        {QNAN<float>{}, QNAN<float>{}},
        {QNAN<float>{}, 9.0f},
        {-14.0f, QNAN<float>{}},
        {-41'000.0f, -41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {QNAN<double>{}, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {23.0, QNAN<double>{}},
        {312.0, 312.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_FALSE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 3 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 6 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 14 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 13 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_une_s );
        auto rd = cp1.execute( cmp_une_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 13 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CMP.NE.S $f6, $f12, $f0 and CMP.NE.D $f3, $f5, $f19 are executed" )
  {
    auto cmp_ne_s = "CMP_NE"_inst | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_ne_d = "CMP_NE"_inst | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    constexpr float f_value_to_check[][2] = {
        {QNAN<float>{}, QNAN<float>{}},
        {QNAN<float>{}, 9.0f},
        {+14.0f, -14.0f},
        {-41'000.0f, -41'000.0f},
    };

    constexpr double d_value_to_check[][2] = {
        {QNAN<double>{}, 0.0},
        {QNAN<double>{}, QNAN<double>{}},
        {+23.0, -23.0},
        {2397.0, 2397.0},
    };

    constexpr std::uint64_t res[] = {
        CMP_TRUE,
        CMP_TRUE,
        CMP_TRUE,
        CMP_FALSE,
    };

    THEN( "The result must be correct" )
    {
      for ( int i = 0; i < 4; ++i ) {
        inspector.CP1_fpr( 3 ) = f_value_to_check[i][0];
        inspector.CP1_fpr( 6 ) = f_value_to_check[i][1];

        inspector.CP1_fpr( 14 ) = d_value_to_check[i][0];
        inspector.CP1_fpr( 13 ) = d_value_to_check[i][1];

        auto rs = cp1.execute( cmp_ne_s );
        auto rd = cp1.execute( cmp_ne_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == std::uint32_t( res[i] ) );
        REQUIRE( inspector.CP1_fpr( 13 ).double_binary() == res[i] );
      }
    }
  }

  WHEN( "CEIL.L.S $f0, $f0 and CEIL.L.D $f13, $f17 are executed" )
  {
    auto ceil_l_s = "CEIL_L"_inst | FMT_S | 0_r1 | 0_r2;
    auto ceil_l_d = "CEIL_L"_inst | FMT_D | 13_r1 | 17_r2;

    inspector.CP1_fpr( 0 )  = 3947.6241f;
    inspector.CP1_fpr( 17 ) = -20.39;

    auto res_s = std::uint64_t( 3948 );
    auto res_d = std::uint64_t( -20 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( ceil_l_s );
      auto rd = cp1.execute( ceil_l_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 0 ).double_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 13 ).double_binary() == res_d );
    }
  }

  WHEN( "CEIL.W.S $f3, $f21 and CEIL.W.D $f8, $f19 are executed" )
  {
    auto ceil_w_s = "CEIL_W"_inst | FMT_S | 3_r1 | 21_r2;
    auto ceil_w_d = "CEIL_W"_inst | FMT_D | 8_r1 | 19_r2;

    inspector.CP1_fpr( 21 ) = -54'876.3487f;
    inspector.CP1_fpr( 19 ) = 98'723.93;

    auto res_s = std::uint32_t( -54'876 );
    auto res_d = std::uint32_t( 98'724 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( ceil_w_s );
      auto rd = cp1.execute( ceil_w_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 3 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 8 ).single_binary() == res_d );
    }
  }

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

  WHEN( "CVT.D.S $f19, $f15 and CVT.D.W $f0, $f1 and CVT.D.L $f31, $f31 are executed" )
  {
    auto cvt_d_s = "CVT_D"_inst | FMT_S | 19_r1 | 15_r2;
    auto cvt_d_w = "CVT_D"_inst | FMT_W | 0_r1 | 1_r2;
    auto cvt_d_l = "CVT_D"_inst | FMT_L | 31_r1 | 31_r2;

    inspector.CP1_fpr( 15 ) = 1509.f;
    inspector.CP1_fpr( 1 )  = std::uint32_t( -2874 );
    inspector.CP1_fpr( 31 ) = std::uint64_t( 34'903 );

    auto res_s = 1509.0;
    auto res_w = -2874.0;
    auto res_l = 34'903.0;

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( cvt_d_s );
      auto rw = cp1.execute( cvt_d_w );
      auto rl = cp1.execute( cvt_d_l );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rw == CP1::Exception::NONE );
      REQUIRE( rl == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 19 ).read_double() == res_s );
      REQUIRE( inspector.CP1_fpr( 0 ).read_double() == res_w );
      REQUIRE( inspector.CP1_fpr( 31 ).read_double() == res_l );
    }
  }

  WHEN( "CVT.L.S $f8, $f27 and CVT.L.D $f30, $f24 are executed" )
  {
    auto cvt_l_s = "CVT_L"_inst | FMT_S | 8_r1 | 27_r2;
    auto cvt_l_d = "CVT_L"_inst | FMT_D | 30_r1 | 24_r2;

    inspector.CP1_fpr( 27 ) = -3094.0f;
    inspector.CP1_fpr( 24 ) = 9074.0;

    auto res_s = std::uint64_t( -3094 );
    auto res_d = std::uint64_t( 9074 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( cvt_l_s );
      auto rd = cp1.execute( cvt_l_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 8 ).double_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 30 ).double_binary() == res_d );
    }
  }

  WHEN( "CVT.S.D $f3, $f9 and CVT.S.W $f11, $f0 and CVT.S.L $f4, $f22 are executed" )
  {
    auto cvt_s_d = "CVT_S"_inst | FMT_D | 3_r1 | 9_r2;
    auto cvt_s_w = "CVT_S"_inst | FMT_W | 11_r1 | 0_r2;
    auto cvt_s_l = "CVT_S"_inst | FMT_L | 4_r1 | 22_r2;

    inspector.CP1_fpr( 9 )  = 8374.0;
    inspector.CP1_fpr( 0 )  = std::uint32_t( 2166 );
    inspector.CP1_fpr( 22 ) = std::uint64_t( -348'763'328 );

    auto res_d = 8374.0f;
    auto res_w = 2166;
    auto res_l = -348'763'328;

    THEN( "The result must be correct" )
    {
      auto rd = cp1.execute( cvt_s_d );
      auto rw = cp1.execute( cvt_s_w );
      auto rl = cp1.execute( cvt_s_l );

      REQUIRE( rd == CP1::Exception::NONE );
      REQUIRE( rw == CP1::Exception::NONE );
      REQUIRE( rl == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 3 ).read_single() == res_d );
      REQUIRE( inspector.CP1_fpr( 11 ).read_single() == res_w );
      REQUIRE( inspector.CP1_fpr( 4 ).read_single() == res_l );
    }
  }

  WHEN( "CVT.W.S $f13, $f31 and CVT.W.D $f21, $f12 are executed" )
  {
    auto cvt_w_s = "CVT_W"_inst | FMT_S | 13_r1 | 31_r2;
    auto cvt_w_d = "CVT_W"_inst | FMT_D | 21_r1 | 12_r2;

    inspector.CP1_fpr( 31 ) = 23'984.0f;
    inspector.CP1_fpr( 12 ) = -4309.0;

    auto res_s = std::uint32_t( 23'984 );
    auto res_d = std::uint32_t( -4309 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( cvt_w_s );
      auto rd = cp1.execute( cvt_w_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 13 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 21 ).single_binary() == res_d );
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

  WHEN( "FLOOR.L.S $f3, $f3 and FLOOR.L.D $f2, $f7 are executed" )
  {
    auto floor_l_s = "FLOOR_L"_inst | FMT_S | 3_r1 | 3_r2;
    auto floor_l_d = "FLOOR_L"_inst | FMT_D | 2_r1 | 7_r2;

    inspector.CP1_fpr( 3 ) = 23'412.8f;
    inspector.CP1_fpr( 7 ) = -2038.309;

    auto res_s = std::uint64_t( 23'412 );
    auto res_d = std::uint64_t( -2039 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( floor_l_s );
      auto rd = cp1.execute( floor_l_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 3 ).double_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 2 ).double_binary() == res_d );
    }
  }

  WHEN( "FLOOR.W.S $f0, $f9 and FLOOR.W.D $f21, $f17 are executed" )
  {
    auto floor_w_s = "FLOOR_W"_inst | FMT_S | 0_r1 | 9_r2;
    auto floor_w_d = "FLOOR_W"_inst | FMT_D | 21_r1 | 17_r2;

    inspector.CP1_fpr( 9 )  = 23'412.0f;
    inspector.CP1_fpr( 17 ) = -2038.309;

    auto res_s = std::uint32_t( 23'412 );
    auto res_d = std::uint32_t( -2039 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( floor_w_s );
      auto rd = cp1.execute( floor_w_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 0 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 21 ).single_binary() == res_d );
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

  WHEN( "ROUND.L.S $f1, $f2 and ROUND.L.D $f30, $f28 are executed" )
  {
    auto round_l_s = "ROUND_L"_inst | FMT_S | 1_r1 | 2_r2;
    auto round_l_d = "ROUND_L"_inst | FMT_D | 30_r1 | 28_r2;

    inspector.CP1_fpr( 2 ) = 29'842.0f;

    inspector.CP1_fpr( 28 ) = -87'431.0;

    auto res_s = std::uint64_t( std::llround( 29'842.0f ) );
    auto res_d = std::uint64_t( std::llround( -87'431.0f ) );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( round_l_s );
      auto rd = cp1.execute( round_l_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 1 ).double_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 30 ).double_binary() == res_d );
    }
  }

  WHEN( "ROUND.W.S $f1, $f2 and ROUND.W.D $f30, $f28 are executed" )
  {
    auto round_w_s = "ROUND_W"_inst | FMT_S | 1_r1 | 2_r2;
    auto round_w_d = "ROUND_W"_inst | FMT_D | 30_r1 | 28_r2;

    inspector.CP1_fpr( 2 ) = 29'842.0f;

    inspector.CP1_fpr( 28 ) = -87'431.0;

    auto res_s = std::uint32_t( std::llround( 29'842.0f ) );
    auto res_d = std::uint32_t( std::llround( -87'431.0f ) );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( round_w_s );
      auto rd = cp1.execute( round_w_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 1 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 30 ).single_binary() == res_d );
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

  WHEN( "TRUNC.L.S $f1, $f3 and TRUNC.L.D $f30, $f29 are executed" )
  {
    auto trunc_l_s = "TRUNC_L"_inst | FMT_S | 1_r1 | 3_r2;
    auto trunc_l_d = "TRUNC_L"_inst | FMT_D | 30_r1 | 29_r2;

    inspector.CP1_fpr( 3 )  = 39.89f;
    inspector.CP1_fpr( 29 ) = -1.85;

    auto res_s = std::uint64_t( 39 );
    auto res_d = std::uint64_t( -1 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( trunc_l_s );
      auto rd = cp1.execute( trunc_l_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 1 ).double_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 30 ).double_binary() == res_d );
    }
  }

  WHEN( "TRUNC.W.S $f6, $f10 and TRUNC.W.D $f4, $f13 are executed" )
  {
    auto trunc_w_s = "TRUNC_W"_inst | FMT_S | 6_r1 | 10_r2;
    auto trunc_w_d = "TRUNC_W"_inst | FMT_D | 4_r1 | 13_r2;

    inspector.CP1_fpr( 10 ) = -290.3463f;
    inspector.CP1_fpr( 13 ) = 0.025;

    auto res_s = std::uint32_t( -290 );
    auto res_d = std::uint32_t( 0 );

    THEN( "The result must be correct" )
    {
      auto rs = cp1.execute( trunc_w_s );
      auto rd = cp1.execute( trunc_w_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( inspector.CP1_fpr( 6 ).single_binary() == res_s );
      REQUIRE( inspector.CP1_fpr( 4 ).single_binary() == res_d );
    }
  }

  WHEN( "I execute reserved instructions" )
  {
    constexpr std::uint32_t opcode{0b010001 << 26};

    constexpr std::uint32_t fmt_S_D_reserved_functions[] = {
        0b100'010,
        0b100'011,
        0b100'110,
        0b100'111,

        0b101'000,
        0b101'001,
        0b101'010,
        0b101'011,
        0b101'100,
        0b101'101,
        0b101'110,
        0b101'111,

        0b110'000,
        0b110'001,
        0b110'010,
        0b110'011,
        0b110'100,
        0b110'101,
        0b110'110,
        0b110'111,

        0b111'000,
        0b111'001,
        0b111'010,
        0b111'011,
        0b111'100,
        0b111'101,
        0b111'110,
        0b111'111,
    };

    constexpr std::uint32_t fmt_W_L_reserved_functions[] = {
        0b010'000,
        0b010'100,
        0b010'101,
        0b010'110,
        0b010'111,

        0b011'000,
        0b011'100,
        0b011'101,
        0b011'110,
        0b011'111,

        0b100'010,
        0b100'011,
        0b100'100,
        0b100'101,
        0b100'110,
        0b100'111,

        0b101'000,
        0b101'001,
        0b101'010,
        0b101'011,
        0b101'100,
        0b101'101,
        0b101'110,
        0b101'111,

        0b110'000,
        0b110'001,
        0b110'010,
        0b110'011,
        0b110'100,
        0b110'101,
        0b110'110,
        0b110'111,

        0b111'000,
        0b111'001,
        0b111'010,
        0b111'011,
        0b111'100,
        0b111'101,
        0b111'110,
        0b111'111,
    };

    THEN( "Executing them shall yield a Reserved Instruction Exception" )
    {
      for ( auto fn : fmt_S_D_reserved_functions ) {
        auto const r = cp1.execute( opcode | FMT_S | fn );
        REQUIRE( r == CP1::Exception::RESERVED );
      }

      for ( auto fn : fmt_W_L_reserved_functions ) {
        auto const r = cp1.execute( opcode | FMT_W | fn );
        REQUIRE( r == CP1::Exception::RESERVED );
      }
    }
  }

  /*  WHEN( "I execute unimplemented instructions" )
  {
    constexpr std::uint32_t opcode{0b010001 << 26};
    constexpr std::uint32_t unimplemented_functions[] = {

    };

    THEN( "Executing them shall yield a Unimplemented Operation Exception" )
    {
      for ( auto fn : unimplemented_functions )
        REQUIRE( cp1.execute( opcode | fn ) == CP1::Exception::UNIMPLEMENTED );
    }
  }*/
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

  constexpr std::array<std::string_view, 40> fmt_S_D_encode_table{
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
      "_"sv, // reserved *
      "_"sv, // reserved *
      "CVT_W"sv,
      "CVT_L"sv,
      "CVT_PS"sv,
      "_"sv, // reserved *
  };

  constexpr std::array<std::string_view, 20> fmt_W_L_encode_table{
      "CMP_AF"sv,
      "CMP_UN"sv,
      "CMP_EQ"sv,
      "CMP_UEQ"sv,
      "CMP_LT"sv,
      "CMP_ULT"sv,
      "CMP_LE"sv,
      "CMP_ULE"sv,
      "CMP_SAF"sv,
      "CMP_SUN"sv,
      "CMP_SEQ"sv,
      "CMP_SUEQ"sv,
      "CMP_SLT"sv,
      "CMP_SULT"sv,
      "CMP_SLE"sv,
      "CMP_SULE"sv,
      "_"sv, // reserved *
      "CMP_OR"sv,
      "CMP_UNE"sv,
      "CMP_NE"sv,
  };

  std::uint32_t code{0};
  for ( auto const &in : fmt_S_D_encode_table ) {
    if ( in == view ) return code | opcode;
    ++code;
  }

  code = 0;
  for ( auto const &in : fmt_W_L_encode_table ) {
    if ( in == view ) return code | opcode;
    ++code;
  }

  return std::uint32_t( 63 ); // function 63 is always reserved
}
