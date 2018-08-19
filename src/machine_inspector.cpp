#include <mips32/cache.hpp>
#include <mips32/cp0.hpp>
#include <mips32/cp1.hpp>
#include <mips32/cpu.hpp>
#include <mips32/ram.hpp>

#include <mips32/machine_inspector.hpp>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>

namespace mips32
{
MachineInspector &MachineInspector::inspect( RAM &ram ) noexcept
{
  this->ram = &ram;
  return *this;
}

//void MachineInspector::inspect( Cache &cache ) noexcept { this->cache = &cache; return *this; }

MachineInspector & MachineInspector::inspect( CP0 & cp0 ) noexcept
{
  this->cp0 = &cp0;
  return *this;
}

MachineInspector &MachineInspector::inspect( CP1 &cp1 ) noexcept
{
  this->cp1 = &cp1;
  return *this;
}

MachineInspector &MachineInspector::inspect( CPU &cpu, bool sub_components ) noexcept
{
  this->cpu = &cpu;
  if ( sub_components )
  {
    inspect( this->cpu->cp0 );
    inspect( this->cpu->cp1 );
  }
  return *this;
}

/* * * * *
 *       *
 * STATE *
 *       *
 * * * * */

bool MachineInspector::save_state( Component c, char const *name ) noexcept
{
  cpu->stop();

  bool error = true;

  if ( c == Component::ALL )
  {
    error = save_state_cp0( name );
    error |= save_state_cp1( name );
    error |= save_state_cpu( name );
    error |= save_state_ram( name );
  }
  else if ( c == Component::CP0 )
  {
    error = save_state_cp0( name );
  }
  else if ( c == Component::CP1 )
  {
    error = save_state_cp1( name );
  }
  else if ( c == Component::CPU )
  {
    error = save_state_cpu( name );
  }
  else if ( c == Component::RAM )
  {
    error = save_state_ram( name );
  }

  return error;
}

bool MachineInspector::restore_state( Component c, char const *name ) noexcept
{
  cpu->stop();

  bool error = true;

  if ( c == Component::ALL )
  {
    error = restore_state_cp0( name );
    error |= restore_state_cp1( name );
    error |= restore_state_cpu( name );
    error |= restore_state_ram( name );
  }
  else if ( c == Component::CP0 )
  {
    error = restore_state_cp0( name );
  }
  else if ( c == Component::CP1 )
  {
    error = restore_state_cp1( name );
  }
  else if ( c == Component::CPU )
  {
    error = restore_state_cpu( name );
  }
  else if ( c == Component::RAM )
  {
    error = restore_state_ram( name );
  }

  return error;
}

/* * * *
 *     *
 * RAM *
 *     *
 * * * */

MachineInspector::RAMInfo MachineInspector::RAM_info() const noexcept
{
  return { RAM_alloc_limit(), RAM_block_size(),
          RAM_allocated_blocks_no(), RAM_swapped_blocks_no(),
          RAM_allocated_addresses(), RAM_swapped_addresses() };
}

std::uint32_t MachineInspector::RAM_alloc_limit() const noexcept
{
  return ram->alloc_limit * RAM::block_size * sizeof( std::uint32_t );
}

std::uint32_t MachineInspector::RAM_block_size() const noexcept
{
  return ram->block_size;
}

std::uint32_t MachineInspector::RAM_allocated_blocks_no() const noexcept
{
  return ( std::uint32_t )ram->blocks.size();
}

std::uint32_t MachineInspector::RAM_swapped_blocks_no() const noexcept
{
  return ( std::uint32_t )ram->swapped.size();
}

std::vector<std::uint32_t> MachineInspector::RAM_allocated_addresses() const
noexcept
{
  std::vector<std::uint32_t> addresses;
  addresses.reserve( RAM_allocated_blocks_no() );

  for ( auto const &block : ram->blocks )
    addresses.emplace_back( block.base_address );

  return addresses;
}

std::vector<std::uint32_t> MachineInspector::RAM_swapped_addresses() const
noexcept
{
  std::vector<std::uint32_t> addresses;
  addresses.reserve( RAM_swapped_blocks_no() );

  for ( auto const &block : ram->swapped )
    addresses.emplace_back( block.base_address );

  return addresses;
}

/* * * * *
 *       *
 * CACHE *
 *       *
 * * * * */
/*
MachineInspector::CacheBlockIterator MachineInspector::CACHE_block_begin() noexcept
{
  return {cache->headers.data(),
          cache->lines.data(),
          cache->words_per_block};
}

MachineInspector::CacheBlockIterator MachineInspector::CACHE_block_end() noexcept
{
  return {cache->headers.data() + cache->headers.size(),
          cache->lines.data() + cache->lines.size(),
          cache->words_per_block};
}

std::uint32_t MachineInspector::CACHE_line_no() const noexcept
{
  return (std::uint32_t)cache->headers.size() / cache->associativity;
}

std::uint32_t MachineInspector::CACHE_associativity() const noexcept
{
  return cache->associativity;
}

std::uint32_t MachineInspector::CACHE_block_size() const noexcept
{
  return cache->words_per_block;
}
*/

/* * * * *
 *       *
 * COP 1 *
 *       *
 * * * * */
MachineInspector::CP1_FPR_iterator MachineInspector::CP1_fpr_begin() noexcept
{
  return cp1->fpr.begin();
}

MachineInspector::CP1_FPR_iterator MachineInspector::CP1_fpr_end() noexcept
{
  return cp1->fpr.end();
}

std::uint32_t MachineInspector::CP1_fir() const noexcept
{
  return cp1->fir;
}

std::uint32_t MachineInspector::CP1_fcsr() const noexcept
{
  return cp1->fcsr;
}
/* * * *
 *     *
 * CPU *
 *     *
 * * * */

MachineInspector::CPU_GPR_iterator MachineInspector::CPU_gpr_begin() noexcept
{
  return cpu->gpr.begin();
}

MachineInspector::CPU_GPR_iterator MachineInspector::CPU_gpr_end() noexcept
{
  return cpu->gpr.end();
}

std::uint32_t &MachineInspector::CPU_pc() noexcept
{
  return cpu->pc;
}

std::uint32_t MachineInspector::CPU_read_exit_code() const noexcept
{
  return cpu->exit_code.load( std::memory_order_acquire );
}

void MachineInspector::CPU_write_exit_code( std::uint32_t value ) noexcept
{
  cpu->exit_code.store( value, std::memory_order_release );
}

CP0 & MachineInspector::access_cp0() noexcept
{
  return *cp0;
}

/* * * * * * * * *
 *               *
 * FILE FORMATS  *
 *               *
 * * * * * * * * */

////
//// RAM
////
/**
 * uint32_t -> alloc_limit -|
 * uint32_t -> blocks_no    |- data
 * uint32_t -> swap_no     -|
 * (uint32_t, uint32_t, uint32_t * RAM::block_size) * blocks_no -> base_address, access_count, data
 * uint32_t * swap_no -> base_address
 **/
bool MachineInspector::save_state_ram( char const * name ) const noexcept
{
  std::string ram_file_name{ name };
  ram_file_name += ".ram";

  auto * file = std::fopen( ram_file_name.c_str(), "wb" );
  if ( !file )
    return true;

  // Data
  std::uint32_t const _alloc_limit = ram->alloc_limit;
  std::uint32_t const _blocks_no = ram->blocks.size();
  std::uint32_t const _swap_no = ram->swapped.size();

  std::fwrite( &_alloc_limit, sizeof( _alloc_limit ), 1, file );
  std::fwrite( &_blocks_no, sizeof( _blocks_no ), 1, file );
  std::fwrite( &_swap_no, sizeof( _swap_no ), 1, file );

  // Allocated blocks
  for ( auto const & block : ram->blocks )
  {
    std::uint32_t const _base_address = block.base_address;
    std::uint32_t const _access_count = block.access_count;

    std::fwrite( &_base_address, sizeof( _base_address ), 1, file );
    std::fwrite( &_access_count, sizeof( _access_count ), 1, file );
    std::fwrite( block.data.get(), sizeof( block.data[0] ), RAM::block_size, file );
  }

  // Swapped blocks
  std::fwrite( ram->swapped.data(), sizeof( ram->swapped[0] ), _swap_no, file );

  std::fflush( file );
  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

////
//// CP0
////
/**
 * Trivially copyable
 **/
bool MachineInspector::save_state_cp0( char const * name ) const noexcept
{
  std::string cp0_file_name{ name };
  cp0_file_name += ".cp0";

  auto * file = std::fopen( cp0_file_name.c_str(), "wb" );
  if ( !file )
    return true;

  // CP0
  [[maybe_unused]] auto cp0_write_count = std::fwrite( cp0, sizeof( CP0 ), 1, file );

  assert( cp0_write_count == 1 && "Coudln't write CP0 to file!" );

  std::fflush( file );
  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

////
//// CP1
////
/**
 * uint32_t * 32, fprs
 * uint32_t, fir
 * uint32_t, fcsr
 * fenv_t, env
 **/
bool MachineInspector::save_state_cp1( char const * name ) const noexcept
{
  std::string cp1_file_name{ name };
  cp1_file_name += ".cp1";

  auto * file = std::fopen( cp1_file_name.c_str(), "wb" );
  if ( !file )
    return true;

  // CP1
  [[maybe_unused]] auto fpr_write_count = std::fwrite( cp1->fpr.data(), sizeof( cp1->fpr[0] ), 32, file );
  [[maybe_unused]] auto fir_write_count = std::fwrite( &cp1->fir, sizeof( cp1->fir ), 1, file );
  [[maybe_unused]] auto fcsr_write_count = std::fwrite( &cp1->fcsr, sizeof( cp1->fcsr ), 1, file );
  [[maybe_unused]] auto env_write_count = std::fwrite( &cp1->env, sizeof( cp1->env ), 1, file );

  assert( fpr_write_count == 32 && "Coudln't write the FPRs to file!" );
  assert( fir_write_count == 1 && "Coudln't write FIR to file!" );
  assert( fcsr_write_count == 1 && "Coudln't write FCSR to file!" );
  assert( env_write_count == 1 && "Coudln't write ENV to file!" );

  std::fflush( file );
  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

////
//// CPU
////
/**
 * -- CP0 --
 * see `save_state_cp0`
 *
 * -- CP1 --
 * see `save_state_cp0`
 *
 * -- MMU --
 * uint32_t, segment_no
 * Segment * segment_no, segments
 *
 * uint32_t, pc
 * uint32_t * 32, gprs
 * exit_code is always restored as `NONE`
 **/
bool MachineInspector::save_state_cpu( char const * name ) const noexcept
{
  if ( save_state_cp0( name ) )
    return true;

  if ( save_state_cp1( name ) )
    return true;

  std::string cpu_file_name{ name };
  cpu_file_name += ".cpu";

  auto * file = std::fopen( cpu_file_name.c_str(), "wb" );
  if ( !file )
    return true;

  // MMU
  std::uint32_t const _segment_no = cpu->mmu.segments.size();
  [[maybe_unused]] auto seg_write_count = std::fwrite( &_segment_no, sizeof( _segment_no ), 1, file );
  [[maybe_unused]] auto segdata_write_count = std::fwrite( cpu->mmu.segments.data(), sizeof( cpu->mmu.segments[0] ), _segment_no, file );

  assert( seg_write_count == 1 && "Coudln't write the number of segments to file!" );
  assert( segdata_write_count == _segment_no && "Coudln't write segment's data to file!" );

  // CPU
  [[maybe_unused]] auto pc_write_count = std::fwrite( &cpu->pc, sizeof( cpu->pc ), 1, file );
  [[maybe_unused]] auto gpr_write_count = std::fwrite( cpu->gpr.data(), sizeof( cpu->gpr[0] ), cpu->gpr.size(), file );

  assert( pc_write_count == 1 && "Coudln't write PC to file!" );
  assert( gpr_write_count == cpu->gpr.size() && "Coudln't write GPRs to file!" );

  std::fflush( file );
  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

////
//// RAM
////
/**
 * uint32_t -> alloc_limit -|
 * uint32_t -> blocks_no    |- data
 * uint32_t -> swap_no     -|
 * (uint32_t, uint32_t, uint32_t * RAM::block_size) * blocks_no -> base_address, access_count, data
 * uint32_t * swap_no -> base_address
 *
 * 1. Load the data from disk
 * 2. Load allocated blocks from disk
 * 3. Resize and overwrite swapped blocks data
 **/
bool MachineInspector::restore_state_ram( char const * name ) noexcept
{
  std::string ram_file_name{ name };
  ram_file_name += ".ram";

  auto * file = std::fopen( ram_file_name.c_str(), "rb" );
  if ( !file )
    return true;

  bool error = false;

  // 1
  std::uint32_t _alloc_limit = 0;
  std::uint32_t _blocks_no = 0;
  std::uint32_t _swap_no = 0;

  [[maybe_unused]] auto alloc_read_count = std::fread( &_alloc_limit, sizeof( _alloc_limit ), 1, file );
  [[maybe_unused]] auto blocks_read_count = std::fread( &_blocks_no, sizeof( _blocks_no ), 1, file );
  [[maybe_unused]] auto swap_read_count = std::fread( &_swap_no, sizeof( _swap_no ), 1, file );

  assert( alloc_read_count == 1 && "Coudln't read alloc_limit from file!" );
  assert( blocks_read_count == 1 && "Coudln't read the number of allocated blocks from file!" );
  assert( swap_read_count == 1 && "Coudln't read the number of swapped blocks from file!" );

  ram->alloc_limit = _alloc_limit;

  // 2
  ram->blocks.resize( _blocks_no );

  for ( auto & block : ram->blocks )
  {
    [[maybe_unused]] auto addr_read_count = std::fread( &block.base_address, sizeof( block.base_address ), 1, file );
    [[maybe_unused]] auto access_read_count = std::fread( &block.access_count, sizeof( block.access_count ), 1, file );

    assert( addr_read_count == 1 && "Coudln't read the base_address from file!" );
    assert( access_read_count == 1 && "Coudln't read the access_count from file!" );

    if ( !block.data )
      block.allocate();

    if ( !block.data )
    {
      error = true;
      break;
    }

    [[maybe_unused]] auto data_read_count = std::fread( block.data.get(), sizeof( *block.data.get() ), RAM::block_size, file );

    assert( data_read_count == RAM::block_size && "Coudln't read the block's data from file!" );
  }

  // 3
  ram->swapped.resize( _swap_no );
  std::fread( ram->swapped.data(), sizeof( ram->swapped[0] ), _swap_no, file );

  error = error || std::ferror( file );
  std::fclose( file );
  return error;
}

//// 
//// CP0
//// 
/**
 * Trivially copyable
 **/
bool MachineInspector::restore_state_cp0( char const * name ) noexcept
{
  std::string cp0_file_name{ name };
  cp0_file_name += ".cp0";

  auto * file = std::fopen( cp0_file_name.c_str(), "rb" );
  if ( !file )
    return true;

  // CP0
  [[maybe_unused]] auto cp0_read_count = std::fread( cp0, sizeof( CP0 ), 1, file );

  assert( cp0_read_count == 1 && "Coudln't read CP0 from file!" );

  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

//// 
//// CP1
//// 
/**
 * After reading back the data we need to:
 * 1. set the FP Environment,
 * 2. set the rounding mode,
 * 3. set flushing mode for denormalized numbers
 **/
bool MachineInspector::restore_state_cp1( char const * name ) noexcept
{
  std::string cp1_file_name{ name };
  cp1_file_name += ".cp1";

  auto * file = std::fopen( cp1_file_name.c_str(), "rb" );
  if ( !file )
    return true;

  [[maybe_unused]] auto fpr_read_count = std::fread( cp1->fpr.data(), sizeof( FPR ), cp1->fpr.size(), file );
  [[maybe_unused]] auto fir_read_count = std::fread( &cp1->fir, sizeof( cp1->fir ), 1, file );
  [[maybe_unused]] auto fcsr_read_count = std::fread( &cp1->fcsr, sizeof( cp1->fcsr ), 1, file );
  [[maybe_unused]] auto env_read_count = std::fread( &cp1->env, sizeof( cp1->env ), 1, file );

  assert( fpr_read_count == 32 && "Coudln't read the FPRs from file!" );
  assert( fir_read_count == 1 && "Coudln't read FIR from file!" );
  assert( fcsr_read_count == 1 && "Coudln't read FCSR from file!" );
  assert( env_read_count == 1 && "Coudln't read ENV from file!" );

  std::fesetenv( &cp1->env );
  cp1->set_round_mode();
  cp1->set_denormal_flush();

  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

////
//// CPU
////
/**
 * -- CP0 --
 * see `restore_state_cp0`
 *
 * -- CP1 --
 * see `restore_state_cp0`
 *
 * -- MMU --
 * uint32_t, segment_no
 * Segment * segment_no, segments
 *
 * uint32_t, pc
 * uint32_t * 32, gprs
 * exit_code is always NONE
 **/
bool MachineInspector::restore_state_cpu( char const * name ) noexcept
{
  if ( restore_state_cp0( name ) )
    return true;

  if ( restore_state_cp1( name ) )
    return true;

  std::string cpu_file_name{ name };
  cpu_file_name += ".cpu";

  auto * file = std::fopen( cpu_file_name.c_str(), "rb" );

  // MMU
  std::uint32_t _segment_no = 0;
  [[maybe_unused]] auto seg_read_count = std::fread( &_segment_no, sizeof( _segment_no ), 1, file );

  assert( seg_read_count == 1 && "Coudln't read the number of segments from file!" );

  cpu->mmu.segments.resize( _segment_no );
  [[maybe_unused]] auto segdata_read_count = std::fread( cpu->mmu.segments.data(), sizeof( MMU::Segment ), _segment_no, file );

  assert( segdata_read_count == _segment_no && "Coudln't read segment's data from file!" );

  // CPU
  [[maybe_unused]] auto pc_read_count = std::fread( &cpu->pc, sizeof( cpu->pc ), 1, file );
  [[maybe_unused]] auto gpr_read_count = std::fread( cpu->gpr.data(), sizeof( cpu->gpr[0] ), cpu->gpr.size(), file );
  cpu->exit_code.store( 0, std::memory_order_release );

  assert( pc_read_count == 1 && "Coudln't read PC from file!" );
  assert( gpr_read_count == cpu->gpr.size() && "Coudln't read GPRs from file!" );

  bool error = std::ferror( file );
  std::fclose( file );
  return error;
}

} // namespace mips32