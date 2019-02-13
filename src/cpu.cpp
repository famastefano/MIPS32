#include "cpu.hpp"

#include <cstring>

namespace mips32
{

std::initializer_list<MMU::Segment> fixed_mapping_segments{
    {0x0000'0000, 0x7FFF'FFFF, MMU::Segment::ALL},       // useg
    {0x8000'0000, 0x3FFF'FFFF, MMU::Segment::KERNEL},     // kseg0 + kseg1
    {0xC000'0000, 0x1FFF'FFFF, MMU::Segment::SUPERVISOR | MMU::Segment::KERNEL}, // ksseg
    {0xE000'0000, 0x1FFF'FFFF, MMU::Segment::KERNEL},     // kseg3
};

constexpr std::uint32_t opcode( std::uint32_t word ) noexcept;
constexpr std::uint32_t rs( std::uint32_t word ) noexcept;
constexpr std::uint32_t rt( std::uint32_t word ) noexcept;
constexpr std::uint32_t rd( std::uint32_t word ) noexcept;
constexpr std::uint32_t shamt( std::uint32_t word ) noexcept;
constexpr std::uint32_t function( std::uint32_t word ) noexcept;
constexpr std::uint32_t immediate( std::uint32_t word ) noexcept;

template <int type>
constexpr std::uint32_t sign_extend( std::uint32_t imm ) noexcept;

constexpr int _load{ 0x1234 };
constexpr int _store{ -_load };

constexpr int _zero_extend{ 0xABCD };
constexpr int _sign_extend{ -_zero_extend };

constexpr int _byte{ 0x9876 };
constexpr int _halfword{ -_byte };

CPU::CPU( RAM &ram ) noexcept : string_handler( ram ), mmu( ram, fixed_mapping_segments ) {}

IODevice * CPU::attach_iodevice( IODevice * device ) noexcept
{
  auto * old = io_device;
  io_device = device;
  return old;
}

FileHandler * CPU::attach_file_handler( FileHandler * handler ) noexcept
{
  auto * old = file_handler;
  file_handler = handler;
  return old;
}

std::uint32_t CPU::start() noexcept
{
  exit_code.store( NONE, std::memory_order_release );

  while ( exit_code.load( std::memory_order_acquire ) == NONE )
  {
    auto const *const word = mmu.access( pc, running_mode() );

    // fetch
    if ( pc & 0b11 || !word )
    {
      signal_exception( ExCause::AdEL, word ? *word : 0, pc ); // word can be nullptr
      continue;
    }

    // execute
    pc += 4;
    ( this->*function_table[opcode( *word )] )( *word );

    gpr[0] = 0;
  }

  return exit_code.load( std::memory_order_acquire );
}

void CPU::stop() noexcept
{
  exit_code.store( MANUAL_STOP, std::memory_order_release );
}

std::uint32_t CPU::single_step() noexcept
{
  exit_code.store( NONE, std::memory_order_release );

  auto * word = mmu.access( pc, running_mode() );

  if ( pc & 0b11 || !word ) // fetch
  {
    signal_exception( ExCause::AdEL, 0, pc );
  }
  else // execute
  {
    pc += 4;
    ( this->*function_table[opcode( *word )] )( *word );

    gpr[0] = 0;
  }

  return exit_code.load( std::memory_order_acquire );
}

void CPU::hard_reset() noexcept
{
  gpr[0] = 0;
  cp0.reset();
  cp1.reset();
  enter_kernel_mode();
  pc = 0xBFC0'0000;
}

constexpr std::uint32_t opcode( std::uint32_t word ) noexcept
{
  return word >> 26;
}
constexpr std::uint32_t rs( std::uint32_t word ) noexcept
{
  return word >> 21 & 0x1F;
}
constexpr std::uint32_t rt( std::uint32_t word ) noexcept
{
  return word >> 16 & 0x1F;
}
constexpr std::uint32_t rd( std::uint32_t word ) noexcept
{
  return word >> 11 & 0x1F;
}
constexpr std::uint32_t shamt( std::uint32_t word ) noexcept
{
  return word >> 6 & 0x1F;
}
constexpr std::uint32_t function( std::uint32_t word ) noexcept
{
  return word & 0x3F;
}
constexpr std::uint32_t immediate( std::uint32_t word ) noexcept
{
  return word & 0xFFFF;
}

template <int type>
constexpr std::uint32_t sign_extend( std::uint32_t imm ) noexcept
{
  static_assert( type == _byte || type == _halfword, "Invalid type! Use '_byte' or '_halfword'." );

  if constexpr ( type == _byte )
    return imm & 0x80 ? imm | 0xFFFF'FF00 : imm;
  else
    return imm & 0x8000 ? imm | 0xFFFF'0000 : imm;
}

/* * * * * * * * *
 *               *
 * INSTRUCTIONS  *
 *               *
 * * * * * * * * */

void CPU::reserved( std::uint32_t word ) noexcept
{
  sigrie( word );
}
void CPU::special( std::uint32_t word ) noexcept
{
  static constexpr std::array<method_ptr, 64> special_fn_table{
      &CPU::sll,
      &CPU::reserved, // MOVCI
      &CPU::srl,
      &CPU::sra,
      &CPU::sllv,
      &CPU::lsa,
      &CPU::srlv,
      &CPU::srav,
      &CPU::reserved, // JR
      &CPU::jalr,
      &CPU::reserved, // MOVZ
      &CPU::reserved, // MOVN
      &CPU::syscall,
      &CPU::break_,
      &CPU::reserved, // SDBBP
      &CPU::reserved, // SYNC
      &CPU::clz,
      &CPU::clo,
      &CPU::reserved, // MFLO
      &CPU::reserved, // MTLO
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::sop30,
      &CPU::sop31,
      &CPU::sop32,
      &CPU::sop33,
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::add,
      &CPU::addu,
      &CPU::sub,
      &CPU::subu,
      &CPU::and_,
      &CPU::or_,
      &CPU::xor_,
      &CPU::nor_,
      &CPU::reserved, // *
      &CPU::reserved, // *
      &CPU::slt,
      &CPU::sltu,
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::tge,
      &CPU::tgeu,
      &CPU::tlt,
      &CPU::tltu,
      &CPU::teq,
      &CPU::seleqz,
      &CPU::tne,
      &CPU::selnez,
      &CPU::reserved, // beta
      &CPU::reserved, // *
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // *
      &CPU::reserved, // beta
      &CPU::reserved, // beta
  };

  ( this->*special_fn_table[function( word )] )( word );
}
void CPU::regimm( std::uint32_t word ) noexcept
{
  switch ( rt( word ) )
  {
  case 0b00'000: bltz( word ); break;
  case 0b00'001: bgez( word ); break;
  case 0b10'000: nal( word ); break;
  case 0b10'001: bal( word ); break;
  case 0b10'111: sigrie( word ); break;
  default: reserved( word ); break;
  }
}
void CPU::special3( std::uint32_t word ) noexcept
{
  auto fn = function( word );

  if ( fn == 0b000'000 )
    ext( word );
  else if ( fn == 0b000'100 )
    ins( word );
  else
    reserved( word );
}
void CPU::cop0( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );

  if ( _rs & 0x10 ) // if the 5th bit is set, then we decode C0
  {
    auto _fn = function( word );

    if ( _fn == 0b011'000 )
      eret( word );
    else
      reserved( word );
  }
  else
  {
    switch ( _rs )
    {
    case 0b00'000: mfc0( word ); break;
    case 0b00'010: mfhc0( word ); break;
    case 0b00'100: mtc0( word ); break;
    case 0b00'110: mthc0( word ); break;
    case 0b01'011: mfmc0( word ); break;

    default: reserved( word ); break;
    }
  }
}
void CPU::pcrel( std::uint32_t word ) noexcept
{
  auto fn_opcode = word >> 16 & 0x1F;

  constexpr void ( CPU::*fn[] )( std::uint32_t ) noexcept =
  {
    &CPU::addiupc,
    &CPU::lwpc,
    &CPU::lwupc,
    &CPU::reserved, // LDPC
  };

  switch ( fn_opcode )
  {
  case 0b111'00:
  case 0b111'01: reserved( word ); break;

  case 0b111'10: auipc( word ); break;

  case 0b111'11: aluipc( word ); break;

  default: ( this->*fn[fn_opcode >> 3] )( word );
  }
  /* DEPRECATED
  if ( fn_opcode == 0b111'00 || fn_opcode == 0b111'01 )
    return reserved( word );

  if ( fn_opcode == 0b111'10 )
    return auipc( word );

  if ( fn_opcode == 0b111'11 )
    return aluipc( word );

  switch ( fn_opcode >> 3 )
  {
  case 0: addiupc( word ); break;
  case 1: lwpc( word ); break;
  case 2: lwupc( word ); break;
  case 3: reserved( word ); break; // LDPC
  }
  */
}
void CPU::cop1( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t MFC1{ 0b00'000 };
  constexpr std::uint32_t MFHC1{ 0b00'011 };
  constexpr std::uint32_t MTC1{ 0b00'100 };
  constexpr std::uint32_t MTHC1{ 0b00'111 };

  auto _ft = rd( word );
  auto _rt = rt( word );
  auto _type = rs( word );

  if ( _type == MFC1 )
  {
    gpr[_rt] = cp1.mfc1( _ft );
  }
  else if ( _type == MFHC1 )
  {
    gpr[_rt] = cp1.mfhc1( _ft );
  }
  else if ( _type == MTC1 )
  {
    cp1.mtc1( _ft, gpr[_rt] );
  }
  else if ( _type == MTHC1 )
  {
    cp1.mthc1( _ft, gpr[_rt] );
  }
  else
  {
    auto ex = cp1.execute( word );
    if ( ex != CP1::Exception::NONE )
      signal_exception( ExCause::FPE, word, pc - 4 );
  }
}

void CPU::j( std::uint32_t word ) noexcept
{
  pc = pc & 0xF000'0000 | word << 6 >> 4;
}

void CPU::jal( std::uint32_t word ) noexcept
{
  gpr[31] = pc + 4;
  pc = pc & 0xF000'0000 | word << 6 >> 4;
}

void CPU::beq( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  if ( gpr[_rs] == gpr[_rt] )
    pc += _imm;
}

void CPU::bne( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  if ( gpr[_rs] != gpr[_rt] )
    pc += _imm;
}

void CPU::pop06( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  bool cond = false;

  gpr[31] = pc;

  if ( _rs == 0 && _rt != 0 ) // BLEZALC
  {
    std::int32_t t = gpr[_rt];
    cond = t <= 0;
  }
  else if ( _rs == _rt && _rt != 0 ) // BGEZALC
  {
    std::int32_t t = gpr[_rt];
    cond = t >= 0;
  }
  else if ( _rs != _rt && _rs != 0 && _rt != 0 ) // BGEUC
  {
    cond = gpr[_rs] >= gpr[_rt];
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;
    pc += _imm;
  }
}
void CPU::pop07( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  bool cond = false;

  gpr[31] = pc;

  if ( _rs == 0 && _rt != 0 ) // BGTZALC
  {
    std::int32_t t = gpr[_rt];
    cond = t > 0;
  }
  else if ( _rs == _rt && _rt != 0 ) // BLTZALC
  {
    std::int32_t t = gpr[_rt];
    cond = t < 0;
  }
  else if ( _rs != _rt && _rs != 0 && _rt != 0 ) // BLTUC
  {
    cond = gpr[_rs] < gpr[_rt];
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;
    pc += _imm;
  }
}
void CPU::pop10( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  bool cond = false;

  gpr[31] = pc;

  if ( _rs == 0 && _rt != 0 ) // BEQZALC
  {
    cond = gpr[_rt] == 0;
  }
  else if ( _rs < _rt && _rs != 0 && _rt != 0 ) // BEQC
  {
    cond = gpr[_rs] == gpr[_rt];
  }
  else if ( _rs >= _rt ) // BOVC
  {
    std::uint64_t s = gpr[_rs];
    std::uint64_t t = gpr[_rt];

    cond = ( s + t ) & 0x1'0000'0000;
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;
    pc += _imm;
  }
}

void CPU::addiu( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) );

  gpr[_rt] = gpr[_rs] + _imm;
}

void CPU::slti( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::int32_t _imm = sign_extend<_halfword>( immediate( word ) );
  std::int32_t s = gpr[_rs];

  gpr[_rt] = s < _imm;
}

void CPU::sltiu( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) );

  gpr[_rt] = gpr[_rs] < _imm;
}

void CPU::andi( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rt] = gpr[_rs] & immediate( word );
}

void CPU::ori( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rt] = gpr[_rs] | immediate( word );
}

void CPU::xori( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rt] = gpr[_rs] ^ immediate( word );
}

