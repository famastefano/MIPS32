#pragma once

#ifdef _MSC_VER
#  define MIPS32_EXPORT __declspec(dllexport)
#else
#  define MIPS32_EXPORT
#endif

#include <mips32/io_device.hpp>
#include <mips32/file_handler.hpp>
#include <mips32/machine_inspector.hpp>

#include <cstdint>

namespace mips32
{
inline namespace v0
{
class MachineImpl;
}

/**
 * 
 * MIPS32 Machine Interface
 * 
 * Simulates a machine with a RAM and a MIPS32 CPU.
 * 
 **/
class MIPS32_EXPORT Machine
{
public:
  Machine( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler ) noexcept;

  // Movable only
  Machine( Machine const& ) = delete;
  Machine( Machine&& ) = default;

  ~Machine();

  /**
   * Loads an executable
   * 
   * For the Executable File Format, read `executable_format.md` on the repository
   * 
   * `data` must point to a valid memory region
   **/
  bool load( void const * data ) noexcept;

  MachineInspector get_inspector() noexcept;

  /**
   * Signals the CPU to start executing instructions.
   * 
   * This function returns only if:
   * - the break instruction is executed
   * - an interrupt/exception is triggered
   * - the exit syscall is called
   * - manually called stop()
   **/
  std::uint32_t start() noexcept;

  /**
   * Signals the CPU to stop executing instructions.
   **/
  void stop() noexcept;

  /**
   * Execute 1 instruction
   **/
  std::uint32_t single_step() noexcept;

  /**
   * Resets the CPU and its Coprocessors
   * The RAM is left untouched
   **/
  void reset() noexcept;

  // Used to modify the handlers to perform I/O
  // Returns the previous handler
  IODevice* swap_iodevice( IODevice *device ) noexcept;
  FileHandler* swap_file_handler( FileHandler *handler ) noexcept;

private:
  MachineImpl *_impl;
};
}