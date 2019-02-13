#pragma once

#include <cstdint>

namespace mips32
{
class CP0
{
  friend class MachineInspector;

public:
  void reset() noexcept;

  void write( std::uint32_t reg, std::uint32_t sel, std::uint32_t data ) noexcept;

  std::uint32_t read( std::uint32_t reg, std::uint32_t sel ) noexcept;

  std::uint32_t
    user_local,
    hwr_ena,
    bad_vaddr,
    bad_instr,
    status,
    int_ctl,
    srs_ctl,
    cause,
    epc,
    pr_id,
    e_base,
    config[5],
    error_epc,
    k_scratch[8];
};
} // namespace mips32
