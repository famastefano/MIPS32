#include <mips32/cp1.hpp>

#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>

#include <pmmintrin.h>
#include <xmmintrin.h>

// TODO: save/restore the Floating-Point Environment in a RAII way.

// TODO: add assertions to every function to check for fmt

// Access to the Floating-Point Environment
#ifdef _MSC_VER
#pragma fenv_access( on )
#else
#pragma STDC FENV_ACCESS ON
#endif

// Disable warnings about truncation of compile time constants
// Disable warnings on parenthesis with '&' and '|'
#ifdef _MSC_VER
#pragma warning( disable : 4309 )
#elif __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#endif

inline constexpr std::uint32_t ROUND_NEAREST{0x0};
inline constexpr std::uint32_t ROUND_ZERO{0x1};
inline constexpr std::uint32_t ROUND_UP{0x2};
inline constexpr std::uint32_t ROUND_DOWN{0x3};

inline constexpr std::uint32_t FUNCTION{0x3F};

inline constexpr std::uint32_t FMT_S{0x10};
inline constexpr std::uint32_t FMT_D{0x11};

inline constexpr std::uint32_t CMP_FMT_S{0b10100};
inline constexpr std::uint32_t CMP_FMT_D{0b10101};
inline constexpr std::uint64_t CMP_TRUE{0xFFFF'FFFF'FFFF'FFFF};
inline constexpr std::uint64_t CMP_FALSE{0};

constexpr bool is_SNaN( float f ) noexcept { return f == std::numeric_limits<float>::signaling_NaN(); }
constexpr bool is_SNaN( double d ) noexcept { return d == std::numeric_limits<double>::signaling_NaN(); }

#define MIPS32_STATIC_CAST( T, v ) static_cast<typename std::remove_reference<decltype( this->fpr[_fs].*T )>::type>( v )

namespace mips32 {

std::uint32_t CP1::round() const noexcept
{
  return fcsr & 0x3;
}
std::uint32_t CP1::flags() const noexcept
{
  return fcsr & 0x7C >> 2;
}
std::uint32_t CP1::enable() const noexcept
{
  return fcsr & 0xF80 >> 7;
}
std::uint32_t CP1::cause() const noexcept
{
  return fcsr & 0x3'F000 >> 12;
}

void CP1::set_flags( std::uint32_t flag ) noexcept
{
  fcsr |= flag & 0x1F << 2;
}
void CP1::set_cause( std::uint32_t data ) noexcept
{
  fcsr |= data & 0x2F << 12;
}

bool CP1::handle_fpu_ex() noexcept
{
  if ( !std::fetestexcept( FE_ALL_EXCEPT ) ) return false;

  static constexpr std::array<std::uint32_t, 5> _flags{
      INVALID,
      DIVBYZERO,
      OVERFLOW_,
      UNDERFLOW_,
      INEXACT,
  };

  std::uint32_t ex{0};

  switch ( std::fetestexcept( FE_ALL_EXCEPT ) ) {
  case FE_INVALID: ex = 0; break;
  case FE_DIVBYZERO: ex = 1; break;
  case FE_OVERFLOW: ex = 2; break;
  case FE_UNDERFLOW: ex = 3; break;
  case FE_INEXACT: ex = 4; break;
  }
  std::feclearexcept( FE_ALL_EXCEPT );

  set_cause( _flags[ex] );

  if ( enable() & _flags[ex] ) {
    return true; // Trap
  } else {
    set_cause( _flags[ex] );
    return false;
  }
}

constexpr std::uint32_t fmt( std::uint32_t word ) noexcept
{
  return word & 0x03E0'0000 >> 21;
}

constexpr std::uint32_t fd( std::uint32_t word ) noexcept
{
  return word & 0x07C0 >> 6;
}

constexpr std::uint32_t fs( std::uint32_t word ) noexcept
{
  return word & 0xF800 >> 11;
}

constexpr std::uint32_t ft( std::uint32_t word ) noexcept
{
  return word & 0x1F'0000 >> 16;
}

void CP1::set_round_mode() noexcept
{
  switch ( round() ) {
  case ROUND_NEAREST: std::fesetround( FE_TONEAREST ); break;
  case ROUND_ZERO: std::fesetround( FE_TOWARDZERO ); break;
  case ROUND_UP: std::fesetround( FE_UPWARD ); break;
  case ROUND_DOWN: std::fesetround( FE_DOWNWARD ); break;
  }
}

void CP1::set_denormal_flush() noexcept
{
  if ( fcsr & ( 1 << 24 ) ) {
    _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
    _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
  } else {
    _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_OFF );
    _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_OFF );
  }
}

