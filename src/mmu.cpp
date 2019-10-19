#include "mmu.hpp"
#include "ram.hpp"

namespace mips32
{
MMU::MMU( RAM &ram, std::initializer_list<Segment> segments ) noexcept
  : ram( ram ), segments( segments )
{}

std::uint32_t *MMU::access( std::uint32_t address, std::uint32_t access_flags ) noexcept
{
  for ( auto const &segment : segments )
  {
  // 1
    if ( segment.contains( address ) && segment.has_access( access_flags ) )
    {
      return &ram[address];
    }
  }

  // 2
  return nullptr;
}

} // namespace mips32