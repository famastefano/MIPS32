#include <mips32/cp0.hpp>

#include <cassert>
#include <cstring>

namespace mips32 {
/*
k_scratch[6];
*/
void CP0::reset() noexcept
{
  *this = {};

  /*
  register name
  bit sequence -> value
  */

  /*
  Status
  29 -> 1
  26 -> 1
  22 -> 1
   2 -> 1
  */
  status = 0x2440'0004;

  /*
  IntCtl
  31..29 -> HW0 -> 2
  */
  int_ctl = 0xC000'0000;

  /*
  SRSCtl
  29..26 -> # of Shadow GPRs sets -> 0 = only standard GPRs available
  */

  /*
  PrId
  23..16 -> CompanyID    -> not a MIPS32/64 CPU   -> 0
   15..8 -> Processor ID -> implementation number -> 0
    7..0 -> Revision     -> should be incremented -> #
  */

  /*
  EBase
  31   -> 1
  9..0 -> # CPU -> 0
  */
  e_base = 0x8000'0000;

  /*
  Config
      15 -> endianess       -> 0/1 Little/Big endian -> 0
  14..13 -> Instruction Set -> 0 MIPS32
  12..10 -> Release         -> Relase 6 -> 2
    9..7 -> MMU Type        -> None -> 0
       3 -> 0
  */
  config[0] = 0x0000'0400;

  /*
  Config1
  31 -> Config2 present       -> 1
  0  -> FPU Implemented (CP1) -> 1
  */
  config[1] = 0x8000'0001;

  /*
  Config2
  31 -> Config3 present -> 1
  */
  config[2] = 0x8000'0000;

  /*
  Config3
      31 -> Config4 present          -> 1
      27 -> BadInstrP                -> 1
      26 -> BadInstr                 -> 1
      13 -> UserLocal                -> 1
      12                             -> 1
  */
  config[3] = 0x8C00'2000;

  /*
  Config4
      31 -> Config5 present                -> 0
  23..16 -> bitmask kernel scratch present -> all 1
  */
  config[4] = 0x00FF'0000;
}

void CP0::write( std::uint32_t reg, std::uint32_t sel, std::uint32_t data ) noexcept
{
  if ( reg == 4 ) {

    if ( sel == 2 ) user_local = data;

  } else if ( reg == 7 ) {

    if ( sel == 0 ) hwr_ena = data;

  } else if ( reg == 8 ) {

    if ( sel == 0 ) return; // bad_vaddr is read only
    if ( sel == 1 ) return; // bad_instr is read only

  } else if ( reg == 12 ) {

    if ( sel == 0 ) {
      status = status & ~0x3040'FF13 | data & 0x3040FF13;
      return;
    }
    if ( sel == 1 ) {
      data &= 0x3E0;

      switch ( data >> 5 ) {
      case 0x01:
      case 0x02:
      case 0x04:
      case 0x08:
      case 0x10:
        int_ctl = int_ctl & ~0x3E0 | data;
      }

      return;
    }
    if ( sel == 2 ) {
      srs_ctl = srs_ctl & ~0xF3C0 | data & 0xF3C0;
      return;
    }
  } else if ( reg == 13 ) {

    if ( sel == 0 ) return; // cause is read only
  } else if ( reg == 14 ) {

    if ( sel == 0 ) epc = data;
  } else if ( reg == 15 ) {

    if ( sel == 0 ) return; // pr_id is read only
    if ( sel == 1 ) {
      // Write Gate enabled
      if ( e_base & ( 1 << 11 ) ) e_base = e_base & ~0x3FFF'F000 | data & 0x3FFF'F000;
      return;
    }
  } else if ( reg == 16 ) {

    if ( sel < 5 ) return; // config registers are read only
  } else if ( reg == 30 ) {

    if ( sel == 0 ) error_epc = data;
  } else if ( reg == 31 ) {

    if ( sel > 1 && sel < 8 ) k_scratch[sel] = data;
  }

  assert( "Unimplemented Coprocessor 0 Register." );
}

std::uint32_t CP0::read( std::uint32_t reg, std::uint32_t sel ) noexcept
{
  if ( reg == 4 ) {

    if ( sel == 2 ) return user_local;

  } else if ( reg == 7 ) {

    if ( sel == 0 ) return hwr_ena;

  } else if ( reg == 8 ) {

    if ( sel == 0 ) return bad_vaddr;
    if ( sel == 1 ) return bad_instr;

  } else if ( reg == 12 ) {

    if ( sel == 0 ) return status;
    if ( sel == 1 ) return int_ctl;
    if ( sel == 2 ) return srs_ctl;

  } else if ( reg == 13 ) {

    if ( sel == 0 ) return cause;

  } else if ( reg == 14 ) {

    if ( sel == 0 ) return epc;

  } else if ( reg == 15 ) {

    if ( sel == 0 ) return pr_id;
    if ( sel == 1 ) return e_base;

  } else if ( reg == 16 ) {

    if ( sel < 5 ) return config[sel];

  } else if ( reg == 30 ) {

    if ( sel == 0 ) return error_epc;

  } else if ( reg == 31 ) {

    if ( sel > 1 && sel < 8 ) return k_scratch[sel];
  }

  assert( "Unimplemented Coprocessor 0 Register." );

  return 0;
}

} // namespace mips32