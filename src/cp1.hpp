#pragma once

#include <mips32/fpr.hpp>

#include <array>
#include <cfenv>
#include <cstdint>

namespace mips32
{

/**
 * MIPS32 FPU
 *
 * Has 32 register of 64-bit width.
 *
 * It heavily relies on the hardware FPU,
 * so every calculation is IEEE 754 compliant
 * based on the hardware FPU compliance itself.
 **/
class CP1
{
  friend class MachineInspector;

public:
  CP1() noexcept;

  ~CP1() noexcept;

  // Resets the FPU to its default state.
  void reset() noexcept;

  // Reads a FPU's register.
  std::uint32_t read( std::uint32_t reg ) noexcept;

  // Overwrites an FPU's register.
  // Writes to read only fields are ignored.
  void write( std::uint32_t reg, std::uint32_t data ) noexcept;

  // Trapped exception that will be signaled to the CPU.
  enum Exception : std::uint32_t
  {
    NONE = 0x00,
    UNIMPLEMENTED = 0x20,
    INVALID = 0x10,
    DIVBYZERO = 0x08,
    OVERFLOW_ = 0x04,
    UNDERFLOW_ = 0x02,
    INEXACT = 0x01,
    RESERVED = 0xFFFF'FFFF,
  };

  // Execute an instruction
  Exception execute( std::uint32_t word ) noexcept;

  // Move operations CPU <--> CP1

  std::uint32_t mfc1( std::uint32_t reg ) noexcept;
  std::uint32_t mfhc1( std::uint32_t reg ) noexcept;

  void mtc1( std::uint32_t reg, std::uint32_t word ) noexcept;
  void mthc1( std::uint32_t reg, std::uint32_t word ) noexcept;

private:
// Set the underlying FPU rounding mode based on the RN field in FCSR.
  void set_round_mode() noexcept;
  // Set the underlying FPU to flush to zero or not denormalized numbers
  // based on the FS flag in FCSR.
  void set_denormal_flush() noexcept;

  // Getter and Setter for the fields of FCSR.

  std::uint32_t round() const noexcept;
  std::uint32_t flags() const noexcept;
  std::uint32_t enable() const noexcept;
  std::uint32_t cause() const noexcept;

  void set_flags( std::uint32_t flag ) noexcept;
  void set_cause( std::uint32_t data ) noexcept;

  // Check if the FPU raised any exception and
  // sets the needed flags in the cause field.
  // Returns true to signal a trap.
  bool handle_fpu_ex() noexcept;

  /****************
   *              *
   * INSTRUCTIONS *
   *              *
   ****************/

  int reserved( std::uint32_t word ) noexcept;
  int unimplemented( std::uint32_t word ) noexcept;

  int add( std::uint32_t word ) noexcept;
  int sub( std::uint32_t word ) noexcept;
  int mul( std::uint32_t word ) noexcept;
  int div( std::uint32_t word ) noexcept;
  int sqrt( std::uint32_t word ) noexcept;
  int abs( std::uint32_t word ) noexcept;
  int mov( std::uint32_t word ) noexcept;
  int neg( std::uint32_t word ) noexcept;
  int round_l( std::uint32_t word ) noexcept;
  int trunc_l( std::uint32_t word ) noexcept;
  int ceil_l( std::uint32_t word ) noexcept;
  int floor_l( std::uint32_t word ) noexcept;
  int round_w( std::uint32_t word ) noexcept;
  int trunc_w( std::uint32_t word ) noexcept;
  int ceil_w( std::uint32_t word ) noexcept;
  int floor_w( std::uint32_t word ) noexcept;
  int sel( std::uint32_t word ) noexcept;
  int seleqz( std::uint32_t word ) noexcept;
  int recip( std::uint32_t word ) noexcept;
  int rsqrt( std::uint32_t word ) noexcept;
  int selnez( std::uint32_t word ) noexcept;
  int maddf( std::uint32_t word ) noexcept;
  int msubf( std::uint32_t word ) noexcept;
  int rint( std::uint32_t word ) noexcept;
  int class_( std::uint32_t word ) noexcept;
  int min( std::uint32_t word ) noexcept;
  int max( std::uint32_t word ) noexcept;
  int mina( std::uint32_t word ) noexcept;
  int maxa( std::uint32_t word ) noexcept;
  int cvt_s( std::uint32_t word ) noexcept;
  int cvt_d( std::uint32_t word ) noexcept;
  int cvt_l( std::uint32_t word ) noexcept;
  int cvt_w( std::uint32_t word ) noexcept;
  int cmp_af( std::uint32_t word ) noexcept;
  int cmp_un( std::uint32_t word ) noexcept;
  int cmp_eq( std::uint32_t word ) noexcept;
  int cmp_ueq( std::uint32_t word ) noexcept;
  int cmp_lt( std::uint32_t word ) noexcept;
  int cmp_ult( std::uint32_t word ) noexcept;
  int cmp_le( std::uint32_t word ) noexcept;
  int cmp_ule( std::uint32_t word ) noexcept;
  int cmp_or( std::uint32_t word ) noexcept;
  int cmp_une( std::uint32_t word ) noexcept;
  int cmp_ne( std::uint32_t word ) noexcept;

  /* Signaling NaN is not supported
  int cabs_saf( std::uint32_t word ) noexcept;
  int cabs_sun( std::uint32_t word ) noexcept;
  int cabs_seq( std::uint32_t word ) noexcept;
  int cabs_sueq( std::uint32_t word ) noexcept;
  int cabs_slt( std::uint32_t word ) noexcept;
  int cabs_sult( std::uint32_t word ) noexcept;
  int cabs_sle( std::uint32_t word ) noexcept;
  int cabs_sule( std::uint32_t word ) noexcept;
  */

  std::array<FPR, 32> fpr;

  std::uint32_t fir, fcsr;

  // Floating Point Environment
  std::fenv_t env;
};

} // namespace mips32
