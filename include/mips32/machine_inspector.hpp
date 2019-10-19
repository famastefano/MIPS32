#pragma once

#include <mips32/fpr.hpp>
#include <mips32/header.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace mips32
{

class RAM;
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
  MachineInspector& inspect( RAM &ram ) noexcept;
  MachineInspector& inspect( CP0 &cp0 ) noexcept;
  MachineInspector& inspect( CP1 &cp1 ) noexcept;
  MachineInspector& inspect( CPU &cpu, bool sub_components = true ) noexcept;

  /* * * * *
   *       *
   * STATE *
   *       *
   * * * * */

  /**
   * The Machine's state is created by composing all the component's states togheter.
   * Each state has its own binary representation.
   * **Do not** modify it manually nor try to recreate it.
   * The representation can change at any time and it is
   * *not* guaranteed to be compatible with previous versions of this library.
   *
   * It is also required to have an already constructed machine with
   * all its components inspected, both for saving and restoring their state.
   **/

  enum class Component
  {
    RAM,
    CP0,
    CP1,
    CPU,
    ALL // saves all components
  };

  // Save the state of the given component into the filename `name`.
  // All the components are left untouched *excepts* the CPU that will be stopped.
  //! This function is free to *add* a custom extension to the filename.
  // Returns:
  // `true`  - in case of *failure*
  // `false` - in case of success
  bool save_state( Component c, char const *name ) noexcept;

  // Restore the state of the given component from the filename `name`.
  //! This function is free to *add* a custom extension to the filename.
  // Returns:
  // `true`  - in case of *failure*. !!! it is not guaranteed to have a valid component in this case !!!
  // `false` - in case of success
  bool restore_state( Component c, char const *name ) noexcept;

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

  // Read `count` bytes from the RAM starting at `address`.
  // If you want to read a string with unspecified length, call `RAM_read(0xABCD'1234, -1, true)`
  // 
  // For space reasons, if the RAM doesn't contain a block in the range [address, address + count),
  // the function immediately returns, so it may contain partial data
  std::vector<char> RAM_read( std::uint32_t address, std::uint32_t count, bool read_string = false ) noexcept;

  // Writes exactly `count` bytes from `src` to `address`
  // Behaves *like* a common memcpy
  // 
  // #!#!#!
  // [WARNING] It is undefined behaviour if `src` holds *less* than `count`bytes
  // !#!#!#
  void RAM_write( std::uint32_t address, void const *src, std::uint32_t count ) noexcept;

  /* * * * *
   *       *
   * COP 0 *
   *       *
   * * * * */

  CP0& access_CP0() noexcept;

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

  std::uint32_t& CPU_pc() noexcept;

  std::uint32_t CPU_read_exit_code() const noexcept;
  void          CPU_write_exit_code( std::uint32_t value ) noexcept;

private:
  RAM *ram;
  CP0 *cp0;
  CP1 *cp1;
  CPU *cpu;

  bool save_state_ram( char const*name ) const noexcept;
  bool save_state_cp0( char const*name ) const noexcept;
  bool save_state_cp1( char const*name ) const noexcept;
  bool save_state_cpu( char const*name ) const noexcept;

  bool restore_state_ram( char const*name ) noexcept;
  bool restore_state_cp0( char const*name ) noexcept;
  bool restore_state_cp1( char const*name ) noexcept;
  bool restore_state_cpu( char const*name ) noexcept;
};

} // namespace mips32

