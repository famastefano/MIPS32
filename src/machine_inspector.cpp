#include <mips32/cache.hpp>
#include <mips32/cp0.hpp>
#include <mips32/cp1.hpp>
#include <mips32/cpu.hpp>
#include <mips32/ram.hpp>

#include <mips32/machine_inspector.hpp>

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

void MachineInspector::save_state( char const * name ) const noexcept
{}

void MachineInspector::restore_state( char const * name ) noexcept
{}

void MachineInspector::save_state( Component, char const * ) const noexcept {}
void MachineInspector::restore_state( Component, char const * ) noexcept {}

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
  return ram->alloc_limit * sizeof( std::uint32_t );
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

std::uint32_t & MachineInspector::CP0_user_local() noexcept
{
  return cp0->user_local;
}

std::uint32_t & MachineInspector::CP0_hwr_ena() noexcept
{
  return cp0->hwr_ena;
}

std::uint32_t & MachineInspector::CP0_bad_vaddr() noexcept
{
  return cp0->bad_vaddr;
}

std::uint32_t & MachineInspector::CP0_bad_instr() noexcept
{
  return cp0->bad_instr;
}

std::uint32_t & MachineInspector::CP0_status() noexcept
{
  return cp0->status;
}

std::uint32_t & MachineInspector::CP0_int_ctl() noexcept
{
  return cp0->int_ctl;
}

std::uint32_t & MachineInspector::CP0_srs_ctl() noexcept
{
  return cp0->srs_ctl;
}

std::uint32_t & MachineInspector::CP0_cause() noexcept
{
  return cp0->cause;
}

std::uint32_t & MachineInspector::CP0_epc() noexcept
{
  return cp0->epc;
}

std::uint32_t & MachineInspector::CP0_pr_id() noexcept
{
  return cp0->pr_id;
}

std::uint32_t & MachineInspector::CP0_e_base() noexcept
{
  return cp0->e_base;
}

std::uint32_t & MachineInspector::CP0_config( std::uint32_t n ) noexcept
{
  return cp0->config[n];
}

std::uint32_t & MachineInspector::CP0_error_epc() noexcept
{
  return cp0->error_epc;
}

std::uint32_t & MachineInspector::CP0_k_scratch( std::uint32_t n ) noexcept
{
  return cp0->k_scratch[n];
}

} // namespace mips32