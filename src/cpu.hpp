#pragma once

#include <mips32/file_handler.hpp>
#include <mips32/io_device.hpp>
#include <mips32/cp0.hpp>
#include "cp1.hpp"
#include "mmu.hpp"
#include "ram.hpp"
#include "ram_io.hpp"

#include <array>
#include <atomic>
#include <cstdint>

namespace mips32
{
class CPU
{
  friend class MachineInspector;

public:
  explicit CPU( RAM &ram ) noexcept;

  IODevice* attach_iodevice( IODevice *device ) noexcept;
  FileHandler* attach_file_handler( FileHandler *handler ) noexcept;

  enum ExitCode : std::uint32_t
  {
    NONE,
    MANUAL_STOP,
    INTERRUPT,
    EXCEPTION,
    EXIT,
  };

  enum ExCause : std::uint32_t
  {
    Int = 0x00,
    AdEL = 0x04,
    AdES = 0x05,
    IBE = 0x06,
    DBE = 0x07,
    Sys = 0x08,
    Bp = 0x09,
    RI = 0x0A,
    CpU = 0x0B,
    Ov = 0x0C,
    Tr = 0x0D,
    FPE = 0x0F,
  };

  std::uint32_t start() noexcept;
  void          stop() noexcept;

  std::uint32_t single_step() noexcept;

  void hard_reset() noexcept;

private:
  RAMIO string_handler;

  MMU mmu;
  CP1 cp1;
  CP0 cp0;

  std::uint32_t pc;

  std::array<std::uint32_t, 32> gpr;

  std::atomic<std::uint32_t> exit_code;

  IODevice* io_device;
  FileHandler* file_handler;

  void reserved( std::uint32_t word ) noexcept;

  void special( std::uint32_t word ) noexcept;
  void regimm( std::uint32_t word ) noexcept;
  void j( std::uint32_t word ) noexcept;
  void jal( std::uint32_t word ) noexcept;
  void beq( std::uint32_t word ) noexcept;
  void bne( std::uint32_t word ) noexcept;
  void pop06( std::uint32_t word ) noexcept;
  void pop07( std::uint32_t word ) noexcept;
  void pop10( std::uint32_t word ) noexcept;
  void addiu( std::uint32_t word ) noexcept;
  void slti( std::uint32_t word ) noexcept;
  void sltiu( std::uint32_t word ) noexcept;
  void andi( std::uint32_t word ) noexcept;
  void ori( std::uint32_t word ) noexcept;
  void xori( std::uint32_t word ) noexcept;
  void aui( std::uint32_t word ) noexcept;
  void cop0( std::uint32_t word ) noexcept;
  void cop1( std::uint32_t word ) noexcept;
  void pop26( std::uint32_t word ) noexcept;
  void pop27( std::uint32_t word ) noexcept;
  void pop30( std::uint32_t word ) noexcept;
  void special3( std::uint32_t word ) noexcept;
  void lb( std::uint32_t word ) noexcept;
  void lh( std::uint32_t word ) noexcept;
  void lw( std::uint32_t word ) noexcept;
  void lbu( std::uint32_t word ) noexcept;
  void lhu( std::uint32_t word ) noexcept;
  void sb( std::uint32_t word ) noexcept;
  void sh( std::uint32_t word ) noexcept;
  void sw( std::uint32_t word ) noexcept;
  void lwc1( std::uint32_t word ) noexcept;
  void bc( std::uint32_t word ) noexcept;
  void ldc1( std::uint32_t word ) noexcept;
  void pop66( std::uint32_t word ) noexcept;
  void swc1( std::uint32_t word ) noexcept;
  void balc( std::uint32_t word ) noexcept;
  void pcrel( std::uint32_t word ) noexcept;
  void sdc1( std::uint32_t word ) noexcept;
  void pop76( std::uint32_t word ) noexcept;

  /* * * * * *
   *         *
   * SPECIAL *
   *         *
   * * * * * */
  void sll( std::uint32_t word ) noexcept;
  void srl( std::uint32_t word ) noexcept;
  void sra( std::uint32_t word ) noexcept;
  void sllv( std::uint32_t word ) noexcept;
  void lsa( std::uint32_t word ) noexcept;
  void srlv( std::uint32_t word ) noexcept;
  void srav( std::uint32_t word ) noexcept;
  void jalr( std::uint32_t word ) noexcept;
  void syscall( std::uint32_t word ) noexcept;
  void break_( std::uint32_t ) noexcept;
  void clz( std::uint32_t word ) noexcept;
  void clo( std::uint32_t word ) noexcept;
  void sop30( std::uint32_t word ) noexcept;
  void sop31( std::uint32_t word ) noexcept;
  void sop32( std::uint32_t word ) noexcept;
  void sop33( std::uint32_t word ) noexcept;
  void add( std::uint32_t word ) noexcept;
  void addu( std::uint32_t word ) noexcept;
  void sub( std::uint32_t word ) noexcept;
  void subu( std::uint32_t word ) noexcept;
  void and_( std::uint32_t word ) noexcept;
  void or_( std::uint32_t word ) noexcept;
  void xor_( std::uint32_t word ) noexcept;
  void nor_( std::uint32_t word ) noexcept;
  void slt( std::uint32_t word ) noexcept;
  void sltu( std::uint32_t word ) noexcept;
  void tge( std::uint32_t word ) noexcept;
  void tgeu( std::uint32_t word ) noexcept;
  void tlt( std::uint32_t word ) noexcept;
  void tltu( std::uint32_t word ) noexcept;
  void teq( std::uint32_t word ) noexcept;
  void seleqz( std::uint32_t word ) noexcept;
  void tne( std::uint32_t word ) noexcept;
  void selnez( std::uint32_t word ) noexcept;

