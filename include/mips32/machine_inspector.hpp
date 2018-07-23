#pragma once

#include <mips32/fpr.hpp>
#include <mips32/header.hpp>

#include <cstdint>
#include <vector>

namespace mips32 {

class RAM;
class Cache;
class CP1;

/**
 * This class allows the inspection and manipulation of the entire Machine.
 * It was created to ease the debugging purposes that a simulator creates,
 * like inspecting the RAM, CPU registers and so on.
 */
class MachineInspector
{
  public:
  void inspect( RAM &ram ) noexcept;
  // void inspect( Cache &cache ) noexcept;
  void inspect( CP1 &cp1 ) noexcept;

  /* * * * *
   *       *
   * STATE *
   *       *
   * * * * */

  /**
   * The Machine's state is created by composing all the
   * component's states togheter.
   * Each state has its own binary representation.
   * **Do not** modify it manually nor try to recreate it.
   * The representation can change at any time and it is
   * *not* guaranteed to be compatible with previous versions of this library.
   */

  enum class Component
  {
    RAM,
    //CACHE,
    CP1,
  };

  // Save and restore the entire Machine state to a file.

  void save_state( char const *name ) const noexcept;
  void restore_state( char const *name ) noexcept;

  // Save and restore the state of the specified component to a file.

  void save_state( Component c, char const *name ) const noexcept;
  void restore_state( Component c, char const *name ) noexcept;

  /* * * *
   *     *
   * RAM *
   *     *
   * * * */

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

  /* * * * *
   *       *
   * CACHE *
   *       *
   * * * * */
  /*
  class CacheBlockIterator;

  CacheBlockIterator CACHE_block_begin() noexcept;
  CacheBlockIterator CACHE_block_end() noexcept;

  std::uint32_t CACHE_line_no() const noexcept;
  std::uint32_t CACHE_associativity() const noexcept;
  std::uint32_t CACHE_block_size() const noexcept;
*/
  /* * * * *
   *       *
   * COP 1 *
   *       *
   * * * * */

  class FPR;

  std::uint32_t CP1_fir() const noexcept;
  std::uint32_t CP1_fcsr() const noexcept;

  FPR CP1_fpr( std::uint32_t index ) noexcept;

  FPR CP1_fpr_begin() noexcept;
  FPR CP1_fpr_end() noexcept;

  private:
  RAM *ram;

  //Cache *cache;

  CP1 *cp1;
};

//#include <mips32/inspector_iterators/cache_iterator.hpp>

#include <mips32/inspector_iterators/cp1_iterator.hpp>

} // namespace mips32