#include <mips32/machine.hpp>
#include <mips32/literals.hpp>

#include "ram.hpp"
#include "cpu.hpp"

namespace mips32
{
using namespace mips32::literals;

inline namespace v0
{
class MachineImpl
{
  friend class MachineInspector;

public:
  MachineImpl( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler ) noexcept;

  MachineImpl( MachineImpl const& ) = delete;

  MachineImpl( MachineImpl&& ) = default;

  ~MachineImpl();

  bool load( void const * data ) noexcept;

  std::uint32_t run() noexcept;

  std::uint32_t stop() noexcept;

  std::uint32_t single_step() noexcept;

  void reset() noexcept;

  IODevice* swap_io_device( IODevice *device ) noexcept;
  FileHandler* swap_file_handler( FileHandler *handler ) noexcept;

private:
  RAM ram;
  CPU cpu;
};
}

Machine::Machine( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler )
  : _impl( new MachineImpl( ram_alloc_limit, io_device, file_handler ) noexcept
{}

Machine::~Machine() { delete _impl; }

bool Machine::load( void const * data ) noexcept;

MachineInspector Machine::get_inspector() noexcept
{
  MachineInspector inspector;
  inspector.inspect( *_impl );
  return inspector;
}

std::uint32_t Machine::run() noexcept
{
  return _impl->run();
}

std::uint32_t Machine::stop() noexcept
{
  return _impl->stop();
}

std::uint32_t Machine::single_step() noexcept
{
  return _impl->single_step();
}

void Machine::reset() noexcept
{
  _impl->reset();
}

IODevice* Machine::swap_io_device( IODevice *device ) noexcept
{
  return _impl->swap_io_device( device );
}

FileHandler* Machine::swap_file_handler( FileHandler *handler ) noexcept
{
  return _impl->swap_file_handler( handler );
}


MachineImpl::MachineImpl( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler )
  : ram( ram_alloc_limit ), cpu( ram )
{
  cpu.attach_io_device( io_device );
  cpu.attach_file_handler( file_handler );
}

MachineImpl::~MachineImpl()
{
  cpu.stop();
}

std::uint32_t MachineImpl::run() noexcept
{
  return cpu.run();
}

std::uint32_t MachineImpl::stop() noexcept
{
  return cpu.stop();
}

std::uint32_t MachineImpl::single_step() noexcept
{
  return cpu.single_step();
}

void reset() noexcept
{
  cpu.hard_reset();
}

IODevice* MachineImpl::swap_io_device( IODevice *device ) noexcept
{
  return cpu.attach_io_device( device );
}

FileHandler* MachineImpl::swap_file_handler( FileHandler *handler ) noexcept
{
  return cpu.attach_file_handler( handler );
}

bool MachineImpl::load( void const * data ) noexcept
{
  return false;
}

}