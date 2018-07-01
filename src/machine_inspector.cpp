#include "RAM/ram.hpp"

#include <machine_inspector.hpp>

namespace mips32 {
void MachineInspector::inspect( RAM &ram ) noexcept { this->ram = &ram; }

/*******
 *     *
 * RAM *
 *     *
 *******/

void MachineInspector::save_RAM_state( char const *name ) const noexcept {}

void MachineInspector::restore_RAM_state( char const *name ) noexcept {}

MachineInspector::RAMInfo MachineInspector::RAM_info() const noexcept
{
  return {RAM_alloc_limit(),         RAM_block_size(),
          RAM_allocated_blocks_no(), RAM_swapped_blocks_no(),
          RAM_allocated_addresses(), RAM_swapped_addresses()};
}

std::uint32_t MachineInspector::RAM_alloc_limit() const noexcept
{
  return ram->alloc_limit;
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
  std::vector<std::uint32_t> addresses( RAM_allocated_blocks_no() );

  for ( auto const &block : ram->blocks )
    addresses.emplace_back( block.base_address );

  return addresses;
}

std::vector<std::uint32_t> MachineInspector::RAM_swapped_addresses() const
    noexcept
{
  std::vector<std::uint32_t> addresses( RAM_swapped_blocks_no() );

  for ( auto const &block : ram->swapped )
    addresses.emplace_back( block.base_address );

  return addresses;
}

} // namespace mips32