void CPU::aui( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rt] = gpr[_rs] + ( immediate( word ) << 16 );
}

void CPU::pop26( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  bool cond = false;

  if ( _rs == 0 && _rt != 0 ) // BLEZC
  {
    std::int32_t t = gpr[_rt];

    cond = t <= 0;
  }
  else if ( _rs == _rt && _rt != 0 ) // BGEZC
  {
    std::int32_t t = gpr[_rt];

    cond = t >= 0;
  }
  else if ( _rs != _rt && _rs != 0 && _rt != 0 ) // BGEC
  {
    std::int32_t s = gpr[_rs];
    std::int32_t t = gpr[_rt];

    cond = s >= t;
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    gpr[31] = pc;
    pc += _imm;
  }
}
void CPU::pop27( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  bool cond = false;

  if ( _rs == 0 && _rt != 0 ) // BGTZC
  {
    cond = ( std::int32_t )gpr[_rt] > 0;
  }
  else if ( _rs == _rt && _rt != 0 ) // BLTZC
  {
    cond = ( std::int32_t )gpr[_rt] < 0;
  }
  else if ( _rs != _rt && _rs != 0 && _rt != 0 ) // BLTC
  {
    cond = ( std::int32_t )gpr[_rs] < ( std::int32_t )gpr[_rt];
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    gpr[31] = pc;
    pc += _imm;
  }
}
void CPU::pop30( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  bool cond = false;

  if ( _rs < _rt && _rs != 0 && _rt != 0 ) // BNEC
  {
    cond = gpr[_rs] != gpr[_rt];
  }
  else if ( _rs < _rt && _rt != 0 ) // BNEZALC
  {
    gpr[31] = pc;
    cond = gpr[_rt] != 0;
  }
  else if ( _rs >= _rt ) // BNVC
  {
    std::uint64_t s = gpr[_rs];
    std::uint64_t t = gpr[_rt];

    cond = !( ( s + t ) & 0x1'0000'0000 );
  }
  else
  {
    reserved( word );
    return;
  }

  if ( cond )
  {
    pc += _imm;
  }
}

