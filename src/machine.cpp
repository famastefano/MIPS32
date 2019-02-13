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

  MachineInspector get_inspector() noexcept;

  bool load( void const * data ) noexcept;

  std::uint32_t start() noexcept;

  void stop() noexcept;

  std::uint32_t single_step() noexcept;

  void reset() noexcept;

  IODevice* swap_io_device( IODevice *device ) noexcept;
  FileHandler* swap_file_handler( FileHandler *handler ) noexcept;

private:
  RAM ram;
  CPU cpu;
};
}

Machine::Machine( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler ) noexcept
  : _impl( new MachineImpl( ram_alloc_limit, io_device, file_handler ) )
{}

Machine::~Machine() { delete _impl; }

bool Machine::load( void const * data ) noexcept { return _impl->load( data ); }

MachineInspector Machine::get_inspector() noexcept { return _impl->get_inspector(); }

std::uint32_t Machine::start() noexcept { return _impl->start(); }

void Machine::stop() noexcept { _impl->stop(); }

std::uint32_t Machine::single_step() noexcept { return _impl->single_step(); }

void Machine::reset() noexcept { _impl->reset(); }

IODevice* Machine::swap_iodevice( IODevice *device ) noexcept { return _impl->swap_io_device( device ); }

FileHandler* Machine::swap_file_handler( FileHandler *handler ) noexcept { return _impl->swap_file_handler( handler ); }

v0::MachineImpl::MachineImpl( std::uint32_t ram_alloc_limit, IODevice* io_device, FileHandler* file_handler ) noexcept
  : ram( ram_alloc_limit ), cpu( ram )
{
  cpu.attach_iodevice( io_device );
  cpu.attach_file_handler( file_handler );
}

v0::MachineImpl::~MachineImpl()
{
  stop();
}

MachineInspector v0::MachineImpl::get_inspector() noexcept { return MachineInspector().inspect( ram ).inspect( cpu ); }

std::uint32_t v0::MachineImpl::start() noexcept { return cpu.start(); }

void v0::MachineImpl::stop() noexcept { cpu.stop(); }

std::uint32_t v0::MachineImpl::single_step() noexcept { return cpu.single_step(); }

void v0::MachineImpl::reset() noexcept { cpu.hard_reset(); }

IODevice* v0::MachineImpl::swap_io_device( IODevice *device ) noexcept { return cpu.attach_iodevice( device ); }

FileHandler* v0::MachineImpl::swap_file_handler( FileHandler *handler ) noexcept { return cpu.attach_file_handler( handler ); }

bool v0::MachineImpl::load( void const * data ) noexcept { return false; }

}