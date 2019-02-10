#include <mips32/ram.hpp>

#include <memory>
#include <utility>

namespace mips32
{
class RAMIO
{
public:
  constexpr RAMIO( RAM &ram ) noexcept : ram( ram ) {}

  std::unique_ptr<char[]> read( std::uint32_t address, std::uint32_t count ) const noexcept;

  void write( std::uint32_t address, char const *src, std::uint32_t count ) noexcept;

private:
  std::pair<std::uint32_t, bool> get_block( std::uint32_t address ) const noexcept;

  RAM &ram;
};
} // namespace mips32