#include "cp1.hpp"

#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>

#include <pmmintrin.h>
#include <xmmintrin.h>

// Access to the Floating-Point Environment
#ifdef _MSC_VER
#pragma fenv_access( on )
#else
#pragma STDC FENV_ACCESS ON
#endif

// Disable warnings about truncation of compile time constants
#ifdef _MSC_VER
#pragma warning( disable : 4309 )
#endif

inline constexpr std::uint32_t ROUND_NEAREST{ 0x0 };
inline constexpr std::uint32_t ROUND_ZERO{ 0x1 };
inline constexpr std::uint32_t ROUND_UP{ 0x2 };
inline constexpr std::uint32_t ROUND_DOWN{ 0x3 };

inline constexpr std::uint32_t FUNCTION{ 0x3F };

inline constexpr std::uint32_t FMT_S{ 0x10 };
inline constexpr std::uint32_t FMT_D{ 0x11 };
inline constexpr std::uint32_t FMT_W{ 0x14 };
inline constexpr std::uint32_t FMT_L{ 0x15 };

inline constexpr std::uint32_t CMP_FMT_S{ 0b10100 };
inline constexpr std::uint32_t CMP_FMT_D{ 0b10101 };
inline constexpr std::uint64_t CMP_TRUE{ 0xFFFF'FFFF'FFFF'FFFF };
inline constexpr std::uint64_t CMP_FALSE{ 0 };

#define MIPS32_STATIC_CAST( T, v ) static_cast<typename std::remove_reference<decltype( this->fpr[_fs].*T )>::type>( v )

namespace mips32
{

CP1::CP1() noexcept
{
  std::fexcept_t flags;
  std::fegetenv( &env );
  std::fesetexceptflag( &flags, FE_ALL_EXCEPT );
}

CP1::~CP1() noexcept
{
  std::fesetenv( &env );
}

std::uint32_t CP1::round() const noexcept
{
  return fcsr & 0x3;
}
std::uint32_t CP1::flags() const noexcept
{
  return ( fcsr & 0x7C ) >> 2;
}
std::uint32_t CP1::enable() const noexcept
{
  return ( fcsr & 0xF80 ) >> 7;
}
std::uint32_t CP1::cause() const noexcept
{
  return ( fcsr & 0x3'F000 ) >> 12;
}

void CP1::set_flags( std::uint32_t flag ) noexcept
{
  fcsr |= ( flag & 0x1F ) << 2;
}
void CP1::set_cause( std::uint32_t data ) noexcept
{
  fcsr |= ( data & 0x2F ) << 12;
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

  std::uint32_t ex{ 0 };

  switch ( std::fetestexcept( FE_ALL_EXCEPT ) )
  {
  case FE_INVALID: ex = 0; break;
  case FE_DIVBYZERO: ex = 1; break;
  case FE_OVERFLOW: ex = 2; break;
  case FE_UNDERFLOW: ex = 3; break;
  case FE_INEXACT: ex = 4; break;
  }
  std::feclearexcept( FE_ALL_EXCEPT );

  set_cause( _flags[ex] );

  if ( enable() & _flags[ex] )
  {
    return true; // Trap
  }
  else
  {
    set_cause( _flags[ex] );
    return false;
  }
}

constexpr std::uint32_t fmt( std::uint32_t word ) noexcept
{
  return ( word & 0x03E0'0000 ) >> 21;
}

constexpr std::uint32_t fd( std::uint32_t word ) noexcept
{
  return ( word & 0x07C0 ) >> 6;
}

constexpr std::uint32_t fs( std::uint32_t word ) noexcept
{
  return ( word & 0xF800 ) >> 11;
}

constexpr std::uint32_t ft( std::uint32_t word ) noexcept
{
  return ( word & 0x1F'0000 ) >> 16;
}

bool valid_fmt( std::uint32_t word ) noexcept
{
  switch ( fmt( word ) )
  {
  case FMT_S:
  case FMT_D:
  case FMT_W:
  case FMT_L:
    return true;
  default:
    return false;
  }
}

void CP1::set_round_mode() noexcept
{
  switch ( round() )
  {
  case ROUND_NEAREST: std::fesetround( FE_TONEAREST ); break;
  case ROUND_ZERO: std::fesetround( FE_TOWARDZERO ); break;
  case ROUND_UP: std::fesetround( FE_UPWARD ); break;
  case ROUND_DOWN: std::fesetround( FE_DOWNWARD ); break;
  }
}

void CP1::set_denormal_flush() noexcept
{
  if ( fcsr & ( 1 << 24 ) )
  {
    _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
    _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
  }
  else
  {
    _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_OFF );
    _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_OFF );
  }
}

void CP1::reset() noexcept
{
  std::memset( fpr.data(), 0, sizeof( FPR ) * fpr.size() );

  /*
  fir
     23 -> HAS 2008    -> 1
     22 -> F64         -> 1
     21 -> L           -> 1
     20 -> W           -> 1
     17 -> D           -> 1
     16 -> S           -> 1
  15..8 -> ProcessorID -> 0
   7..0 -> Revision    -> 0
  */
  fir = 0x00F3'0000;

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
  assert( ( reg == 0 || reg == 31 || reg == 26 || reg == 28 ) && "Unimplemented Coprocessor 1 Register." );

  if ( reg == 0 )
    return fir;
  else if ( reg == 31 )
    return fcsr;
  else if ( reg == 26 )
    return fcsr & 0x0003'F07C;
  else
    return fcsr & 0x0000'0F87;
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

  assert( valid_fmt( word ) && "Invalid format!" );

  static constexpr std::array<int ( CP1::* )( std::uint32_t ) noexcept, 64>
    fmt_S_D_fn_table{
        &CP1::add,
        &CP1::sub,
        &CP1::mul,
        &CP1::div,
        &CP1::sqrt,
        &CP1::abs,
        &CP1::mov,
        &CP1::neg,
        &CP1::round_l,
        &CP1::trunc_l,
        &CP1::ceil_l,
        &CP1::floor_l,
        &CP1::round_w,
        &CP1::trunc_w,
        &CP1::ceil_w,
        &CP1::floor_w,
        &CP1::sel,
        &CP1::reserved, // MOVCF
        &CP1::reserved, // MOVZ
        &CP1::reserved, // MOVN
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
        &CP1::reserved, // *
        &CP1::reserved, // *
        &CP1::cvt_w,
        &CP1::cvt_l,
        &CP1::reserved, // *
        &CP1::reserved, // *
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        // Pre-Release 6 c.condn.fmt
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
  };

  static constexpr std::array<int ( CP1::* )( std::uint32_t ) noexcept, 64>
    fmt_W_L_fn_table{
        &CP1::cmp_af,
        &CP1::cmp_un,
        &CP1::cmp_eq,
        &CP1::cmp_ueq,
        &CP1::cmp_lt,
        &CP1::cmp_ult,
        &CP1::cmp_le,
        &CP1::cmp_ule,
        &CP1::unimplemented, // CMP.SAF.fmt
        &CP1::unimplemented, // CMP.SUN.fmt
        &CP1::unimplemented, // CMP.SEQ.fmt
        &CP1::unimplemented, // CMP.SUEQ.fmt
        &CP1::unimplemented, // CMP.SLT.fmt
        &CP1::unimplemented, // CMP.SULT.fmt
        &CP1::unimplemented, // CMP.SLE.fmt
        &CP1::unimplemented, // CMP.SULE.fmt
        &CP1::reserved,
        &CP1::cmp_or,
        &CP1::cmp_une,
        &CP1::cmp_ne,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::unimplemented, // CMP.SOR.FMT
        &CP1::unimplemented, // CMP.SUNE.FMT
        &CP1::unimplemented, // CMP.SNE.FMT
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::cvt_s,
        &CP1::cvt_d,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved, // CVT.PS.PW
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
        &CP1::reserved,
  };

  int v;

  switch ( fmt( word ) )
  {
  default:
    return RESERVED;
  case FMT_S:
  case FMT_D:
    v = ( this->*fmt_S_D_fn_table[word & FUNCTION] )( word );
    break;
  case FMT_W:
  case FMT_L:
    v = ( this->*fmt_W_L_fn_table[word & FUNCTION] )( word );
    break;
  }

  if ( v == 1 )
  {
    return ( CP1::Exception )cause();
  }
  else if ( v == -1 )
  {
    return RESERVED;
  }
  else
  {
    return NONE;
  }
}

std::uint32_t CP1::mfc1( std::uint32_t reg ) noexcept
{
  assert( reg < 32 && "Invalid Floating-Point Register!" );

  return std::uint32_t( fpr[reg].i64 & 0xFFFF'FFFF );
}
std::uint32_t CP1::mfhc1( std::uint32_t reg ) noexcept
{
  assert( reg < 32 && "Invalid Floating-Point Register!" );

  return std::uint32_t( fpr[reg].i64 >> 32 );
}

void CP1::mtc1( std::uint32_t reg, std::uint32_t word ) noexcept
{
  assert( reg < 32 && "Invalid Floating-Point Register!" );

  fpr[reg].i64 = ( fpr[reg].i64 & 0xFFFF'FFFF'0000'0000ULL ) | word;
}
void CP1::mthc1( std::uint32_t reg, std::uint32_t word ) noexcept
{
  assert( reg < 32 && "Invalid Floating-Point Register!" );

  fpr[reg].i64 = ( fpr[reg].i64 & 0xFFFF'FFFF ) | ( std::uint64_t( word ) << 32 );
}

int CP1::reserved( std::uint32_t ) noexcept { return -1; }
int CP1::unimplemented( std::uint32_t ) noexcept
{
  set_cause( CP1::UNIMPLEMENTED );
  return 1;
}

int CP1::add( std::uint32_t word ) noexcept
{
  auto _add = [ this, word ] ( auto t )
  {
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
  auto _sub = [ this, word ] ( auto t )
  {
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
    return _sub( &FPR::d );
}
int CP1::mul( std::uint32_t word ) noexcept
{
  auto _mul = [ this, word ] ( auto t )
  {
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
  auto _div = [ this, word ] ( auto t )
  {
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

  auto _sqrt = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

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
  auto _abs = [ this, word ] ( auto t )
  {
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
  auto _mov = [ this, word ] ( auto t )
  {
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
  auto _neg = [ this, word ] ( auto t )
  {
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
int CP1::round_l( std::uint32_t word ) noexcept
{
  auto _round_l = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = std::llround( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i64 = std::uint64_t( res );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _round_l( &FPR::f );
  else
    return _round_l( &FPR::d );
}
int CP1::trunc_l( std::uint32_t word ) noexcept
{
  auto _trunc_l = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint64_t )std::trunc( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i64 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _trunc_l( &FPR::f );
  else
    return _trunc_l( &FPR::d );
}
int CP1::ceil_l( std::uint32_t word ) noexcept
{
  auto _ceil_l = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint64_t )std::ceil( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i64 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _ceil_l( &FPR::f );
  else
    return _ceil_l( &FPR::d );
}
int CP1::floor_l( std::uint32_t word ) noexcept
{
  auto _floor_l = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint64_t )std::floor( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i64 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _floor_l( &FPR::f );
  else
    return _floor_l( &FPR::d );
}
int CP1::round_w( std::uint32_t word ) noexcept
{
  auto _round_w = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = std::lround( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i32 = std::uint32_t( res );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _round_w( &FPR::f );
  else
    return _round_w( &FPR::d );
}
int CP1::trunc_w( std::uint32_t word ) noexcept
{
  auto _trunc_w = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint32_t )std::trunc( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i32 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _trunc_w( &FPR::f );
  else
    return _trunc_w( &FPR::d );
}
int CP1::ceil_w( std::uint32_t word ) noexcept
{
  auto _ceil_w = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint32_t )std::ceil( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i32 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _ceil_w( &FPR::f );
  else
    return _ceil_w( &FPR::d );
}
int CP1::floor_w( std::uint32_t word ) noexcept
{
  auto _floor_w = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint32_t )std::floor( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i32 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _floor_w( &FPR::f );
  else
    return _floor_w( &FPR::d );
}
int CP1::sel( std::uint32_t word ) noexcept
{
  auto _sel = [ this, word ] ( auto t, auto i )
  {
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
  auto _seleqz = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_ft].*i & 0x1 ? 0 : this->fpr[_fs].*t;
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
  auto _recip = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = MIPS32_STATIC_CAST( t, 1.0 ) / this->fpr[_fs].*t;
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
  auto _rsqrt = [ this, word ] ( auto t )
  {
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
  auto _selnez = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_ft].*i & 0x1 ? this->fpr[_fs].*t : 0;
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
  auto _maddf = [ this, word ] ( auto t )
  {
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
  auto _msubf = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    auto const res = this->fpr[_fd].*t - ( this->fpr[_fs].*t / this->fpr[_ft].*t );
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
  auto _rint = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = MIPS32_STATIC_CAST( i, std::llrint( this->fpr[_fs].*t ) );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].*i = res;

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

  auto _classify = [ this, word ] ( auto t, auto i )
  {
//constexpr std::uint32_t SNAN{0x01};
    constexpr std::uint32_t QNAN{ 0x02 };
    constexpr std::uint32_t INFINITY_{ 0x04 };
    constexpr std::uint32_t NORMAL{ 0x08 };
    constexpr std::uint32_t SUBNORMAL{ 0x10 };
    constexpr std::uint32_t ZERO{ 0x20 };

    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto value = this->fpr[_fs].*t; // copy in case fd == fs

    auto class_type = std::fpclassify( value );

    switch ( class_type )
    {
    case FP_INFINITE: this->fpr[_fd].*i = INFINITY_; break;
    case FP_NAN: this->fpr[_fd].*i = QNAN; break;
    case FP_NORMAL: this->fpr[_fd].*i = NORMAL; break;
    case FP_SUBNORMAL: this->fpr[_fd].*i = SUBNORMAL; break;
    case FP_ZERO: this->fpr[_fd].*i = ZERO; break;
    }

    if ( class_type != FP_NAN && !std::signbit( value ) )
    {
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
  auto _min = [ this, word ] ( auto t )
  {
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
  auto _max = [ this, word ] ( auto t )
  {
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
  auto _mina = [ this, word ] ( auto t )
  {
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
  auto _maxa = [ this, word ] ( auto t )
  {
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
  auto _cvt_s = [ this, word ] ( auto t, int raw_data_type )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    float res;

    if ( raw_data_type == 0 )
      res = ( float )( this->fpr[_fs].*t );
    else if ( raw_data_type == 1 )
      res = ( float )( std::int32_t )( this->fpr[_fs].*t );
    else
      res = ( float )( std::int64_t )( this->fpr[_fs].*t );

    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].f = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_D )
    return _cvt_s( &FPR::d, 0 );
  else if ( _fmt == FMT_W )
    return _cvt_s( &FPR::i32, 1 );
  else
    return _cvt_s( &FPR::i64, 2 );
}
int CP1::cvt_d( std::uint32_t word ) noexcept
{
  auto _cvt_d = [ this, word ] ( auto t, int raw_data_type )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    double res;
    if ( raw_data_type == 0 )
      res = ( double )( this->fpr[_fs].*t );
    else if ( raw_data_type == 1 )
      res = ( double )( std::int32_t )( this->fpr[_fs].*t );
    else
      res = ( double )( std::int64_t )( this->fpr[_fs].*t );

    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].d = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_S )
    return _cvt_d( &FPR::f, 0 );
  else if ( _fmt == FMT_W )
    return _cvt_d( &FPR::i32, 1 );
  else
    return _cvt_d( &FPR::i64, 2 );
}
int CP1::cvt_l( std::uint32_t word ) noexcept
{
  auto _cvt_l = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint64_t )( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i64 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_D )
    return _cvt_l( &FPR::d );
  else
    return _cvt_l( &FPR::f );
}
int CP1::cvt_w( std::uint32_t word ) noexcept
{
  auto _cvt_w = [ this, word ] ( auto t )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );

    auto const res = ( std::uint32_t )( this->fpr[_fs].*t );
    if ( handle_fpu_ex() ) return 1;
    this->fpr[_fd].i32 = res;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == FMT_D )
    return _cvt_w( &FPR::d );
  else
    return _cvt_w( &FPR::f );
}
int CP1::cmp_af( std::uint32_t word ) noexcept
{
  auto _cmp_af = [ this, word ] ( auto i )
  {
    auto const _fd = fd( word );

    this->fpr[_fd].*i = CMP_FALSE;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_af( &FPR::i32 );
  else
    return _cmp_af( &FPR::i64 );
}
int CP1::cmp_un( std::uint32_t word ) noexcept
{
  auto _cmp_un = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isunordered( fpr[_fs].*t, fpr[_ft].*t ) )
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_TRUE );
    else
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_un( &FPR::f, &FPR::i32 );
  else
    return _cmp_un( &FPR::d, &FPR::i64 );
}
int CP1::cmp_eq( std::uint32_t word ) noexcept
{
  auto _cmp_eq = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_eq( &FPR::f, &FPR::i32 );
  else
    return _cmp_eq( &FPR::d, &FPR::i64 );
}
int CP1::cmp_ueq( std::uint32_t word ) noexcept
{
  auto _cmp_ueq = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_ueq( &FPR::f, &FPR::i32 );
  else
    return _cmp_ueq( &FPR::d, &FPR::i64 );
}
int CP1::cmp_lt( std::uint32_t word ) noexcept
{
  auto _cmp_lt = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_lt( &FPR::f, &FPR::i32 );
  else
    return _cmp_lt( &FPR::d, &FPR::i64 );
}
int CP1::cmp_ult( std::uint32_t word ) noexcept
{
  auto _cmp_ult = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_ult( &FPR::f, &FPR::i32 );
  else
    return _cmp_ult( &FPR::d, &FPR::i64 );
}
int CP1::cmp_le( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );

  auto _cmp_le = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  if ( _fmt == CMP_FMT_S )
    return _cmp_le( &FPR::f, &FPR::i32 );
  else
    return _cmp_le( &FPR::d, &FPR::i64 );
}
int CP1::cmp_ule( std::uint32_t word ) noexcept
{
  auto _cmp_ule = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_ule( &FPR::f, &FPR::i32 );
  else
    return _cmp_ule( &FPR::d, &FPR::i64 );
}

int CP1::cmp_or( std::uint32_t word ) noexcept
{
  auto _cmp_or = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( !std::isunordered( fpr[_fs].*t, fpr[_ft].*t ) )
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_TRUE );
    else
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_or( &FPR::f, &FPR::i32 );
  else
    return _cmp_or( &FPR::d, &FPR::i64 );
}

int CP1::cmp_une( std::uint32_t word ) noexcept
{
  auto _cmp_une = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) || this->fpr[_fs].*t != this->fpr[_ft].*t )
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_TRUE );
    else
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_une( &FPR::f, &FPR::i32 );
  else
    return _cmp_une( &FPR::d, &FPR::i64 );
}

int CP1::cmp_ne( std::uint32_t word ) noexcept
{
  auto _cmp_ne = [ this, word ] ( auto t, auto i )
  {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    if ( this->fpr[_fs].*t != this->fpr[_ft].*t )
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_TRUE );
    else
      this->fpr[_fd].*i = MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_ne( &FPR::f, &FPR::i32 );
  else
    return _cmp_ne( &FPR::d, &FPR::i64 );
}

// Signaling NaN is not supported
/*
int CP1::cmp_saf( std::uint32_t word ) noexcept
{
  auto _cmp_saf = [this, word]( auto i ) {
    auto const _fd = fd( word );

    this->fpr[_fd].*i = CMP_FALSE;

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_saf( &FPR::i32 );
  else
    return _cmp_saf( &FPR::i64 );
}
int CP1::cmp_sun( std::uint32_t word ) noexcept
{
  auto _cmp_sun = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_sun( &FPR::f, &FPR::i32 );
  else
    return _cmp_sun( &FPR::d, &FPR::i64 );
}
int CP1::cmp_seq( std::uint32_t word ) noexcept
{
  auto _cmp_seq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_seq( &FPR::f, &FPR::i32 );
  else
    return _cmp_seq( &FPR::d, &FPR::i64 );
}
int CP1::cmp_sueq( std::uint32_t word ) noexcept
{
  auto _cmp_sueq = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= this->fpr[_fs].*t == this->fpr[_ft].*t ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_sueq( &FPR::f, &FPR::i32 );
  else
    return _cmp_sueq( &FPR::d, &FPR::i64 );
}
int CP1::cmp_slt( std::uint32_t word ) noexcept
{
  auto _cmp_slt = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_slt( &FPR::f, &FPR::i32 );
  else
    return _cmp_slt( &FPR::d, &FPR::i64 );
}
int CP1::cmp_sult( std::uint32_t word ) noexcept
{
  auto _cmp_sult = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::isless( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_sult( &FPR::f, &FPR::i32 );
  else
    return _cmp_sult( &FPR::d, &FPR::i64 );
}
int CP1::cmp_sle( std::uint32_t word ) noexcept
{
  auto const _fmt = fmt( word );

  auto _cmp_sle = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  if ( _fmt == CMP_FMT_S )
    return _cmp_sle( &FPR::f, &FPR::i32 );
  else
    return _cmp_sle( &FPR::d, &FPR::i64 );
}
int CP1::cmp_sule( std::uint32_t word ) noexcept
{
  auto _cmp_sule = [this, word]( auto t, auto i ) {
    auto const _fd = fd( word );
    auto const _fs = fs( word );
    auto const _ft = ft( word );

    this->fpr[_fd].*i = std::isunordered( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );
    this->fpr[_fd].*i |= std::islessequal( this->fpr[_fs].*t, this->fpr[_ft].*t ) ? MIPS32_STATIC_CAST( i, CMP_TRUE ) : MIPS32_STATIC_CAST( i, CMP_FALSE );

    return 0;
  };

  auto const _fmt = fmt( word );

  if ( _fmt == CMP_FMT_S )
    return _cmp_sule( &FPR::f, &FPR::i32 );
  else
    return _cmp_sule( &FPR::d, &FPR::i64 );
}
*/

} // namespace mips32

#undef MIPS32_STATIC_CAST

#ifdef _MSC_VER
#pragma warning( default : 4309 )
#endif