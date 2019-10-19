#include <catch.hpp>

#include "../src/cp1.hpp"
#include <mips32/machine_inspector.hpp>

#include "helpers/test_cp1_instructions.hpp"

#include <cfenv>
#include <cmath>
#include <limits>

#define FP(n) inspector.CP1_fpr_begin() + n

using namespace mips32;

constexpr std::uint32_t FMT_S{ 0x10 << 21 };
constexpr std::uint32_t FMT_D{ 0x11 << 21 };
constexpr std::uint32_t FMT_W{ 0x14 << 21 };
constexpr std::uint32_t FMT_L{ 0x15 << 21 };

constexpr std::uint32_t CMP_FMT_S{ 0b10100 << 21 };
constexpr std::uint32_t CMP_FMT_D{ 0b10101 << 21 };

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

TEST_CASE( "A Coprocessor 1 object exists and is resetted" )
{
  CP1 cp1;
  cp1.reset();

  MachineInspector inspector;

  inspector.inspect( cp1 );

  SECTION( "Its registers shall be on their default state" )
  {
    std::uint32_t regs[4] = {
        cp1.read( 0 ),  // FIR
        cp1.read( 31 ), // FCSR
        cp1.read( 26 ), // FEXR
        cp1.read( 28 )  // FENR
    };

    REQUIRE( regs[0] == 0x00F3'0000 );
    REQUIRE( regs[1] == 0x010C'0000 );
    REQUIRE( regs[2] == ( regs[1] & 0x0003'F07C ) );
    REQUIRE( regs[3] == ( regs[1] & 0x0000'0F87 ) );
  }

  SECTION( "Writing to the registers shall not change read-only fields" )
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

    REQUIRE( before[0] == after[0] );
    REQUIRE( ( before[1] & ~0x0103'FFFF ) == ( after[1] & ~0x0103'FFFF ) );
  }

  SECTION( "Resetted FPRs are all 0" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end = inspector.CP1_fpr_end();
    while ( begin != end )
    {
      REQUIRE( begin->f == 0.0f );
      REQUIRE( begin->d == 0.0 );
      REQUIRE( begin->i32 == 0 );
      REQUIRE( begin->i64 == 0 );

      ++begin;
    }
  }

  SECTION( "Writing a float to a register through the inspector shall change the register's value" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end = inspector.CP1_fpr_end();
    while ( begin != end )
    {
      begin->f = 42.f;
      ++begin;
    }

    auto _begin = inspector.CP1_fpr_begin();
    auto _end = inspector.CP1_fpr_end();
    while ( _begin != _end )
    {
      REQUIRE( _begin->f == 42.f );
      ++_begin;
    }
  }

  SECTION( "Writing a double to a register through the inspector shall change the register's value" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end = inspector.CP1_fpr_end();
    while ( begin != end )
    {
      begin->d = 6657.0;
      ++begin;
    }

    auto _begin = inspector.CP1_fpr_begin();
    auto _end = inspector.CP1_fpr_end();
    while ( _begin != _end )
    {
      REQUIRE( _begin->d == 6657.0 );
      ++_begin;
    }
  }

  SECTION( "Writing an i32 to a register through the inspector shall change the register's value" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end = inspector.CP1_fpr_end();
    while ( begin != end )
    {
      begin->i32 = 0xABCD'7531;
      ++begin;
    }

    auto _begin = inspector.CP1_fpr_begin();
    auto _end = inspector.CP1_fpr_end();
    while ( _begin != _end )
    {
      REQUIRE( _begin->i32 == 0xABCD'7531 );
      ++_begin;
    }
  }

  SECTION( "Writing an i64 to a register through the inspector shall change the register's value" )
  {
    auto begin = inspector.CP1_fpr_begin();
    auto end = inspector.CP1_fpr_end();
    while ( begin != end )
    {
      begin->i64 = 0x8518'FBBB'9871'412C;
      ++begin;
    }

    auto _begin = inspector.CP1_fpr_begin();
    auto _end = inspector.CP1_fpr_end();
    while ( _begin != _end )
    {
      REQUIRE( _begin->i64 == 0x8518'FBBB'9871'412C );
      ++_begin;
    }
  }

  SECTION( "Changing the rounding in the FCSR shall change the rounding of the underlying FP Environment" )
  {
    /*
    FCSR is register 28
    0 RN - Round to Nearest
    1 RZ - Round Toward Zero
    2 RP - Round Towards Plus Infinity
    3 RM - Round Towards Minus Infinity
    */

    cp1.write( 28, 1 );
    REQUIRE( std::fegetround() == FE_TOWARDZERO );

    cp1.write( 28, 2 );
    REQUIRE( std::fegetround() == FE_UPWARD );

    cp1.write( 28, 3 );
    REQUIRE( std::fegetround() == FE_DOWNWARD );

    cp1.write( 28, 0 );
    REQUIRE( std::fegetround() == FE_TONEAREST );
  }

  /* * * * * * * * *
   *               *
   *  INSTRUCTIONS *
   *               *
   * * * * * * * * */

  // In alphabetically order, because that's how the manual order them

  SECTION( "ABS.S $f0, $f2 and ABS.D $f14, $f8 is executed" )
  {
    auto abs_s = "ABS"_cp1 | FMT_S | 0_r1 | 2_r2;
    auto abs_d = "ABS"_cp1 | FMT_D | 14_r1 | 8_r2;

    auto f0 = FP( 0 );
    auto f2 = FP( 2 );

    auto f14 = FP( 14 );
    auto f8 = FP( 8 );

    f2->f = -1.f;
    f8->d = -897.0;

    auto rs = cp1.execute( abs_s );
    auto rd = cp1.execute( abs_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f0->f == 1.f );
    REQUIRE( f14->d == 897.0 );
  }

  SECTION( "ADD.S $f31, $f7, $f21 and ADD.D $f18, $f17, $f16 is executed" )
  {
    auto add_s = "ADD"_cp1 | FMT_S | 31_r1 | 7_r2 | 21_r3;
    auto add_d = "ADD"_cp1 | FMT_D | 18_r1 | 17_r2 | 16_r3;

    auto f31 = FP( 31 );
    auto f7 = FP( 7 );
    auto f21 = FP( 21 );

    auto f18 = FP( 18 );
    auto f17 = FP( 17 );
    auto f16 = FP( 16 );

    f7->f = 38.f;
    f21->f = 1285.f;

    f17->d = 429.0;
    f16->d = -2943.0;

    auto res_s = 38.0f + 1285.0f;
    auto res_d = 429.0 + ( -2943.0 );

    auto rs = cp1.execute( add_s );
    auto rd = cp1.execute( add_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f31->f == res_s );
    REQUIRE( f18->d == res_d );
  }

  SECTION( "CMP.AF.S $f0, $f0, $f0 and CMP.AF.D $f4, $f5, $f4 are executed" )
  {
    auto cmp_af_s = "CMP_AF"_cp1 | CMP_FMT_S | 0_r1 | 0_r2 | 0_r3;
    auto cmp_af_d = "CMP_AF"_cp1 | CMP_FMT_D | 4_r1 | 5_r2 | 4_r3;

    auto f0 = FP( 0 );

    auto f4 = FP( 4 );

    f0->f = 256.0f;
    f4->d = 394.0;

    auto res_s = CMP_FALSE;
    auto res_d = CMP_FALSE;

    auto rs = cp1.execute( cmp_af_s );
    auto rd = cp1.execute( cmp_af_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f0->i32 == res_s );
    REQUIRE( f4->i64 == res_d );
  }

  SECTION( "CMP.UN.S $f3, $f4, $f3 and CMP.AF.D $f18, $f11, $f0 are executed" )
  {
    auto cmp_un_s = "CMP_UN"_cp1 | CMP_FMT_S | 3_r1 | 4_r2 | 3_r3;
    auto cmp_un_d = "CMP_UN"_cp1 | CMP_FMT_D | 18_r1 | 11_r2 | 0_r3;

    auto f3 = FP( 3 );
    auto f4 = FP( 4 );

    auto f18 = FP( 18 );
    auto f11 = FP( 11 );
    auto f0 = FP( 0 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f4->f = f_value_to_check[i][0];
      f3->f = f_value_to_check[i][1];

      f11->d = d_value_to_check[i][0];
      f0->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_un_s );
      auto rd = cp1.execute( cmp_un_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f3->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f18->i64 == res[i] );
    }
  }

  SECTION( "CMP.EQ.S $f30, $f29, $f30 and CMP.EQ.D $f1, $f2, $f3 are executed" )
  {
    auto cmp_eq_s = "CMP_EQ"_cp1 | CMP_FMT_S | 30_r1 | 29_r2 | 30_r3;
    auto cmp_eq_d = "CMP_EQ"_cp1 | CMP_FMT_D | 1_r1 | 2_r2 | 3_r3;

    auto f30 = FP( 30 );
    auto f29 = FP( 29 );

    auto f1 = FP( 1 );
    auto f2 = FP( 2 );
    auto f3 = FP( 3 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f29->f = f_value_to_check[i][0];
      f30->f = f_value_to_check[i][1];

      f2->d = d_value_to_check[i][0];
      f3->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_eq_s );
      auto rd = cp1.execute( cmp_eq_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f30->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f1->i64 == res[i] );
    }
  }

  SECTION( "CMP.UEQ.S $f10, $f9, $f27 and CMP.UEQ.D $f4, $f6, $f26 are executed" )
  {
    auto cmp_ueq_s = "CMP_UEQ"_cp1 | CMP_FMT_S | 10_r1 | 9_r2 | 27_r3;
    auto cmp_ueq_d = "CMP_UEQ"_cp1 | CMP_FMT_D | 4_r1 | 6_r2 | 26_r3;

    auto f10 = FP( 10 );
    auto f9 = FP( 9 );
    auto f27 = FP( 27 );

    auto f4 = FP( 4 );
    auto f6 = FP( 6 );
    auto f26 = FP( 26 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f9->f = f_value_to_check[i][0];
      f27->f = f_value_to_check[i][1];

      f6->d = d_value_to_check[i][0];
      f26->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_ueq_s );
      auto rd = cp1.execute( cmp_ueq_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f10->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f4->i64 == res[i] );
    }
  }

  SECTION( "CMP.LT.S $f5, $f10, $f15 and CMP.LT.D $f6, $f12, $18 are executed" )
  {
    auto cmp_lt_s = "CMP_LT"_cp1 | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_lt_d = "CMP_LT"_cp1 | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    auto f5 = FP( 5 );
    auto f10 = FP( 10 );
    auto f15 = FP( 15 );

    auto f6 = FP( 6 );
    auto f12 = FP( 12 );
    auto f18 = FP( 18 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f10->f = f_value_to_check[i][0];
      f15->f = f_value_to_check[i][1];

      f12->d = d_value_to_check[i][0];
      f18->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_lt_s );
      auto rd = cp1.execute( cmp_lt_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f5->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f6->i64 == res[i] );
    }
  }

  SECTION( "CMP.ULT.S $f5, $f10, $f15 and CMP.ULT.D $f6, $f12, $18 are executed" )
  {
    auto cmp_ult_s = "CMP_ULT"_cp1 | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_ult_d = "CMP_ULT"_cp1 | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    auto f5 = FP( 5 );
    auto f10 = FP( 10 );
    auto f15 = FP( 15 );

    auto f6 = FP( 6 );
    auto f12 = FP( 12 );
    auto f18 = FP( 18 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f10->f = f_value_to_check[i][0];
      f15->f = f_value_to_check[i][1];

      f12->d = d_value_to_check[i][0];
      f18->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_ult_s );
      auto rd = cp1.execute( cmp_ult_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f5->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f6->i64 == res[i] );
    }
  }

  SECTION( "CMP.LE.S $f5, $f10, $f15 and CMP.LE.D $f6, $f12, $18 are executed" )
  {
    auto cmp_le_s = "CMP_LE"_cp1 | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_le_d = "CMP_LE"_cp1 | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    auto f5 = FP( 5 );
    auto f10 = FP( 10 );
    auto f15 = FP( 15 );

    auto f6 = FP( 6 );
    auto f12 = FP( 12 );
    auto f18 = FP( 18 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f10->f = f_value_to_check[i][0];
      f15->f = f_value_to_check[i][1];

      f12->d = d_value_to_check[i][0];
      f18->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_le_s );
      auto rd = cp1.execute( cmp_le_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f5->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f6->i64 == res[i] );
    }
  }

  SECTION( "CMP.ULE.S $f5, $f10, $f15 and CMP.ULE.D $f6, $f12, $18 are executed" )
  {
    auto cmp_ule_s = "CMP_ULE"_cp1 | CMP_FMT_S | 5_r1 | 10_r2 | 15_r3;
    auto cmp_ule_d = "CMP_ULE"_cp1 | CMP_FMT_D | 6_r1 | 12_r2 | 18_r3;

    auto f5 = FP( 5 );
    auto f10 = FP( 10 );
    auto f15 = FP( 15 );

    auto f6 = FP( 6 );
    auto f12 = FP( 12 );
    auto f18 = FP( 18 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f10->f = f_value_to_check[i][0];
      f15->f = f_value_to_check[i][1];

      f12->d = d_value_to_check[i][0];
      f18->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_ule_s );
      auto rd = cp1.execute( cmp_ule_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f5->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f6->i64 == res[i] );
    }
  }

  SECTION( "CMP.OR.S $f0, $f3, $f6 and CMP.OR.D $f13, $f14, $f13 are executed" )
  {
    auto cmp_or_s = "CMP_OR"_cp1 | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_or_d = "CMP_OR"_cp1 | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    auto f0 = FP( 0 );
    auto f3 = FP( 3 );
    auto f6 = FP( 6 );

    auto f13 = FP( 13 );
    auto f14 = FP( 14 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f3->f = f_value_to_check[i][0];
      f6->f = f_value_to_check[i][1];

      f14->d = d_value_to_check[i][0];
      f13->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_or_s );
      auto rd = cp1.execute( cmp_or_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f0->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f13->i64 == res[i] );
    }
  }

  SECTION( "CMP.UNE.S $f31, $f30, $f29 and CMP.UNE.D $f18, $f21, $f2 are executed" )
  {
    auto cmp_une_s = "CMP_UNE"_cp1 | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_une_d = "CMP_UNE"_cp1 | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    auto f0 = FP( 0 );
    auto f3 = FP( 3 );
    auto f6 = FP( 6 );

    auto f13 = FP( 13 );
    auto f14 = FP( 14 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f3->f = f_value_to_check[i][0];
      f6->f = f_value_to_check[i][1];

      f14->d = d_value_to_check[i][0];
      f13->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_une_s );
      auto rd = cp1.execute( cmp_une_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f0->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f13->i64 == res[i] );
    }
  }

  SECTION( "CMP.NE.S $f6, $f12, $f0 and CMP.NE.D $f3, $f5, $f19 are executed" )
  {
    auto cmp_ne_s = "CMP_NE"_cp1 | CMP_FMT_S | 0_r1 | 3_r2 | 6_r3;
    auto cmp_ne_d = "CMP_NE"_cp1 | CMP_FMT_D | 13_r1 | 14_r2 | 13_r3;

    auto f0 = FP( 0 );
    auto f3 = FP( 3 );
    auto f6 = FP( 6 );

    auto f13 = FP( 13 );
    auto f14 = FP( 14 );

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

    for ( int i = 0; i < 4; ++i )
    {
      f3->f = f_value_to_check[i][0];
      f6->f = f_value_to_check[i][1];

      f14->d = d_value_to_check[i][0];
      f13->d = d_value_to_check[i][1];

      auto rs = cp1.execute( cmp_ne_s );
      auto rd = cp1.execute( cmp_ne_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f0->i32 == std::uint32_t( res[i] ) );
      REQUIRE( f13->i64 == res[i] );
    }
  }

  SECTION( "CEIL.L.S $f0, $f0 and CEIL.L.D $f13, $f17 are executed" )
  {
    auto ceil_l_s = "CEIL_L"_cp1 | FMT_S | 0_r1 | 0_r2;
    auto ceil_l_d = "CEIL_L"_cp1 | FMT_D | 13_r1 | 17_r2;

    auto f0 = FP( 0 );

    auto f13 = FP( 13 );
    auto f17 = FP( 17 );

    f0->f = 3947.6241f;
    f17->d = -20.39;

    auto res_s = std::uint64_t( 3948 );
    auto res_d = std::uint64_t( -20 );

    auto rs = cp1.execute( ceil_l_s );
    auto rd = cp1.execute( ceil_l_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f0->i64 == res_s );
    REQUIRE( f13->i64 == res_d );
  }

  SECTION( "CEIL.W.S $f3, $f21 and CEIL.W.D $f8, $f19 are executed" )
  {
    auto ceil_w_s = "CEIL_W"_cp1 | FMT_S | 3_r1 | 21_r2;
    auto ceil_w_d = "CEIL_W"_cp1 | FMT_D | 8_r1 | 19_r2;

    auto f3 = FP( 3 );
    auto f21 = FP( 21 );

    auto f8 = FP( 8 );
    auto f19 = FP( 19 );

    f21->f = -54'876.3487f;
    f19->d = 98'723.93;

    auto res_s = std::uint32_t( -54'876 );
    auto res_d = std::uint32_t( 98'724 );

    auto rs = cp1.execute( ceil_w_s );
    auto rd = cp1.execute( ceil_w_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f3->i32 == res_s );
    REQUIRE( f8->i32 == res_d );
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
  SECTION( "CLASS.S $f0, $f2 and CLASS.D $f31, $f31 are executed" )
  {
    auto class_s = "CLASS"_cp1 | FMT_S | 0_r1 | 2_r2;
    auto class_d = "CLASS"_cp1 | FMT_D | 31_r1 | 31_r2;

    auto f0 = FP( 0 );
    auto f2 = FP( 2 );

    auto f31 = FP( 31 );

    // Quiet NaN
    SECTION( "Quiet NaN should be classified correctly" )
    {
      f2->f = QNAN<float>{};
      f31->d = QNAN<double>{};

      auto res_s = std::uint32_t( 1 << 1 );
      auto res_d = std::uint64_t( 1 << 1 );

      auto rs = cp1.execute( class_s );
      auto rd = cp1.execute( class_d );

      REQUIRE( rs == CP1::Exception::NONE );
      REQUIRE( rd == CP1::Exception::NONE );

      REQUIRE( f0->i32 == res_s );
      REQUIRE( f31->i64 == res_d );
    }

    // [NEGATIVE] Infinity + Normal + Denormal + Zero
    SECTION( "[NEGATIVE] Infinity + Normal + Denormal + Zero" )
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

      for ( int i = 0; i < 4; ++i )
      {
        f2->f = f_value_to_test[i];
        f31->d = d_value_to_test[i];

        auto rs = cp1.execute( class_s );
        auto rd = cp1.execute( class_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( f0->i32 == res_s[i] );
        REQUIRE( f31->i64 == res_d[i] );
      }
    }

    // [POSITIVE] Infinity + Normal + Denormal + Zero
    SECTION( "[POSITIVE] Infinity + Normal + Denormal + Zero" )
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

      for ( int i = 0; i < 4; ++i )
      {
        f2->f = f_value_to_test[i];
        f31->d = d_value_to_test[i];

        auto rs = cp1.execute( class_s );
        auto rd = cp1.execute( class_d );

        REQUIRE( rs == CP1::Exception::NONE );
        REQUIRE( rd == CP1::Exception::NONE );

        REQUIRE( f0->i32 == res_s[i] );
        REQUIRE( f31->i64 == res_d[i] );
      }
    }
  }

  SECTION( "CVT.D.S $f19, $f15 and CVT.D.W $f0, $f1 and CVT.D.L $f31, $f31 are executed" )
  {
    auto cvt_d_s = "CVT_D"_cp1 | FMT_S | 19_r1 | 15_r2;
    auto cvt_d_w = "CVT_D"_cp1 | FMT_W | 0_r1 | 1_r2;
    auto cvt_d_l = "CVT_D"_cp1 | FMT_L | 31_r1 | 31_r2;

    auto f19 = FP( 19 );
    auto f15 = FP( 15 );

    auto f0 = FP( 0 );
    auto f1 = FP( 1 );

    auto f31 = FP( 31 );

    f15->f = 1509.f;
    f1->i32 = ( std::uint32_t ) - 2874;
    f31->i64 = 34'903;

    auto res_s = 1509.0;
    auto res_w = -2874.0;
    auto res_l = 34'903.0;

    auto rs = cp1.execute( cvt_d_s );
    auto rw = cp1.execute( cvt_d_w );
    auto rl = cp1.execute( cvt_d_l );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rw == CP1::Exception::NONE );
    REQUIRE( rl == CP1::Exception::NONE );

    REQUIRE( f19->d == res_s );
    REQUIRE( f0->d == res_w );
    REQUIRE( f31->d == res_l );
  }

  SECTION( "CVT.L.S $f8, $f27 and CVT.L.D $f30, $f24 are executed" )
  {
    auto cvt_l_s = "CVT_L"_cp1 | FMT_S | 8_r1 | 27_r2;
    auto cvt_l_d = "CVT_L"_cp1 | FMT_D | 30_r1 | 24_r2;

    auto f8 = FP( 8 );
    auto f27 = FP( 27 );

    auto f30 = FP( 30 );
    auto f24 = FP( 24 );

    f27->f = -3094.0f;
    f24->d = 9074.0;

    auto res_s = std::uint64_t( -3094 );
    auto res_d = std::uint64_t( 9074 );

    auto rs = cp1.execute( cvt_l_s );
    auto rd = cp1.execute( cvt_l_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f8->i64 == res_s );
    REQUIRE( f30->i64 == res_d );
  }

  SECTION( "CVT.S.D $f3, $f9 and CVT.S.W $f11, $f0 and CVT.S.L $f4, $f22 are executed" )
  {
    auto cvt_s_d = "CVT_S"_cp1 | FMT_D | 3_r1 | 9_r2;
    auto cvt_s_w = "CVT_S"_cp1 | FMT_W | 11_r1 | 0_r2;
    auto cvt_s_l = "CVT_S"_cp1 | FMT_L | 4_r1 | 22_r2;

    auto f3 = FP( 3 );
    auto f9 = FP( 9 );

    auto f11 = FP( 11 );
    auto f0 = FP( 0 );

    auto f4 = FP( 4 );
    auto f22 = FP( 22 );

    f9->d = 8374.0;
    f0->i32 = 2166;
    f22->i64 = ( std::uint64_t ) - 348'763'328;

    auto res_d = 8374.0f;
    auto res_w = 2166;
    auto res_l = -348'763'328;

    auto rd = cp1.execute( cvt_s_d );
    auto rw = cp1.execute( cvt_s_w );
    auto rl = cp1.execute( cvt_s_l );

    REQUIRE( rd == CP1::Exception::NONE );
    REQUIRE( rw == CP1::Exception::NONE );
    REQUIRE( rl == CP1::Exception::NONE );

    REQUIRE( f3->f == res_d );
    REQUIRE( f11->f == res_w );
    REQUIRE( f4->f == res_l );
  }

  SECTION( "CVT.W.S $f13, $f31 and CVT.W.D $f21, $f12 are executed" )
  {
    auto cvt_w_s = "CVT_W"_cp1 | FMT_S | 13_r1 | 31_r2;
    auto cvt_w_d = "CVT_W"_cp1 | FMT_D | 21_r1 | 12_r2;

    auto f13 = FP( 13 );
    auto f31 = FP( 31 );

    auto f21 = FP( 21 );
    auto f12 = FP( 12 );

    f31->f = 23'984.0f;
    f12->d = -4309.0;

    auto res_s = std::uint32_t( 23'984 );
    auto res_d = std::uint32_t( -4309 );

    auto rs = cp1.execute( cvt_w_s );
    auto rd = cp1.execute( cvt_w_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f13->i32 == res_s );
    REQUIRE( f21->i32 == res_d );
  }

  SECTION( "DIV.S $f1, $f2, $f3 and DIV.D $f4, $f5, $f6 are executed" )
  {
    auto div_s = "DIV"_cp1 | FMT_S | 1_r1 | 2_r2 | 3_r3;
    auto div_d = "DIV"_cp1 | FMT_D | 4_r1 | 5_r2 | 6_r3;

    auto f1 = FP( 1 );
    auto f2 = FP( 2 );
    auto f3 = FP( 3 );

    auto f4 = FP( 4 );
    auto f5 = FP( 5 );
    auto f6 = FP( 6 );

    f2->f = 121.0f;
    f3->f = 11.0f;

    f5->d = 240.0;
    f6->d = 2.0;

    auto res_s = 121.0f / 11.0f;
    auto res_d = 240.0f / 2.0f;

    auto rs = cp1.execute( div_s );
    auto rd = cp1.execute( div_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f1->f == res_s );
    REQUIRE( f4->d == res_d );
  }

  SECTION( "FLOOR.L.S $f3, $f3 and FLOOR.L.D $f2, $f7 are executed" )
  {
    auto floor_l_s = "FLOOR_L"_cp1 | FMT_S | 3_r1 | 3_r2;
    auto floor_l_d = "FLOOR_L"_cp1 | FMT_D | 2_r1 | 7_r2;

    auto f3 = FP( 3 );

    auto f2 = FP( 2 );
    auto f7 = FP( 7 );

    f3->f = 23'412.8f;
    f7->d = -2038.309;

    auto res_s = std::uint64_t( 23'412 );
    auto res_d = std::uint64_t( -2039 );

    auto rs = cp1.execute( floor_l_s );
    auto rd = cp1.execute( floor_l_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f3->i64 == res_s );
    REQUIRE( f2->i64 == res_d );
  }

  SECTION( "FLOOR.W.S $f0, $f9 and FLOOR.W.D $f21, $f17 are executed" )
  {
    auto floor_w_s = "FLOOR_W"_cp1 | FMT_S | 0_r1 | 9_r2;
    auto floor_w_d = "FLOOR_W"_cp1 | FMT_D | 21_r1 | 17_r2;

    auto f0 = FP( 0 );
    auto f9 = FP( 9 );

    auto f21 = FP( 21 );
    auto f17 = FP( 17 );

    f9->f = 23'412.0f;
    f17->d = -2038.309;

    auto res_s = std::uint32_t( 23'412 );
    auto res_d = std::uint32_t( -2039 );

    auto rs = cp1.execute( floor_w_s );
    auto rd = cp1.execute( floor_w_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f0->i32 == res_s );
    REQUIRE( f21->i32 == res_d );
  }

  SECTION( "MADDF.S $f10, $f11, $f12 and MADDF.D $f20, $f20, $f20 are executed" )
  {
    auto maddf_s = "MADDF"_cp1 | FMT_S | 10_r1 | 11_r2 | 12_r3;
    auto maddf_d = "MADDF"_cp1 | FMT_D | 20_r1 | 20_r2 | 20_r3;

    auto f10 = FP( 10 );
    auto f11 = FP( 11 );
    auto f12 = FP( 12 );

    auto f20 = FP( 20 );

    f10->f = 31.0f;
    f11->f = 89.0f;
    f12->f = 61000.0f;

    f20->d = 14912.0;

    auto res_s = std::fma( 89.0f, 61000.0f, 31.0f );
    auto res_d = std::fma( 14912.0, 14912.0, 14912.0 );

    auto rs = cp1.execute( maddf_s );
    auto rd = cp1.execute( maddf_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f10->f == res_s );
    REQUIRE( f20->d == res_d );
  }

  SECTION( "MSUBF.S $f9, $f21, $f13 and MSUBF.D $f1, $f1, $f1 are executed" )
  {
    auto msubf_s = "MSUBF"_cp1 | FMT_S | 9_r1 | 21_r2 | 13_r3;
    auto msubf_d = "MSUBF"_cp1 | FMT_D | 1_r1 | 1_r2 | 1_r3;

    auto f9 = FP( 9 );
    auto f21 = FP( 21 );
    auto f13 = FP( 13 );

    auto f1 = FP( 1 );

    f9->f = 9.0f;
    f21->f = 40'000.0f;
    f13->f = 2.0f;

    f1->d = 7202.0;

    auto res_s = 9.0f - ( 40'000.0f / 2.0f );
    auto res_d = 7202.0 - ( 7202.0 / 7202.0 );

    auto rs = cp1.execute( msubf_s );
    auto rd = cp1.execute( msubf_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f9->f == res_s );
    REQUIRE( f1->d == res_d );
  }

  SECTION( "MAX.S $f5, $f8, $f0 and MAX.D $f0, $f11, $f11 are executed" )
  {
    auto max_s = "MAX"_cp1 | FMT_S | 5_r1 | 8_r2 | 0_r3;
    auto max_d = "MAX"_cp1 | FMT_D | 0_r1 | 11_r2 | 11_r3;

    auto f5 = FP( 5 );
    auto f8 = FP( 8 );
    auto f0 = FP( 0 );

    auto f11 = FP( 11 );

    f8->f = 60.0f;
    f0->f = 59.0f;

    f11->d = 239'457.0;

    auto res_s = 60.0f;
    auto res_d = 239'457.0;

    auto rs = cp1.execute( max_s );
    auto rd = cp1.execute( max_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f5->f == res_s );
    REQUIRE( f0->d == res_d );
  }

  SECTION( "MIN.S $f18, $f30, $f27 and MIN.D $f4, $f25, $f22 are executed" )
  {
    auto max_s = "MIN"_cp1 | FMT_S | 18_r1 | 30_r2 | 27_r3;
    auto max_d = "MIN"_cp1 | FMT_D | 4_r1 | 25_r2 | 22_r3;

    auto f18 = FP( 18 );
    auto f30 = FP( 30 );
    auto f27 = FP( 27 );

    auto f4 = FP( 4 );
    auto f25 = FP( 25 );
    auto f22 = FP( 22 );

    f30->f = 8345.0f;
    f27->f = 34'897.0f;

    f25->d = -98'345.0;
    f22->d = 0.0;

    auto res_s = 8345.0f;
    auto res_d = -98'345.0;

    auto rs = cp1.execute( max_s );
    auto rd = cp1.execute( max_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f18->f == res_s );
    REQUIRE( f4->d == res_d );
  }

  SECTION( "MAXA.S $f26, $f12, $f29 and MAXA.D $f0, $f1, $f6 are executed" )
  {
    auto maxa_s = "MAXA"_cp1 | FMT_S | 26_r1 | 12_r2 | 29_r3;
    auto maxa_d = "MAXA"_cp1 | FMT_D | 0_r1 | 1_r2 | 6_r3;

    auto f26 = FP( 26 );
    auto f12 = FP( 12 );
    auto f29 = FP( 29 );

    auto f0 = FP( 0 );
    auto f1 = FP( 1 );
    auto f6 = FP( 6 );

    f12->f = -3984.0f;
    f29->f = -6230.0f;

    f1->d = 923.0;
    f6->d = -18'000.0;

    auto res_s = 6230.0f;
    auto res_d = 18'000.0;

    auto rs = cp1.execute( maxa_s );
    auto rd = cp1.execute( maxa_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f26->f == res_s );
    REQUIRE( f0->d == res_d );
  }

  SECTION( "MINA.S $f31, $f30, $f29 and MINA.D $f2, $f4, $f8 are executed" )
  {
    auto mina_s = "MINA"_cp1 | FMT_S | 31_r1 | 30_r2 | 29_r3;
    auto mina_d = "MINA"_cp1 | FMT_D | 2_r1 | 4_r2 | 8_r3;

    auto f31 = FP( 31 );
    auto f30 = FP( 30 );
    auto f29 = FP( 29 );

    auto f2 = FP( 2 );
    auto f4 = FP( 4 );
    auto f8 = FP( 8 );

    f30->f = -245.0f;
    f29->f = -988.0f;

    f4->d = 586.0;
    f8->d = -6000.0;

    auto res_s = 245.0f;
    auto res_d = 586.0;

    auto rs = cp1.execute( mina_s );
    auto rd = cp1.execute( mina_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f31->f == res_s );
    REQUIRE( f2->d == res_d );
  }

  SECTION( "MOV.S $f10, $f18 and MOV.D $f21, $f3 are executed" )
  {
    auto mov_s = "MOV"_cp1 | FMT_S | 10_r1 | 18_r2;
    auto mov_d = "MOV"_cp1 | FMT_D | 21_r1 | 3_r2;

    auto f10 = FP( 10 );
    auto f18 = FP( 18 );

    auto f21 = FP( 21 );
    auto f3 = FP( 3 );

    f18->f = 681.0f;

    f3->d = -50'000.0;

    auto res_s = 681.0f;
    auto res_d = -50'000.0;

    auto rs = cp1.execute( mov_s );
    auto rd = cp1.execute( mov_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f10->f == res_s );
    REQUIRE( f21->d == res_d );
  }

  SECTION( "MUL.S $f15, $f9, $f2 and MUL.D $f13, $f0, $f7 are executed" )
  {
    auto mul_s = "MUL"_cp1 | FMT_S | 15_r1 | 9_r2 | 2_r3;
    auto mul_d = "MUL"_cp1 | FMT_D | 13_r1 | 0_r2 | 7_r3;

    auto f15 = FP( 15 );
    auto f9 = FP( 9 );
    auto f2 = FP( 2 );

    auto f13 = FP( 13 );
    auto f0 = FP( 0 );
    auto f7 = FP( 7 );

    f9->f = 2.0f;
    f2->f = -1024.0f;

    f0->d = 88.0;
    f7->d = 621.0;

    auto res_s = 2.0f * -1024.0f;
    auto res_d = 88.0 * 621.0;

    auto rs = cp1.execute( mul_s );
    auto rd = cp1.execute( mul_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f15->f == res_s );
    REQUIRE( f13->d == res_d );
  }

  SECTION( "NEG.S $f5, $f6 and NEG.D $f7, $f8 are executed" )
  {
    auto neg_s = "NEG"_cp1 | FMT_S | 5_r1 | 6_r2;
    auto neg_d = "NEG"_cp1 | FMT_D | 7_r1 | 8_r2;

    auto f5 = FP( 5 );
    auto f6 = FP( 6 );

    auto f7 = FP( 7 );
    auto f8 = FP( 9 );

    f6->f = 1234.0f;

    f8->d = 0.0;

    auto res_s = -1234.0f;
    auto res_d = -0.0;

    auto rs = cp1.execute( neg_s );
    auto rd = cp1.execute( neg_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f5->f == res_s );
    REQUIRE( f7->d == res_d );
  }

  SECTION( "RECIP.S $f9, $f8 and RECIP.D $f31, $f0 are executed" )
  {
    auto recip_s = "RECIP"_cp1 | FMT_S | 9_r1 | 8_r2;
    auto recip_d = "RECIP"_cp1 | FMT_D | 31_r1 | 0_r2;

    auto f9 = FP( 9 );
    auto f8 = FP( 8 );

    auto f31 = FP( 31 );
    auto f0 = FP( 0 );

    f8->f = 67.0f;

    f0->d = -851.0;

    auto res_s = 1.0f / 67.0f;
    auto res_d = 1.0 / -851.0;

    auto rs = cp1.execute( recip_s );
    auto rd = cp1.execute( recip_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f9->f == res_s );
    REQUIRE( f31->d == res_d );
  }

  SECTION( "RINT.S $f1, $f18 and RINT.D $f4, $f26 are executed" )
  {
    auto rint_s = "RINT"_cp1 | FMT_S | 1_r1 | 18_r2;
    auto rint_d = "RINT"_cp1 | FMT_D | 4_r1 | 26_r2;

    auto f1 = FP( 1 );
    auto f18 = FP( 18 );

    auto f4 = FP( 4 );
    auto f26 = FP( 26 );

    f18->f = 29'842.0f;

    f26->d = -87'431.0;

    auto res_s = std::uint32_t( std::llrint( 29'842.0f ) );
    auto res_d = std::uint64_t( std::llrint( -87'431.0 ) );

    auto rs = cp1.execute( rint_s );
    auto rd = cp1.execute( rint_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f1->i32 == res_s );
    REQUIRE( f4->i64 == res_d );
  }

  SECTION( "ROUND.L.S $f1, $f2 and ROUND.L.D $f30, $f28 are executed" )
  {
    auto round_l_s = "ROUND_L"_cp1 | FMT_S | 1_r1 | 2_r2;
    auto round_l_d = "ROUND_L"_cp1 | FMT_D | 30_r1 | 28_r2;

    auto f1 = FP( 1 );
    auto f2 = FP( 2 );

    auto f30 = FP( 30 );
    auto f28 = FP( 28 );

    f2->f = 29'842.0f;

    f28->d = -87'431.0;

    auto res_s = std::uint64_t( std::llround( 29'842.0f ) );
    auto res_d = std::uint64_t( std::llround( -87'431.0f ) );

    auto rs = cp1.execute( round_l_s );
    auto rd = cp1.execute( round_l_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f1->i64 == res_s );
    REQUIRE( f30->i64 == res_d );
  }

  SECTION( "ROUND.W.S $f1, $f2 and ROUND.W.D $f30, $f28 are executed" )
  {
    auto round_w_s = "ROUND_W"_cp1 | FMT_S | 1_r1 | 2_r2;
    auto round_w_d = "ROUND_W"_cp1 | FMT_D | 30_r1 | 28_r2;

    auto f1 = FP( 1 );
    auto f2 = FP( 2 );

    auto f30 = FP( 30 );
    auto f28 = FP( 28 );

    f2->f = 29'842.0f;

    f28->d = -87'431.0;

    auto res_s = std::uint32_t( std::llround( 29'842.0f ) );
    auto res_d = std::uint32_t( std::llround( -87'431.0f ) );

    auto rs = cp1.execute( round_w_s );
    auto rd = cp1.execute( round_w_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f1->i32 == res_s );
    REQUIRE( f30->i32 == res_d );
  }

  SECTION( "RSQRT.S $f17, $f21 and RSQRT.D $f24, $f30 are executed" )
  {
    auto rsqrt_s = "RSQRT"_cp1 | FMT_S | 17_r1 | 21_r2;
    auto rsqrt_d = "RSQRT"_cp1 | FMT_D | 24_r1 | 30_r2;

    auto f17 = FP( 17 );
    auto f21 = FP( 21 );

    auto f24 = FP( 24 );
    auto f30 = FP( 30 );

    f21->f = 25.0f;

    f30->d = 256.0;

    auto res_s = 1.0f / std::sqrt( 25.0f );
    auto res_d = 1.0 / std::sqrt( 256.0 );

    auto rs = cp1.execute( rsqrt_s );
    auto rd = cp1.execute( rsqrt_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f17->f == res_s );
    REQUIRE( f24->d == res_d );
  }

  SECTION( "SEL.S $f8, $f11, $f12 and SEL.D $f0, $f1, $f3 are executed" )
  {
    auto sel_s = "SEL"_cp1 | FMT_S | 8_r1 | 11_r2 | 12_r3;
    auto sel_d = "SEL"_cp1 | FMT_D | 0_r1 | 1_r2 | 3_r3;

    auto f8 = FP( 8 );
    auto f11 = FP( 11 );
    auto f12 = FP( 12 );

    auto f0 = FP( 0 );
    auto f1 = FP( 1 );
    auto f3 = FP( 3 );

    f8->i32 = 0;
    f11->f = 48.0f;
    f12->f = 18'000.0f;

    f0->i64 = 1;
    f1->d = -8888.0;
    f3->d = -141.0;

    auto res_s = 48.0f;
    auto res_d = -141.0;

    auto rs = cp1.execute( sel_s );
    auto rd = cp1.execute( sel_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f8->f == res_s );
    REQUIRE( f0->d == res_d );
  }

  SECTION( "SELEQZ.S $f9, $f8, $f7 and SELEQZ.D $f27, $f26, $f25 are executed" )
  {
    auto seleqz_s = "SELEQZ"_cp1 | FMT_S | 9_r1 | 8_r2 | 7_r3;
    auto seleqz_d = "SELEQZ"_cp1 | FMT_D | 27_r1 | 26_r2 | 25_r3;

    auto f9 = FP( 9 );
    auto f8 = FP( 8 );
    auto f7 = FP( 7 );

    auto f27 = FP( 27 );
    auto f26 = FP( 26 );
    auto f25 = FP( 25 );

    f9->f = 92'837.0f;
    f8->f = 564.0f;
    f7->i32 = 1;

    f27->d = 39'847.0;
    f26->d = 987.0;
    f25->i64 = 0;

    auto res_s = 0.0f;
    auto res_d = 987.0;

    auto rs = cp1.execute( seleqz_s );
    auto rd = cp1.execute( seleqz_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f9->f == res_s );
    REQUIRE( f27->d == res_d );
  }

  SECTION( "SELNEZ.S $f22, $f3, $f4 and SELNEZ.D $f18, $f22, $f14 are executed" )
  {
    auto selnez_s = "SELNEZ"_cp1 | FMT_S | 22_r1 | 3_r2 | 4_r3;
    auto selnez_d = "SELNEZ"_cp1 | FMT_D | 18_r1 | 22_r2 | 14_r3;

    auto f22 = FP( 22 );
    auto f3 = FP( 3 );
    auto f4 = FP( 4 );

    auto f18 = FP( 18 );
    auto f14 = FP( 14 );

    f3->f = 17.0f;
    f4->i32 = 1;

    f22->d = -41'000.0;
    f14->i64 = 0;

    auto res_s = 17.0f;
    auto res_d = 0.0;

    auto rs = cp1.execute( selnez_s );
    auto rd = cp1.execute( selnez_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f22->f == res_s );
    REQUIRE( f18->d == res_d );
  }

  SECTION( "SQRT.S $f13, $f7 and SQRT.D $f14, $f8 are executed" )
  {
    auto sqrt_s = "SQRT"_cp1 | FMT_S | 13_r1 | 7_r2;
    auto sqrt_d = "SQRT"_cp1 | FMT_D | 14_r1 | 8_r2;

    auto f13 = FP( 13 );
    auto f7 = FP( 7 );

    auto f14 = FP( 14 );
    auto f8 = FP( 8 );

    f7->f = 1024.0f;

    f8->d = 16'000.0;

    auto res_s = std::sqrt( 1024.0f );
    auto res_d = std::sqrt( 16'000.0 );

    auto rs = cp1.execute( sqrt_s );
    auto rd = cp1.execute( sqrt_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f13->f == res_s );
    REQUIRE( f14->d == res_d );
  }

  SECTION( "SUB.S $f10, $f9, $f10 and SUB.D $f1, $f1, $f1 are executed" )
  {
    auto sub_s = "SUB"_cp1 | FMT_S | 10_r1 | 9_r2 | 10_r3;
    auto sub_d = "SUB"_cp1 | FMT_D | 1_r1 | 1_r2 | 1_r3;

    auto f10 = FP( 10 );
    auto f9 = FP( 9 );

    auto f1 = FP( 1 );

    f9->f = 15.0f;
    f10->f = -61'000.0f;

    f1->d = -1985.0;

    auto res_s = 15.0f - ( -61'000.0f );
    auto res_d = -1985.0 - ( -1985.0 );

    auto rs = cp1.execute( sub_s );
    auto rd = cp1.execute( sub_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f10->f == res_s );
    REQUIRE( f1->d == res_d );
  }

  SECTION( "TRUNC.L.S $f1, $f3 and TRUNC.L.D $f30, $f29 are executed" )
  {
    auto trunc_l_s = "TRUNC_L"_cp1 | FMT_S | 1_r1 | 3_r2;
    auto trunc_l_d = "TRUNC_L"_cp1 | FMT_D | 30_r1 | 29_r2;

    auto f1 = FP( 1 );
    auto f3 = FP( 3 );

    auto f30 = FP( 30 );
    auto f29 = FP( 29 );

    f3->f = 39.89f;
    f29->d = -1.85;

    auto res_s = std::uint64_t( 39 );
    auto res_d = std::uint64_t( -1 );

    auto rs = cp1.execute( trunc_l_s );
    auto rd = cp1.execute( trunc_l_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f1->i64 == res_s );
    REQUIRE( f30->i64 == res_d );
  }

  SECTION( "TRUNC.W.S $f6, $f10 and TRUNC.W.D $f4, $f13 are executed" )
  {
    auto trunc_w_s = "TRUNC_W"_cp1 | FMT_S | 6_r1 | 10_r2;
    auto trunc_w_d = "TRUNC_W"_cp1 | FMT_D | 4_r1 | 13_r2;

    auto f6 = FP( 6 );
    auto f10 = FP( 10 );

    auto f4 = FP( 4 );
    auto f13 = FP( 13 );

    f10->f = -290.3463f;
    f13->d = 0.025;

    auto res_s = std::uint32_t( -290 );
    auto res_d = std::uint32_t( 0 );

    auto rs = cp1.execute( trunc_w_s );
    auto rd = cp1.execute( trunc_w_d );

    REQUIRE( rs == CP1::Exception::NONE );
    REQUIRE( rd == CP1::Exception::NONE );

    REQUIRE( f6->i32 == res_s );
    REQUIRE( f4->i32 == res_d );
  }

  SECTION( "I execute reserved instructions" )
  {
    constexpr std::uint32_t opcode{ 0b010001 << 26 };

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

    for ( auto fn : fmt_S_D_reserved_functions )
    {
      auto const r = cp1.execute( opcode | FMT_S | fn );
      REQUIRE( r == CP1::Exception::RESERVED );
    }

    for ( auto fn : fmt_W_L_reserved_functions )
    {
      auto const r = cp1.execute( opcode | FMT_W | fn );
      REQUIRE( r == CP1::Exception::RESERVED );
    }
  }

  // Used to increase coverage, because it's certain that they work
  SECTION( "I read FIR or FCSR" )
  {
    inspector.CP1_fir();
    inspector.CP1_fcsr();
  }
}

#undef FP