  /* * * * * *
   *         *
   * REGIMM  *
   *         *
   * * * * * */
  void bltz( std::uint32_t word ) noexcept;
  void bgez( std::uint32_t word ) noexcept;
  void nal( std::uint32_t ) noexcept;
  void bal( std::uint32_t word ) noexcept;
  void sigrie( std::uint32_t word ) noexcept;

  /* * * * *
   *       *
   * COP0  *
   *       *
   * * * * */
  void mfc0( std::uint32_t word ) noexcept;
  void mfhc0( std::uint32_t word ) noexcept;
  void mtc0( std::uint32_t word ) noexcept;
  void mthc0( std::uint32_t ) noexcept;
  void mfmc0( std::uint32_t word ) noexcept;
  void eret( std::uint32_t ) noexcept;

  /* * * * *
   *       *
   * PCREL *
   *       *
   * * * * */
  void auipc( std::uint32_t word ) noexcept;
  void aluipc( std::uint32_t word ) noexcept;
  void addiupc( std::uint32_t word ) noexcept;
  void lwpc( std::uint32_t word ) noexcept;
  void lwupc( std::uint32_t word ) noexcept;
  void ldpc( std::uint32_t word ) noexcept;

  /* * * * * * *
   *           *
   *  SPECIAL3 *
   *           *
   * * * * * * */
  void ext( std::uint32_t word ) noexcept;
  void ins( std::uint32_t word ) noexcept;

  std::uint32_t running_mode() noexcept;

  template <int op, int extend>
  void op_byte( std::uint32_t word ) noexcept;

  template <int op, int extend>
  void op_halfword( std::uint32_t word ) noexcept;

  template <int op>
  void op_word( std::uint32_t word ) noexcept;

  template <int op>
  void op_word( std::uint32_t _rt, std::uint32_t address, std::uint32_t word ) noexcept;

  void enter_kernel_mode() noexcept;
  void enter_user_mode() noexcept;

  void set_ex_cause( std::uint32_t ex ) noexcept;
  void signal_exception( std::uint32_t ex, std::uint32_t word, std::uint32_t pc ) noexcept;

  using method_ptr = void ( CPU::* )( std::uint32_t ) noexcept;

  static inline constexpr std::array<method_ptr, 64> function_table{
      &CPU::special,
      &CPU::regimm,
      &CPU::j,
      &CPU::jal,
      &CPU::beq,
      &CPU::bne,
      &CPU::pop06,
      &CPU::pop07,
      &CPU::pop10,
      &CPU::addiu,
      &CPU::slti,
      &CPU::sltiu,
      &CPU::andi,
      &CPU::ori,
      &CPU::xori,
      &CPU::aui,
      &CPU::cop0,
      &CPU::cop1,
      &CPU::reserved, // COP2
      &CPU::reserved, // COP1X
      &CPU::reserved, // BEQL
      &CPU::reserved, // BNEL
      &CPU::pop26,
      &CPU::pop27,
      &CPU::pop30,
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // SPECIAL2
      &CPU::reserved, // JALX
      &CPU::reserved, // MSA
      &CPU::special3,
      &CPU::lb,
      &CPU::lh,
      &CPU::reserved, // LWL
      &CPU::lw,
      &CPU::lbu,
      &CPU::lhu,
      &CPU::reserved, // LWR
      &CPU::reserved, // beta
      &CPU::sb,
      &CPU::sh,
      &CPU::reserved, // SWL
      &CPU::sw,
      &CPU::reserved, // beta
      &CPU::reserved, // beta
      &CPU::reserved, // SWR
      &CPU::reserved, // CACHE (moved)
      &CPU::reserved, // LL (moved)
      &CPU::lwc1,
      &CPU::bc,
      &CPU::reserved, // PREF (moved)
      &CPU::reserved, // beta
      &CPU::ldc1,
      &CPU::pop66,
      &CPU::reserved, // beta
      &CPU::reserved, // SC (moved)
      &CPU::swc1,
      &CPU::balc,
      &CPU::pcrel,
      &CPU::reserved, // beta
      &CPU::sdc1,
      &CPU::pop76,
      &CPU::reserved, // beta
  };
};
} // namespace mips32