#include <mips32/cache.hpp>
#include <mips32/ram.hpp>

#include <mips32/machine_inspector.hpp>

namespace mips32 {
void MachineInspector::inspect( RAM &ram ) noexcept { this->ram = &ram; }

void MachineInspector::inspect( Cache &cache ) noexcept { this->cache = &cache; }

/*********
 *       *
 * STATE *
 *       *
 *********/

void MachineInspector::save_state( Component c, char const *name ) const noexcept {}
void MachineInspector::restore_state( Component c, char const *name ) noexcept {}

/*******
 *     *
 * RAM *
 *     *
 *******/

MachineInspector::RAMInfo MachineInspector::RAM_info() const noexcept
{
  return {RAM_alloc_limit(), RAM_block_size(),
          RAM_allocated_blocks_no(), RAM_swapped_blocks_no(),
          RAM_allocated_addresses(), RAM_swapped_addresses()};
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
  return (std::uint32_t)ram->blocks.size();
}

std::uint32_t MachineInspector::RAM_swapped_blocks_no() const noexcept
{
  return (std::uint32_t)ram->swapped.size();
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

/*********
 *       *
 * CACHE *
 *       *
 *********/

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

} // namespace mips32