/* * * * * * *
 *           *
 *  SPECIAL3 *
 *           *
 * * * * * * */
void CPU::ext( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _size = rd( word ) + 1;
  auto _pos = shamt( word );

  auto left_shift = 32 - ( _pos + _size );
  auto right_shift = left_shift + _pos;

  gpr[_rt] = gpr[_rs] << left_shift >> right_shift;
}
void CPU::ins( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );
  auto _pos = shamt( word );
  auto _size = rd( word ) + 1 - _pos;

  auto mask = 0xFFFF'FFFF << ( 32 - _size ) >> ( 32 - _size );

  gpr[_rt] = ( gpr[_rt] & ~( mask << _pos ) ) | ( gpr[_rs] & mask ) << _pos;
}

/**
 * Load/Store Byte, always aligned.
 *
 * `address & 0b11` gives us which byte to load/store:
 * 0: 1st, 0x0000'00FF
 * 1: 2nd, 0x0000'FF00
 * 2: 3rd, 0x00FF'0000
 * 3: 4th, 0xFF00'0000
 *
 * load:
 * GPR = memword >> shift_align & 0xFF;
 *
 * store:
 * memword = memword & mask_align | GPR << shift_align
 **/
template <int op, int extend>
void CPU::op_byte( std::uint32_t word ) noexcept
{
  static_assert( op == _load || op == _store, "Invalid operation! Use '_load' or '_store'." );
  static_assert( extend == _sign_extend || extend == _zero_extend, "Invalid extension policy! Use '_sign_extend' or '_zero_extend'." );
  static_assert( op == _load || ( op == _store && extend == _zero_extend ), "Invalid combination! Sign extensions is invalid with a Store operation." );

  auto _base = rs( word );
  auto _rt = rt( word );

  if constexpr ( op == _load )
  {
    if ( _rt == 0 )
      return;
  }

  auto address = gpr[_base] + sign_extend<_halfword>( immediate( word ) );
  auto align = address & 0b11;

  std::uint32_t byte;

  constexpr std::uint32_t shift_align[] = { 0, 8, 16, 24 };

  if constexpr ( op == _load )
  {
    auto *load_byte = mmu.access( address, running_mode() );

    if ( !load_byte )
    {
      signal_exception( ExCause::AdEL, word, pc - 4 );
      return;
    }

    byte = *load_byte;
  }
  else // store
  {
    byte = gpr[_rt];
  }

  if constexpr ( op == _load )
  {
    byte = byte >> shift_align[align] & 0xFF;

    if constexpr ( extend == _sign_extend )
      byte = sign_extend<_byte>( byte );

    gpr[_rt] = byte;
  }
  else // store
  {
    constexpr std::uint32_t mask_align[] = {
        0xFFFF'FF00,
        0xFFFF'00FF,
        0xFF00'FFFF,
        0x00FF'FFFF,
    };

    auto *store_byte = mmu.access( address, running_mode() );

    if ( !store_byte )
    {
      signal_exception( ExCause::AdES, word, pc - 4 );
      return;
    }

    *store_byte = *store_byte & mask_align[align] | byte << shift_align[align];
  }
}