void CP1::reset() noexcept
{
  *this = {};

  /*
  fir
     23 -> HAS 2008    -> 1
     22 -> F64         -> 1
     21 -> L           -> 0
     20 -> W           -> 0
     17 -> D           -> 1
     16 -> S           -> 1
  15..8 -> ProcessorID -> 0
   7..0 -> Revision    -> 0
  */
  fir = 0x00C3'0000;

  /*
  fcsr
  24 -> Flush Subnormals -> 1
  19 -> ABS2008          -> 1
  18 -> NAN2008          -> 1
  */
  fcsr = 0x010C'0000;

  set_round_mode();

  set_denormal_flush();
}

std::uint32_t CP1::read( std::uint32_t reg ) noexcept
{
  if ( reg == 0 )
    return fir;
  else if ( reg == 31 )
    return fcsr;
  else if ( reg == 26 )
    return fcsr & 0x0003'F07C;
  else if ( reg == 28 )
    return fcsr & 0x0000'0F87;

  assert( "Unimplemented Coprocessor 1 Register." );
  return 0;
}

void CP1::write( std::uint32_t reg, std::uint32_t data ) noexcept
{
  assert( ( reg == 0 || reg == 31 || reg == 26 || reg == 28 ) && "Unimplemented Coprocessor 1 Register." );

  if ( reg == 0 )
    return; // fir is read only
  else if ( reg == 31 )
    fcsr = fcsr & ~0x0163'FFFF | data & 0x0163'FFFF;
  else if ( reg == 26 )
    fcsr = fcsr & ~0x0003'F07C | data & 0x0003'F07C;
  else if ( reg == 28 )
    fcsr = fcsr & ~0x0000'0F87 | data & 0x0000'0F87;

  set_round_mode();

  set_denormal_flush();
}

