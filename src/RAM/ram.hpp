#pragma once

#include <literals.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace mips32 {

using namespace literals;

class RAM
{
  friend class MachineInspector;

  public:
  static inline constexpr std::uint32_t block_size{
      64_KB / sizeof( std::uint32_t )}; // 16K std::uint32_ts

  explicit RAM( std::uint32_t alloc_limit );

  RAM( RAM && ) = default;
  RAM &operator=( RAM && ) = default;

  RAM( RAM const & ) = delete;
  RAM &operator=( RAM const & ) = delete;

  std::uint32_t &operator[]( std::uint32_t address ) noexcept;

  private:
  struct Block
  {
    std::uint32_t                    base_address;
    std::uint32_t                    access_count{0};
    std::unique_ptr<std::uint32_t[]> data;

    Block &allocate() noexcept;
    Block &deallocate() noexcept;

    Block &serialize() noexcept;
    Block &deserialize() noexcept;

    std::uint32_t &operator[]( std::uint32_t pos ) noexcept
    {
      ++access_count;
      return data[pos];
    }
  };

  struct SwappedBlock
  {
    std::uint32_t base_address;
  };

  bool contains( std::uint32_t base, std::uint32_t address,
                 std::uint32_t limit ) const noexcept
  {
    return base <= address && address < base + limit;
  }

  Block &least_accessed() noexcept
  {
    std::sort( blocks.begin(), blocks.end(),
               []( Block &lhs, Block &rhs ) -> bool {
                 return lhs.access_count > rhs.access_count;
               } );

    for ( auto &block : blocks ) block.access_count = 0;

    return blocks.back();
  }

  std::uint32_t             alloc_limit;
  std::vector<Block>        blocks;
  std::vector<SwappedBlock> swapped;
};
} // namespace mips32