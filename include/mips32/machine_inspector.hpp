#pragma once

#include <mips32/fpr.hpp>
#include <mips32/header.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace mips32
{

class RAM;
//class Cache;
class CP0;
class CP1;
class CPU;

/**
 * This class allows the inspection and manipulation of the entire Machine.
 * It was created to ease the debugging purposes that a simulator creates,
 * like inspecting the RAM, CPU registers and so on.
 **/
class MachineInspector
{
public:
  MachineInspector & inspect( RAM &ram ) noexcept;
  // MachineInspector& inspect( Cache &cache ) noexcept;
  MachineInspector &inspect( CP0 &cp0 ) noexcept;
  MachineInspector &inspect( CP1 &cp1 ) noexcept;
  MachineInspector &inspect( CPU &cpu, bool sub_components = true) noexcept;

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
   **/

  enum class Component
  {
    RAM,
    //CACHE,
    CP0,
    CP1,
    CPU
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
   * COP 0 *
   *       *
   * * * * */

  std::uint32_t& CP0_user_local() noexcept;
  std::uint32_t& CP0_hwr_ena() noexcept;
  std::uint32_t& CP0_bad_vaddr() noexcept;
  std::uint32_t& CP0_bad_instr() noexcept;
  std::uint32_t& CP0_status() noexcept;
  std::uint32_t& CP0_int_ctl() noexcept;
  std::uint32_t& CP0_srs_ctl() noexcept;
  std::uint32_t& CP0_cause() noexcept;
  std::uint32_t& CP0_epc() noexcept;
  std::uint32_t& CP0_pr_id() noexcept;
  std::uint32_t& CP0_e_base() noexcept;
  std::uint32_t& CP0_config( std::uint32_t n ) noexcept;
  std::uint32_t& CP0_error_epc() noexcept;
  std::uint32_t& CP0_k_scratch( std::uint32_t n ) noexcept;

 /* * * * *
  *       *
  * COP 1 *
  *       *
  * * * * */

  using CP1_FPR_iterator = typename std::array<FPR, 32>::iterator;

  CP1_FPR_iterator CP1_fpr_begin() noexcept;
  CP1_FPR_iterator CP1_fpr_end() noexcept;

  std::uint32_t CP1_fir() const noexcept;
  std::uint32_t CP1_fcsr() const noexcept;

  /* * * *
   *     *
   * CPU *
   *     *
   * * * */

  using CPU_GPR_iterator = typename std::array<std::uint32_t, 32>::iterator;

  CPU_GPR_iterator CPU_gpr_begin() noexcept;
  CPU_GPR_iterator CPU_gpr_end() noexcept;

  std::uint32_t &CPU_pc() noexcept;

  std::uint32_t CPU_read_exit_code() const noexcept;
  void          CPU_write_exit_code( std::uint32_t value ) noexcept;

private:
  RAM * ram;
  //Cache *cache;
  CP0 *cp0;
  CP1 *cp1;
  CPU *cpu;
};

//#include <mips32/inspector_iterators/cache_iterator.hpp>

} // namespace mips32