/**
 * Load/Store Halfword, aligned if address is a multiple of 2.
 *
 * `address & 0b11` gives us the alignment we are working with:
 * 0: aligned,   1 load/store
 *                             0x0000'FFFF
 * 1: unaligned, 1 load/store
 *                             0x00FF'FF00
 * 2: unaligned, 1 load/store
 *                             0xFFFF'0000
 * 3: unaligned, 2 load/store
 *               0x0000'00FF | 0xFF00'0000
 *
 *                    ^             ^
 *               address + 4 |   address
 *
 * load:
 * GPR = high_half << shift_align | low_half >> shift_align & mask_align
 *
 * store:
 * mem[addr] = mem[addr] & mask_align | low_half
 * mem[addr+4] = mem[addr+4] & mask_align | high_half <-- only if `align == 3`
 **/
template <int op, int extend>
void CPU::op_halfword( std::uint32_t word ) noexcept
{
  static_assert( op == _load || op == _store, "Invalid operation! Use '_load' or '_store'." );
  static_assert( extend == _sign_extend || extend == _zero_extend, "Invalid extension policy! Use '_sign_extend' or '_zero_extend'." );
  static_assert( op == _load || ( op == _store && extend == _zero_extend ), "Invalid combination! Sign extensions is invalid with a Store operation." );

  auto _base = rs( word );
  auto _rt = rt( word );

  if constexpr ( op == _load )
  {
    if ( _rt == 0 )
      return;
  }

  auto address = gpr[_base] + sign_extend<_halfword>( immediate( word ) );

  auto align = address & 0b11;

  std::uint32_t lowhalf_value;
  std::uint32_t highhalf_value = 0;

  constexpr std::uint32_t shift_align[] = { 0, 8, 16 };

  if constexpr ( op == _load )
  {
    auto *lowhalf_ptr = mmu.access( address, running_mode() );
    if ( !lowhalf_ptr )
    {
      signal_exception( ExCause::AdEL, word, pc - 4 );
      return;
    }

    lowhalf_value = *lowhalf_ptr;

    if ( align == 3 )
    {
      if ( address > 0xFFFF'FFFB )
      {
        signal_exception( ExCause::DBE, word, pc - 4 );
        return;
      }

      auto *highhalf_ptr = mmu.access( address + 4, running_mode() );
      if ( !highhalf_ptr )
      {
        signal_exception( ExCause::AdEL, word, pc - 4 );
        return;
      }

      highhalf_value = *highhalf_ptr << 8 & 0xFF00;
      lowhalf_value = lowhalf_value >> 24 & 0x00FF;
    }
    else
    {
      lowhalf_value = lowhalf_value >> shift_align[align] & 0xFFFF;
    }

    if constexpr ( extend == _sign_extend )
      gpr[_rt] = sign_extend<_halfword>( highhalf_value | lowhalf_value );
    else
      gpr[_rt] = highhalf_value | lowhalf_value;
  }
  else // store
  {
    constexpr std::uint32_t mask_align[] = {
        0xFFFF'0000,
        0xFF00'00FF,
        0x0000'FFFF,
    };

    lowhalf_value = gpr[_rt] & 0xFFFF;

    auto *lowhalf_ptr = mmu.access( address, running_mode() );
    if ( !lowhalf_ptr )
    {
      signal_exception( ExCause::AdES, word, pc - 4 );
      return;
    }

    if ( align == 3 )
    {
      if ( address > 0xFFFF'FFFB )
      {
        signal_exception( ExCause::DBE, word, pc - 4 );
        return;
      }
      auto *highhalf_ptr = mmu.access( address + 4, running_mode() );
      if ( !highhalf_ptr )
      {
        signal_exception( ExCause::AdES, word, pc - 4 );
        return;
      }

      *lowhalf_ptr = *lowhalf_ptr & 0x00FF'FFFF | lowhalf_value << 24;
      *highhalf_ptr = *highhalf_ptr & ~0xFF | lowhalf_value >> 8;
    }
    else
    {
      *lowhalf_ptr = *lowhalf_ptr & mask_align[align] | lowhalf_value << shift_align[align];
    }
  }
}

/**
 * Load/Store Word, aligned if address is a multiple of 4.
 *
 * `address & 0b11` gives us the alignment we are working with:
 * 0: aligned,   1 load/store
 *                             0xFFFF'FFFF
 * 1: unaligned, 2 load/store
 *               0x0000'00FF | 0xFFFF'FF00
 * 2: unaligned, 2 load/store
 *               0x0000'FFFF | 0xFFFF'0000
 * 3: unaligned, 2 load/store
 *               0x00FF'FFFF | 0xFF00'0000
 *
 *                     ^             ^
 *               address + 4 |    address
 **/
template <int op>
void CPU::op_word( std::uint32_t word ) noexcept
{
  auto _base = rs( word );
  auto _rt = rt( word );
  auto address = gpr[_base] + sign_extend<_halfword>( immediate( word ) );

  if constexpr ( op == _load )
  {
    if ( _rt == 0 )
      return;
  }

  op_word<op>( _rt, address, word );
}

template <int op>
void CPU::op_word( std::uint32_t _rt, std::uint32_t address, std::uint32_t _word ) noexcept
{
  static_assert( op == _load || op == _store, "Invalid operation! Use '_load' or '_store'." );

  auto align = address & 0b11;

  if ( align == 0 )
  {
    auto *word = mmu.access( address, running_mode() );

    if constexpr ( op == _load )
    {
      if ( !word )
      {
        signal_exception( ExCause::AdEL, _word, pc - 4 );
        return;
      }
      gpr[_rt] = *word;
    }
    else // store
    {
      if ( !word )
      {
        signal_exception( ExCause::AdES, _word, pc - 4 );
        return;
      }
      *word = gpr[_rt];
    }
  }
  else // unaligned
  {
    if ( address > 0xFFFF'FFFB )
    {
      signal_exception( ExCause::DBE, _word, pc - 4 );
      return;
    }

    auto *low = mmu.access( address, running_mode() );
    auto *high = mmu.access( address + 4, running_mode() );

    if constexpr ( op == _load )
    {
      if ( !low || !high )
      {
        signal_exception( ExCause::AdEL, _word, pc - 4 );
        return;
      }
      auto low_word = *low;
      auto high_word = *high;

      switch ( align )
      {
      case 3: low_word >>= 8;
      case 2: low_word >>= 8;
      case 1: low_word >>= 8;
      }

      switch ( align )
      {
      case 1: high_word <<= 8;
      case 2: high_word <<= 8;
      case 3: high_word <<= 8;
      }

      gpr[_rt] = high_word | low_word;
    }
    else // store
    {
      if ( !low || !high )
      {
        signal_exception( ExCause::AdES, _word, pc - 4 );
        return;
      }
      if ( align == 1 )
      {
        *low = *low & 0xFF | gpr[_rt] << 8;
        *high = *high & ~0xFF | gpr[_rt] >> 24;
      }
      else if ( align == 2 )
      {
        *low = *low & 0xFFFF | gpr[_rt] << 16;
        *high = *high & ~0xFFFF | gpr[_rt] >> 16;
      }
      else
      {
        *low = *low & 0x00FF'FFFF | gpr[_rt] << 24;
        *high = *high & 0xFF00'0000 | gpr[_rt] >> 8;
      }
    }
  }
}

void CPU::lb( std::uint32_t word ) noexcept
{
  op_byte<_load, _sign_extend>( word );
}

void CPU::lh( std::uint32_t word ) noexcept
{
  op_halfword<_load, _sign_extend>( word );
}

void CPU::lw( std::uint32_t word ) noexcept
{
  op_word<_load>( word );
}

void CPU::lbu( std::uint32_t word ) noexcept
{
  op_byte<_load, _zero_extend>( word );
}

void CPU::lhu( std::uint32_t word ) noexcept
{
  op_halfword<_load, _zero_extend>( word );
}

void CPU::sb( std::uint32_t word ) noexcept
{
  op_byte<_store, _zero_extend>( word );
}
void CPU::sh( std::uint32_t word ) noexcept
{
  op_halfword<_store, _zero_extend>( word );
}
void CPU::sw( std::uint32_t word ) noexcept
{
  op_word<_store>( word );
}

void CPU::lwc1( std::uint32_t word ) noexcept
{
  auto ft = rt( word );
  auto _base = rs( word );
  auto _imm = immediate( word );
  auto address = gpr[_base] + sign_extend<_halfword>( _imm );

  op_word<_load>( 0, address, word );
  cp1.mtc1( ft, gpr[0] );

  gpr[0] = 0;
}

void CPU::bc( std::uint32_t word ) noexcept
{
  auto target_offset = word & 0x03FF'FFFF;

  if ( target_offset & 1 << 25 ) // sign extend
    target_offset = 0xF000'0000 | target_offset << 2;
  else
    target_offset <<= 2;

  pc += target_offset;
}

void CPU::ldc1( std::uint32_t word ) noexcept
{
  auto ft = rt( word );
  auto _base = rs( word );
  auto _immediate = immediate( word );
  auto address = gpr[_base] + sign_extend<_halfword>( _immediate );

  auto copy_1 = gpr[1];
  auto copy_2 = gpr[2];

  op_word<_load>( 1, address, word );
  op_word<_load>( 2, address + 4, word );
  cp1.mtc1( ft, gpr[1] );
  cp1.mthc1( ft, gpr[2] );

  gpr[1] = copy_1;
  gpr[2] = copy_2;
}

void CPU::pop66( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( _rs != 0 ) // BEQZC
  {
    if ( gpr[_rs] != 0 )
      return;

    auto _imm = word & 0x1F'FFFF; // 21 bits

    if ( _imm & 1 << 21 ) // sign extend
      _imm |= 0xFFE0'0000;

    pc += _imm << 2;
  }
  else // JIC
  {
    auto _imm = sign_extend<_halfword>( immediate( word ) );
    pc = gpr[_rt] + _imm;
  }
}

void CPU::swc1( std::uint32_t word ) noexcept
{
  auto ft = rt( word );
  auto _base = rs( word );
  auto _immediate = immediate( word );
  auto address = gpr[_base] + sign_extend<_halfword>( _immediate );

  gpr[0] = cp1.mfc1( ft );

  op_word<_store>( 0, address, word );

  gpr[0] = 0;
}

void CPU::balc( std::uint32_t word ) noexcept
{
  auto target_offset = word & 0x03FF'FFFF;

  if ( target_offset & 1 << 25 ) // sign extend
    target_offset = 0xF000'0000 | target_offset << 2;
  else
    target_offset <<= 2;

  gpr[31] = pc;
  pc += target_offset;
}

void CPU::sdc1( std::uint32_t word ) noexcept
{
  auto ft = rt( word );
  auto _base = rs( word );
  auto _immediate = immediate( word );
  auto address = gpr[_base] + sign_extend<_halfword>( _immediate );

  gpr[0] = cp1.mfc1( ft );
  op_word<_store>( 0, address, word );

  gpr[0] = cp1.mfhc1( ft );
  op_word<_store>( 0, address + 4, word );

  gpr[0] = 0;
}
void CPU::pop76( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( _rs != 0 ) // BNEZC
  {
    if ( gpr[_rs] != 0 )
    {
      auto _imm = word & 0x1F'FFFF; // 21 bits

      if ( _imm & 1 << 21 ) // sign extend
        _imm |= 0xFFE0'0000;

      gpr[31] = pc;
      pc += _imm << 2;
    }
  }
  else // JIALC
  {
    gpr[31] = pc;
    pc = gpr[_rt] + sign_extend<_halfword>( immediate( word ) );
  }
}

/* * * * * *
 *         *
 * SPECIAL *
 *         *
 * * * * * */
void CPU::sll( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _shamt = shamt( word );

  gpr[_rd] = gpr[_rt] << _shamt;
}
void CPU::srl( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _shamt = shamt( word );

  if ( word & 1 << 21 ) // ROTR
  {
    gpr[_rd] = gpr[_rt] >> _shamt | gpr[_rt] << ( 32 - _shamt );
  }
  else // SRL
  {
    gpr[_rd] = gpr[_rt] >> _shamt;
  }
}
void CPU::sra( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _shamt = shamt( word );

  if ( gpr[_rt] & 0x8000'0000 ) // sign propagation
    gpr[_rd] = gpr[_rt] >> _shamt | ( std::uint32_t )0xFFFF'FFFF << ( 32 - _shamt );
  else
    gpr[_rd] = gpr[_rt] >> _shamt;
}
void CPU::sllv( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _rs = rs( word );

  gpr[_rd] = gpr[_rt] << gpr[_rs];
}
void CPU::lsa( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _rs = rs( word );
  auto _shamt = shamt( word ) + 1;

  gpr[_rd] = ( gpr[_rs] << _shamt ) + gpr[_rt];
}
void CPU::srlv( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _rs = rs( word );

  if ( word & 1 << 6 ) // ROTRV
  {
    gpr[_rd] = gpr[_rt] >> gpr[_rs] | gpr[_rt] << ( 32 - gpr[_rs] );
  }
  else // SRLV
  {
    gpr[_rd] = gpr[_rt] >> gpr[_rs];
  }
}
void CPU::srav( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _rs = rs( word );

  if ( gpr[_rt] & 0x8000'0000 )
    gpr[_rd] = gpr[_rt] >> gpr[_rs] | ( std::uint32_t )0xFFFF'FFFF << ( 32 - gpr[_rs] );
  else
    gpr[_rd] = gpr[_rt] >> gpr[_rs];
}
void CPU::jalr( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );

  gpr[_rd] = pc + 4;

  pc = gpr[_rs];
}
void CPU::syscall( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t v0{ 2 };
  constexpr std::uint32_t v1{ 3 };

  constexpr std::uint32_t a0{ 4 };
  constexpr std::uint32_t a1{ 5 };
  constexpr std::uint32_t a2{ 6 };

  auto const sysnum = gpr[v0];

  if ( sysnum == 0 || sysnum > 17 )
  {
    signal_exception( ExCause::Sys, word, pc - 4 );
    return;
  }

  if ( sysnum == 1 ) // print int
  {
    io_device->print_integer( gpr[a0] );
  }
  else if ( sysnum == 2 ) // print float
  {
    union
    {
      std::uint32_t i32;
      float         f;
    };

    i32 = cp1.mfc1( 12 );

    io_device->print_float( f ); // $f12
  }
  else if ( sysnum == 3 ) // print double
  {
    union
    {
      double        d;
      std::uint64_t i64;
    };

    i64 = ( std::uint64_t )cp1.mfc1( 12 );
    i64 |= std::uint64_t( cp1.mfhc1( 12 ) ) << 32;

    io_device->print_double( d );
  }
  else if ( sysnum == 4 ) // print string
  {
    auto address = gpr[a0];
    auto str = string_handler.read( address, 0xFFFF'FFFF, true);
    io_device->print_string( str.data() );
  }
  else if ( sysnum == 5 ) // read int
  {
    io_device->read_integer( gpr.data() + v0 );
  }
  else if ( sysnum == 6 ) // read float
  {
    union
    {
      float         f;
      std::uint32_t i32;
    };

    io_device->read_float( &f );

    cp1.mtc1( 0, i32 ); // $f0
  }
  else if ( sysnum == 7 ) // read double
  {
    union
    {
      double          d;
      std::uint64_t i64;
    };

    io_device->read_double( &d );

    cp1.mtc1( 0, i64 & 0xFFFF'FFFF ); // $f0 low
    cp1.mthc1( 0, i64 >> 32 );        // $f0 high
  }
  else if ( sysnum == 8 ) // read string
  {
    auto address = gpr[a0];
    auto length = gpr[a1];

    // Allocate enough space
    std::unique_ptr<char[]> buf( new( std::nothrow ) char[length] );

    // Read the string from the device
    io_device->read_string( buf.get(), length );

    // Write the string into the RAM
    string_handler.write( address, buf.get(), length );
  }
  else if ( sysnum == 9 ) // sbrk
  {
    signal_exception( ExCause::Int, word, pc - 4 );
  }
  else if ( sysnum == 10 || sysnum == 17 ) // exit
  {
    exit_code.store( EXIT, std::memory_order_release );
  }
  else if ( sysnum == 11 ) // print char
  {
    char str[2] = {
        ( char )( gpr[a0] & 0xFF ),
        '\0',
    };

    io_device->print_string( str );
  }
  else if ( sysnum == 12 ) // read char
  {
    char c;

    io_device->read_string( &c, 1 );

    gpr[v0] = ( std::uint32_t )c;
  }
  else if ( sysnum == 13 ) // file open
  {
    auto filename_address = gpr[a0];

    auto filename = string_handler.read( filename_address, 0xFFFF'FFFF, true );
    char flags[5] = { 0 }; // flags must be null terminated, but $a1 can contain up to 4 chars without '\0'.

    std::memcpy( flags, &gpr[a1], 4 );

    gpr[v0] = file_handler->open( filename.data(), flags );
  }
  else if ( sysnum == 14 ) // file read
  {
    auto fd = gpr[a0];
    auto buf = gpr[a1];
    auto count = gpr[a2];

    // Allocate enough space
    std::unique_ptr<char[]> data( new char[count] );

    // Read the data from file and store the result
    gpr[v0] = file_handler->read( fd, data.get(), count );

    // Write the data into the RAM
    string_handler.write( buf, data.get(), count );
  }
  else if ( sysnum == 15 ) // file write
  {
    auto fd = gpr[a0];
    auto buf = gpr[a1];
    auto count = gpr[a2];

    auto data = string_handler.read( buf, count );

    gpr[v0] = file_handler->write( fd, data.data(), count );
  }
  else if ( sysnum == 16 ) // file close
  {
    auto fd = gpr[a0];

    file_handler->close( fd );

    gpr[v0] = 0;
  }
  else
  {
    signal_exception( ExCause::Sys, word, pc - 4 );
  }
}
void CPU::break_( std::uint32_t ) noexcept
{
  set_ex_cause( ExCause::Bp );
  exit_code.store( EXCEPTION, std::memory_order_release );
}
void CPU::clz( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );

  if ( _rd == 0 )
    return;

  std::uint32_t count = 0;

  auto num = gpr[_rs];

  if ( num == 0 )
    count = 32;
  else
    while ( !( num & 0x8000'0000 ) )
    {
      ++count;
      num <<= 1;
    }

  gpr[_rd] = count;
}
void CPU::clo( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );

  if ( _rd == 0 )
    return;

  std::uint32_t count = 0;

  auto num = gpr[_rs];

  while ( num & 0x8000'0000 )
  {
    ++count;
    num <<= 1;
  }

  gpr[_rd] = count;
}
void CPU::sop30( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t MUL{ 0b00010 };
  constexpr std::uint32_t MUH{ 0b00011 };

  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  auto _fn = shamt( word );

  if ( _fn == MUL )
  {
    std::int32_t s = gpr[_rs];
    std::int32_t t = gpr[_rt];

    gpr[_rd] = s * t;
  }
  else if ( _fn == MUH )
  {
    auto s = ( std::int64_t )gpr[_rs] & 0xFFFF'FFFF;
    auto t = ( std::int64_t )gpr[_rt] & 0xFFFF'FFFF;

    gpr[_rd] = ( s*t ) >> 32;
  }
  else
  {
    reserved( word );
  }
}
void CPU::sop31( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t MULU{ 0b00010 };
  constexpr std::uint32_t MUHU{ 0b00011 };

  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  auto _fn = shamt( word );

  if ( _fn == MULU )
  {
    gpr[_rd] = gpr[_rs] * gpr[_rt];
  }
  else if ( _fn == MUHU )
  {
    std::uint64_t s = gpr[_rs];
    std::uint64_t t = gpr[_rt];

    gpr[_rd] = ( s * t ) >> 32;
  }
  else
  {
    reserved( word );
  }
}
void CPU::sop32( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t _DIV{ 0b00010 };
  constexpr std::uint32_t _MOD{ 0b00011 };

  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  auto _fn = shamt( word );

  std::int32_t s = gpr[_rs];
  std::int32_t t = gpr[_rt];

  // Div By Zero is UNPREDICTABLE. UNPREDICTABLE operations may cause a result to be generated or not.

  if ( _fn == _DIV )
  {
    if ( t != 0 )
      gpr[_rd] = s / t;
  }
  else if ( _fn == _MOD )
  {
    if ( t != 0 )
      gpr[_rd] = s % t;
  }
  else
  {
    reserved( word );
  }
}
void CPU::sop33( std::uint32_t word ) noexcept
{
  constexpr std::uint32_t _DIVU{ 0b00010 };
  constexpr std::uint32_t _MODU{ 0b00011 };

  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  auto _fn = shamt( word );

  std::uint32_t s = gpr[_rs];
  std::uint32_t t = gpr[_rt];

  // Div By Zero is UNPREDICTABLE. UNPREDICTABLE operations may cause a result to be generated or not.

  if ( _fn == _DIVU )
  {
    if ( t != 0 )
      gpr[_rd] = s / t;
  }
  else if ( _fn == _MODU )
  {
    if ( t != 0 )
      gpr[_rd] = s % t;
  }
  else
  {
    reserved( word );
  }
}
void CPU::add( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::uint64_t s = gpr[_rs];
  std::uint64_t t = gpr[_rt];

  auto res = s + t;

  if ( res & 0x1'0000'0000 )
    signal_exception( ExCause::Ov, word, pc - 4 );
  else
    gpr[_rd] = (std::uint32_t)res;
}
void CPU::addu( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] + gpr[_rt];
}
void CPU::sub( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::uint64_t s = gpr[_rs];
  std::uint64_t t = gpr[_rt];

  auto res = s - t;

  if ( res & 0x1'0000'0000 )
    signal_exception( ExCause::Ov, word, pc - 4 );
  else
    gpr[_rd] = (std::uint32_t)res;
}
void CPU::subu( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] - gpr[_rt];
}
void CPU::and_( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] & gpr[_rt];
}
void CPU::or_( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] | gpr[_rt];
}
void CPU::xor_( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] ^ gpr[_rt];
}
void CPU::nor_( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = ~( gpr[_rs] | gpr[_rt] );
}
void CPU::slt( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::int32_t s = gpr[_rs];
  std::int32_t t = gpr[_rt];

  gpr[_rd] = s < t;
}
void CPU::sltu( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rs] < gpr[_rt];
}
void CPU::tge( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::int32_t s = gpr[_rs];
  std::int32_t t = gpr[_rt];

  if ( s >= t )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::tgeu( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( gpr[_rs] >= gpr[_rt] )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::tlt( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  std::int32_t s = gpr[_rs];
  std::int32_t t = gpr[_rt];

  if ( s < t )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::tltu( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( gpr[_rs] < gpr[_rt] )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::teq( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( gpr[_rs] == gpr[_rt] )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::seleqz( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rt] ? 0 : gpr[_rs];
}
void CPU::tne( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _rt = rt( word );

  if ( gpr[_rs] != gpr[_rt] )
    signal_exception( ExCause::Tr, word, pc - 4 );
}
void CPU::selnez( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rs = rs( word );
  auto _rt = rt( word );

  gpr[_rd] = gpr[_rt] ? gpr[_rs] : 0;
}

/* * * * * *
 *         *
 * REGIMM  *
 *         *
 * * * * * */
void CPU::bltz( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  std::int32_t s = gpr[_rs];

  if ( s < 0 )
    pc += _imm;
}
void CPU::bgez( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  std::int32_t s = gpr[_rs];

  if ( s >= 0 )
    pc += _imm;
}
void CPU::nal( std::uint32_t ) noexcept
{
  gpr[31] = pc + 4;
}
void CPU::bal( std::uint32_t word ) noexcept
{
  gpr[31] = pc + 4;
  pc += sign_extend<_halfword>( immediate( word ) ) << 2;
}
void CPU::sigrie( std::uint32_t word ) noexcept
{
  signal_exception( ExCause::RI, word, pc - 4 );
}

/* * * * *
 *       *
 * COP0  *
 *       *
 * * * * */
void CPU::mfc0( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _sel = word & 0x7;

  gpr[_rt] = cp0.read( _rd, _sel );
}
void CPU::mfhc0( std::uint32_t word ) noexcept
{
  auto _rt = rt( word );

  gpr[_rt] = 0;
}
void CPU::mtc0( std::uint32_t word ) noexcept
{
  auto _rd = rd( word );
  auto _rt = rt( word );
  auto _sel = word & 0x7;

  cp0.write( _rd, _sel, gpr[_rt] );
}
void CPU::mthc0( std::uint32_t ) noexcept
{
  return;
}
void CPU::mfmc0( std::uint32_t word ) noexcept
{
  auto _rt = rt( word );

  auto enable_interrupt = word & 1 << 5;

  gpr[_rt] = cp0.status;

  if ( enable_interrupt )
    cp0.status |= 1;
  else
    cp0.status &= ~1;
}

void CPU::eret( std::uint32_t ) noexcept
{
  enter_user_mode();

  if ( cp0.status & 0b100 ) // Status ERL
    pc = cp0.error_epc;
  else
    pc = cp0.epc;

  cp0.status &= ~0b110;
}

/* * * * *
 *       *
 * PCREL *
 *       *
 * * * * */
void CPU::auipc( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = immediate( word ) << 16;

  gpr[_rs] = pc - 4 + _imm;
}
void CPU::aluipc( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = immediate( word ) << 16;

  gpr[_rs] = ~0xFFFF & pc + _imm;
}
void CPU::addiupc( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = sign_extend<_halfword>( immediate( word ) ) << 2;

  gpr[_rs] = pc - 4 + _imm;
}
void CPU::lwpc( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );

  auto address = ( word & 0x0007'FFFF ) << 2;

  if ( address & 0x0020'0000 ) // sign extend
    address |= 0xFFC0'0000;

  address += pc - 4;

  op_word<_load>( _rs, address, word );
}
void CPU::lwupc( std::uint32_t word ) noexcept
{
  auto _rs = rs( word );
  auto _imm = ( word & 0x0007'FFFF ) << 2;
  auto address = pc - 4 + _imm;

  op_word<_load>( _rs, address, word );
}

std::uint32_t CPU::running_mode() noexcept
{
  if ( cp0.status & 0x6 || !( cp0.status & 0x18 ) )
    return MMU::Segment::KERNEL;
  else
    return MMU::Segment::USER;
}
void CPU::enter_kernel_mode() noexcept
{
  /*
  ANY
    The KSU field in the CP0 Status register contains 0b00.
    The EXL bit in the Status register is 1.
    The ERL bit in the Status register is 1.
  */
  cp0.status &= 0x18; // Set KSU to 0b00
}
void CPU::enter_user_mode() noexcept
{
  /*
  ALL
    The KSU field in the Status register contains 0b10.
    The EXL and ERL bits in the Status register are both 0.
  */
  cp0.status |= 0x16;
}

void CPU::set_ex_cause( std::uint32_t ex ) noexcept
{
  cp0.cause = cp0.cause & ~0x7C | ex << 2;
}

/**
 * To handle Interrupts and Exceptions the CPU must
 * jump to the corresponding handler.
 *
 * 1. Check if it's an Interrupt or an Exception.
 *    1.1 If it's an Interrupt, check if Interrupts are enabled
 *    1.2 If Interrupts are enabled, check if we need to handle it
 * 2. If Exception, write EPC, BadInstr, Cause ExcCode.
 * 3. Enter kernel mode
 * 4. Jump to the handler.
 **/
void CPU::signal_exception( std::uint32_t ex, std::uint32_t word, std::uint32_t pc ) noexcept
{
  auto const interrupt = ex == ExCause::Int;

  if ( interrupt )
  {
    auto const ie = cp0.status & 1;
    auto const erl_exl = cp0.status & 0b110;
    if ( !ie || erl_exl )
      return;

    cp0.epc = pc;
  }
  else
  {
    if ( ex == ExCause::AdEL || ex == ExCause::AdES )
      cp0.bad_vaddr = pc;

    cp0.bad_instr = word;
    cp0.error_epc = pc;
    cp0.status |= 0b10; // Sets Status EXL
  }

  enter_kernel_mode();

  set_ex_cause( ex );

  this->pc = ( cp0.e_base & 0xFFFF'F000 ) + 0x180;
}
} // namespace mips32