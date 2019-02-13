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

class MIPS32_EXPORT Machine
{
public:
  Machine( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler ) noexcept;

  Machine( Machine const& ) = delete;

  Machine( Machine&& ) = default;

  ~Machine();

  bool load( void const * data ) noexcept;

  MachineInspector get_inspector() noexcept;

  std::uint32_t start() noexcept;

  void stop() noexcept;

  std::uint32_t single_step() noexcept;

  void reset() noexcept;

  IODevice* swap_iodevice( IODevice *device ) noexcept;
  FileHandler* swap_file_handler( FileHandler *handler ) noexcept;

private:
  MachineImpl *_impl;
};
}