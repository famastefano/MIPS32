#pragma once

#include <cstdint>
#include <vector>

namespace mips32 {

class RAM;

class MachineInspector
{
  public:
  void inspect( RAM &ram ) noexcept;

  /*******
   *     *
   * RAM *
   *     *
   *******/

  void save_RAM_state( char const *name = nullptr ) const noexcept;
  void restore_RAM_state( char const *name = nullptr ) noexcept;

  struct RAMInfo
  {
    std::uint32_t              alloc_limit;
    std::uint32_t              block_size;
    std::uint32_t              allocated_blocks_no;
    std::uint32_t              swapped_blocks_no;
    std::vector<std::uint32_t> allocated_addresses;
    std::vector<std::uint32_t> swapped_addresses;
  };

  RAMInfo RAM_info() const noexcept;

  std::uint32_t              RAM_alloc_limit() const noexcept;
  std::uint32_t              RAM_block_size() const noexcept;
  std::uint32_t              RAM_allocated_blocks_no() const noexcept;
  std::uint32_t              RAM_swapped_blocks_no() const noexcept;
  std::vector<std::uint32_t> RAM_allocated_addresses() const noexcept;
  std::vector<std::uint32_t> RAM_swapped_addresses() const noexcept;

  private:
  RAM *ram;
};
} // namespace mips32