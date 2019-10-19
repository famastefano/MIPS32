#pragma once

#include <mips32/literals.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace mips32
{

using namespace literals;

/**
 * Simulate the RAM of a computer.
 * The address space is 2^32, so 4 GB.
 *
 * The API is very simple, treat it like an array of 32-bit words
 * through operator[], for example: ram[0], ram[4], ram[8].
 *
 * Due to size optimization purposes, this class starts to
 * swap blocks on disk once it reaches the allocation limit.
 * This allows you to use the entire address space of 4GB
 * without using it all at once.
 *
 * Every block is guaranteed to hold a contiguous sequence
 * of words, while the blocks, to each other, are not guaranteed to be.
 *
 * It satisfies MoveConstructible and MoveAssignable.
 *
 **/
class RAM
{
  friend class MachineInspector;
  friend class RAMIO;

public:
  // A block holds 64KB.
  static inline constexpr std::uint32_t block_size{ 64_KB };

  // Construct a RAM object and specifies
  // how much memory, in bytes, it can use to hold the blocks.
  //
  // A minimum of ``RAM::block_size`` is required.
  explicit RAM( std::uint32_t alloc_limit );

  // Movable
  RAM( RAM && ) = default;
  RAM &operator=( RAM && ) = default;

  // Non copyable
  RAM( RAM const & ) = delete;
  RAM &operator=( RAM const & ) = delete;

  // Returns the word at the given address
  std::uint32_t &operator[]( std::uint32_t address ) noexcept;

  inline static constexpr std::uint32_t calculate_base_address( std::uint32_t address ) noexcept
  {
    std::uint32_t base_address = 0;

    while ( base_address + RAM::block_size <= address )
      base_address += RAM::block_size;

    return base_address;
  }

private:
  // Represent a portion of data of our RAM.
  // It's a very simple class that owns `RAM::block_size` words.
  struct Block
  {
    std::uint32_t                    base_address;    // base address of our block
    std::uint32_t                    access_count{ 0 }; // number of accesses through operator[]
    std::unique_ptr<std::uint32_t[]> data;            // Words array

    // Allocate a `RAM::block_size` array of words.
    // If it fails, `data` holds nullptr,
    // otherwise `data` points to a valid memory region.
    Block &allocate() noexcept;

    // Deallocate the data.
    Block &deallocate() noexcept;

    // Copies the data to disk inside a file
    // called 0xXXXXXXXX.block, where XXX
    // stands for the `base_address` in hexadecimal.
    Block &serialize() noexcept;

    // Copies the data from disk, from a file
    // called 0xXXXXXXXX.block, where XXX
    // stands for the `base_address` in hexadecimal.
    Block &deserialize() noexcept;

    // Returns the word specified by `pos`.
    // Also increase the counter of `access_count` by 1.
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

  // Helper function that checks if the given addres belongs to a block.
  bool contains( std::uint32_t base, std::uint32_t address, std::uint32_t limit ) const noexcept
  {
    return base <= address && address < base + limit;
  }

  /**
   * This is our algorithm that selects a block to overwrite.
   * It does 3 things:
   *   1. Sort the block list by their `access_count` in descending order.
   *      This means that the most accessed block is also the first.
   *   2. Resets the `access_count`.
   *      This means that every block can be selected for the next substitution.
   *   3. Returns the last block, that is the least accessed (due to sorting).
   **/
  Block &least_accessed() noexcept;

  std::uint32_t             alloc_limit; // Maximum number of allocable blocks.
  std::vector<Block>        blocks;      // Block list.
  std::vector<SwappedBlock> swapped;     // Swapped block list.
};
} // namespace mips32