CP1::Exception CP1::execute( std::uint32_t word ) noexcept
{
  assert( ( ( ( word & 0xFC00'0000 ) >> 26 ) == 0b010001 ) && "Invalid Opcode!" );

#if defined( __GNUC__ ) && !defined( __clang__ )
  static const
#else
  static constexpr
#endif
      std::array<int ( CP1::* )( std::uint32_t ), 64>
          function_table{
              &CP1::add,
              &CP1::sub,
              &CP1::mul,
              &CP1::div,
              &CP1::sqrt,
              &CP1::abs,
              &CP1::mov,
              &CP1::neg,
              &CP1::unimplemented, // ROUND.L
              &CP1::unimplemented, // TRUNC.L
              &CP1::unimplemented, // CEIL.L
              &CP1::unimplemented, // FLOOR.L
              &CP1::unimplemented, // ROUND.W
              &CP1::unimplemented, // TRUNC.W
              &CP1::unimplemented, // CEIL.W
              &CP1::unimplemented, // FLOOR.W
              &CP1::sel,
              &CP1::reserved, // MOVCF [6R]
              &CP1::reserved, // MOVZ  [6R]
              &CP1::reserved, // MOVN  [6R]
              &CP1::seleqz,
              &CP1::recip,
              &CP1::rsqrt,
              &CP1::selnez,
              &CP1::maddf,
              &CP1::msubf,
              &CP1::rint,
              &CP1::class_,
              &CP1::min,
              &CP1::max,
              &CP1::mina,
              &CP1::maxa,
              &CP1::cvt_s,
              &CP1::cvt_d,
              &CP1::reserved,      // *
              &CP1::reserved,      // *
              &CP1::unimplemented, // CVT.L
              &CP1::unimplemented, // CVT.W
              &CP1::unimplemented, // CVT.PS
              &CP1::reserved,      // *
              &CP1::cabs_af,
              &CP1::cabs_un,
              &CP1::cabs_eq,
              &CP1::cabs_ueq,
              &CP1::cabs_lt,
              &CP1::cabs_ult,
              &CP1::cabs_le,
              &CP1::cabs_ule,
              &CP1::cabs_saf,
              &CP1::cabs_sun,
              &CP1::cabs_seq,
              &CP1::cabs_sueq,
              &CP1::cabs_slt,
              &CP1::cabs_sult,
              &CP1::cabs_sle,
              &CP1::cabs_sule,
          };

  int v = ( this->*function_table[word & FUNCTION] )( word );

  if ( v == 1 ) {
    return (CP1::Exception)cause();
  } else if ( v == -1 ) {
    return RESERVED;
  } else {
    return NONE;
  }
}

int CP1::reserved( std::uint32_t word ) noexcept { return -1; }
int CP1::unimplemented( std::uint32_t word ) noexcept
{
  set_cause( CP1::UNIMPLEMENTED );
  return 1;
}

// TODO: Add assertions for `fmt` and correct fpr

int CP1::add( std::uint32_t word ) noexcept
{
  auto _add = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fs].*t + this->fpr[_ft].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;
    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _add( &FPR::f );
  else
    return _add( &FPR::d );
}
int CP1::sub( std::uint32_t word ) noexcept
{
  auto _sub = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fs].*t - this->fpr[_ft].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _sub( &FPR::f );
  else
    return _sub( &FPR::f );
}
int CP1::mul( std::uint32_t word ) noexcept
{
  auto _mul = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fs].*t * this->fpr[_ft].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _mul( &FPR::f );
  else
    return _mul( &FPR::d );
}
int CP1::div( std::uint32_t word ) noexcept
{
  auto _div = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fs].*t / this->fpr[_ft].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _div( &FPR::f );
  else
    return _div( &FPR::d );
}
int CP1::sqrt( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );

  auto _sqrt = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::sqrt( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  if ( _fmt == FMT_S )
    return _sqrt( &FPR::f );
  else
    return _sqrt( &FPR::d );
}
int CP1::abs( std::uint32_t word ) noexcept
{
  auto _abs = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = std::fabs( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _abs( &FPR::f );
  else
    return _abs( &FPR::d );
}
int CP1::mov( std::uint32_t word ) noexcept
{
  auto _mov = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = this->fpr[_fs].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _mov( &FPR::f );
  else
    return _mov( &FPR::d );
}
int CP1::neg( std::uint32_t word ) noexcept
{
  auto _neg = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = -( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _neg( &FPR::f );
  else
    return _neg( &FPR::d );
}
int CP1::sel( std::uint32_t word ) noexcept
{
  auto _sel = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fd].*i & 0x1 ? this->fpr[_ft].*t : this->fpr[_fs].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _sel( &FPR::f, &FPR::i32 );
  else
    return _sel( &FPR::d, &FPR::i64 );
}
int CP1::seleqz( std::uint32_t word ) noexcept
{
  auto _seleqz = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = this->fpr[_fd].*i & 0x1 ? 0 : this->fpr[_fs].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _seleqz( &FPR::f, &FPR::i32 );
  else
    return _seleqz( &FPR::d, &FPR::i64 );
}
int CP1::recip( std::uint32_t word ) noexcept
{
  auto _recip = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = 1.0f / this->fpr[_fs].*t;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _recip( &FPR::f );
  else
    return _recip( &FPR::d );
}
int CP1::rsqrt( std::uint32_t word ) noexcept
{
  auto _rsqrt = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = 1.0f / std::sqrt( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _rsqrt( &FPR::f );
  else
    return _rsqrt( &FPR::d );
}
int CP1::selnez( std::uint32_t word ) noexcept
{
  auto _selnez = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fd].*i & 0x1 ? this->fpr[_ft].*t : 0;
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _selnez( &FPR::f, &FPR::i32 );
  else
    return _selnez( &FPR::d, &FPR::i64 );
}
int CP1::maddf( std::uint32_t word ) noexcept
{
  auto _maddf = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::fma( this->fpr[_fs].*t, this->fpr[_ft].*t, this->fpr[_fd].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _maddf( &FPR::f );
  else
    return _maddf( &FPR::d );
}
int CP1::msubf( std::uint32_t word ) noexcept
{
  auto _msubf = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fd].*t - ( this->fpr[_fs].*t * this->fpr[_ft].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _msubf( &FPR::f );
  else
    return _msubf( &FPR::d );
}
int CP1::rint( std::uint32_t word ) noexcept
{
  auto _rint = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = MIPS32_STATIC_CAST( i, std::llrint( this->fpr[_fs].*t ) );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*i = res;
    ;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _rint( &FPR::f, &FPR::i32 );
  else
    return _rint( &FPR::d, &FPR::i64 );
}
int CP1::class_( std::uint32_t word ) noexcept
{
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

  auto _classify = [this, word]( auto t, auto i ) {
    constexpr std::uint32_t SNAN{0x01};
    constexpr std::uint32_t QNAN{0x02};
    constexpr std::uint32_t INFINITY_{0x04};
    constexpr std::uint32_t NORMAL{0x08};
    constexpr std::uint32_t SUBNORMAL{0x10};
    constexpr std::uint32_t ZERO{0x20};

    auto const _fd = fd( word );
    auto const _fs = fs( word );

    switch ( std::fpclassify( this->fpr[_fs].*t ) ) {
    case FP_INFINITE: this->fpr[_fd].*i = INFINITY_; break;
    case FP_NAN: this->fpr[_fd].*i = QNAN; break;
    case FP_NORMAL: this->fpr[_fd].*i = NORMAL; break;
    case FP_SUBNORMAL: this->fpr[_fd].*i = SUBNORMAL; break;
    case FP_ZERO: this->fpr[_fd].*i = ZERO; break;
    }

    if ( this->fpr[_fd].*i != QNAN && this->fpr[_fs].*t > 0 ) {
      this->fpr[_fd].*i <<= 4;
    }

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _classify( &FPR::f, &FPR::i32 );
  else
    return _classify( &FPR::d, &FPR::i64 );
}
int CP1::min( std::uint32_t word ) noexcept
{
  auto _min = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::fmin( this->fpr[_fs].*t, this->fpr[_ft].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _min( &FPR::f );
  else
    return _min( &FPR::d );
}
int CP1::max( std::uint32_t word ) noexcept
{
  auto _max = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::fmax( this->fpr[_fs].*t, this->fpr[_ft].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _max( &FPR::f );
  else
    return _max( &FPR::d );
}
int CP1::mina( std::uint32_t word ) noexcept
{
  auto _mina = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::fmin( std::fabs( this->fpr[_fs].*t ), std::fabs( this->fpr[_ft].*t ) );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _mina( &FPR::f );
  else
    return _mina( &FPR::d );
}
int CP1::maxa( std::uint32_t word ) noexcept
{
  auto _maxa = [this, word]( auto t ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = std::fmax( std::fabs( this->fpr[_fs].*t ), std::fabs( this->fpr[_ft].*t ) );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*t = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _maxa( &FPR::f );
  else
    return _maxa( &FPR::d );
}
int CP1::cvt_s( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );
  auto const _fd  = fd( word );
  auto const _fs  = fs( word );

  fpr[_fd].f = (float)fpr[_fs].d;

  return 0;
}
int CP1::cvt_d( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );
  auto const _fd  = fd( word );
  auto const _fs  = fs( word );

  fpr[_fd].d = (double)fpr[_fs].f;

  return 0;
}
int CP1::cabs_af( std::uint32_t word ) noexcept
{
  auto _cabs_af = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = CMP_FALSE;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_af( &FPR::f, &FPR::i32 );
  else
    return _cabs_af( &FPR::d, &FPR::i64 );
}
int CP1::cabs_un( std::uint32_t word ) noexcept
{
  auto _cabs_un = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    if ( std::isunordered( fpr[_fs].d, fpr[_ft].d ) )
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_TRUE );
    else
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_un( &FPR::f, &FPR::i32 );
  else
    return _cabs_un( &FPR::d, &FPR::i64 );
}
int CP1::cabs_eq( std::uint32_t word ) noexcept
{
  auto _cabs_eq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_eq( &FPR::f, &FPR::i32 );
  else
    return _cabs_eq( &FPR::d, &FPR::i64 );
}
int CP1::cabs_ueq( std::uint32_t word ) noexcept
{
  auto _cabs_ueq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_ueq( &FPR::f, &FPR::i32 );
  else
    return _cabs_ueq( &FPR::d, &FPR::i64 );
}
int CP1::cabs_lt( std::uint32_t word ) noexcept
{
  auto _cabs_lt = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }
    this->fpr[_fd].*i = std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_lt( &FPR::f, &FPR::i32 );
  else
    return _cabs_lt( &FPR::d, &FPR::i64 );
}
int CP1::cabs_ult( std::uint32_t word ) noexcept
{
  auto _cabs_ult = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_ult( &FPR::f, &FPR::i32 );
  else
    return _cabs_ult( &FPR::d, &FPR::i64 );
}
int CP1::cabs_le( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );

  auto _cabs_le = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      this->set_cause( CP1::INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  if ( _fmt == CMP_FMT_S )
    return _cabs_le( &FPR::f, &FPR::i32 );
  else
    return _cabs_le( &FPR::d, &FPR::i64 );
}
int CP1::cabs_ule( std::uint32_t word ) noexcept
{
  auto _cabs_ule = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( is_SNaN( this->fpr[_fd].*t ) || is_SNaN( this->fpr[_fs].*t ) || is_SNaN( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_ule( &FPR::f, &FPR::i32 );
  else
    return _cabs_ule( &FPR::d, &FPR::i64 );
}
int CP1::cabs_saf( std::uint32_t word ) noexcept
{
  auto _cabs_saf = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_saf( &FPR::f, &FPR::i32 );
  else
    return _cabs_saf( &FPR::d, &FPR::i64 );
}
int CP1::cabs_sun( std::uint32_t word ) noexcept
{
  auto _cabs_sun = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_sun( &FPR::f, &FPR::i32 );
  else
    return _cabs_sun( &FPR::d, &FPR::i64 );
}
int CP1::cabs_seq( std::uint32_t word ) noexcept
{
  auto _cabs_seq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_seq( &FPR::f, &FPR::i32 );
  else
    return _cabs_seq( &FPR::d, &FPR::i64 );
}
int CP1::cabs_sueq( std::uint32_t word ) noexcept
{
  auto _cabs_sueq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_sueq( &FPR::f, &FPR::i32 );
  else
    return _cabs_sueq( &FPR::d, &FPR::i64 );
}
int CP1::cabs_slt( std::uint32_t word ) noexcept
{
  auto _cabs_slt = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }
    this->fpr[_fd].*i = std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_slt( &FPR::f, &FPR::i32 );
  else
    return _cabs_slt( &FPR::d, &FPR::i64 );
}
int CP1::cabs_sult( std::uint32_t word ) noexcept
{
  auto _cabs_sult = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_sult( &FPR::f, &FPR::i32 );
  else
    return _cabs_sult( &FPR::d, &FPR::i64 );
}
int CP1::cabs_sle( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );

  auto _cabs_sle = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      this->set_cause( CP1::INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  if ( _fmt == CMP_FMT_S )
    return _cabs_sle( &FPR::f, &FPR::i32 );
  else
    return _cabs_sle( &FPR::d, &FPR::i64 );
}
int CP1::cabs_sule( std::uint32_t word ) noexcept
{
  auto _cabs_sule = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isnan( this->fpr[_fd].*t ) || std::isnan( this->fpr[_fs].*t ) || std::isnan( this->fpr[_ft].*t ) ) {
      set_cause( INVALID );
      return 1;
    }

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cabs_sule( &FPR::f, &FPR::i32 );
  else
    return _cabs_sule( &FPR::d, &FPR::i64 );
}

} // namespace mips32

#undef MIPS32_STATIC_CAST

#ifdef _MSC_VER
#pragma warning( default : 4309 )
#elif __clang__
#pragma clang diagnostic pop
#endif