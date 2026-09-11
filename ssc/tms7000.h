// tms7000.h
//
// Standalone, portable emulation of the Texas Instruments TMS7000 /
// TMS7040 microcontroller core, adapted for use as the "PIC7040" engine
// inside a Tandy Speech/Sound Cartridge (SSC) emulation for VCC.
//
// This is a from-source port of the instruction set, addressing modes,
// interrupt logic and internal timer logic documented in MAME's
// src/devices/cpu/tms7000/tms7000.cpp / tms7000.h / tms7000op.cpp
// (license: BSD-3-Clause, copyright hap, Tim Lindner). The MAME device
// framework (device_t, address_map, emu_timer, attotime, save_item,
// debugger hooks, disassembler) has been stripped out and replaced with
// plain callbacks and a cycle-budget Execute() loop so the core can run
// standalone inside a Windows DLL with no emulator framework underneath
// it. The opcode table, cycle counts, flag behavior and timer math are
// preserved from the original as closely as possible.
//
// Only the TMS7040 configuration is implemented: 128 bytes of internal
// register-file RAM at $0000-$007F, the peripheral file at $0100-$010B
// (I/O control, one timer, ports A-D), and 4KB of internal ROM at
// $F000-$FFFF. This matches how the SSC's PIC7040 is wired: it uses no
// external memory bus, only its four I/O ports.
//
// license:BSD-3-Clause
// Original copyright-holders: hap, Tim Lindner (MAME tms7000 core)
// Port for VCC SSC cartridge: 2026

#pragma once
#include <cstdint>
#include <cstring>
#include <functional>

namespace ssc {

// External interrupt line identifiers (INT2/4/5 are internally generated
// by the timer logic and are not exposed here).
enum class Tms7000Irq {
    Int1 = 0,   // wired to SP0256 DRQ in the SSC
    Int3 = 1,   // wired to "host wrote a command byte" in the SSC
};

class Tms7000 {
public:
    Tms7000();

    // --- host wiring -------------------------------------------------
    // Internal ROM image (4096 bytes) must be supplied before Reset().
    void LoadRom(const uint8_t* data, size_t len);

    // Port callbacks. Port A is input-only (host -> chip), Port B is
    // output-only, Ports C and D are bidirectional, matching the real
    // TMS7040 wiring used by the SSC.
    std::function<uint8_t()>            InPortA;
    std::function<void(uint8_t)>        OutPortB;
    std::function<uint8_t()>            InPortC;
    std::function<void(uint8_t)>        OutPortC;
    std::function<uint8_t()>            InPortD;
    std::function<void(uint8_t)>        OutPortD;

    // --- execution -----------------------------------------------------
    void Reset();

    // Runs instructions until at least `cycles` worth of icount has been
    // consumed (icount units == input-clock cycles / 2, matching the
    // TMS7000's fixed /2 internal clock divider). Returns the number of
    // cycles actually consumed (>= cycles, since instructions are atomic).
    int Execute(int cycles);

    // External interrupt line control (level-sensitive, active-high
    // "asserted" semantics matching MAME's execute_set_input: call with
    // asserted=true to assert, false to clear).
    void SetIrqLine(Tms7000Irq line, bool asserted);

    // Debug/introspection
    uint16_t PC() const { return m_pc; }
    uint8_t  PeekReg(uint8_t r) const { return m_rf[r & 0x7f]; }
    // Count of undefined-opcode fetches since Reset() -- a real firmware
    // image running correctly should keep this at 0; a nonzero count is
    // a strong signal the CPU has run off into data or a decode bug.
    uint32_t IllegalCount() const { return m_illegal_count; }

private:
    // ---- addressable state ----
    uint8_t  m_rf[128];       // internal register file RAM ($00-$7F); R0=A, R1=B
    uint8_t  m_rom[4096];     // internal ROM ($F000-$FFFF)
    bool     m_rom_loaded = false;

    uint16_t m_pc = 0;
    uint32_t m_illegal_count = 0;
    uint8_t  m_sp = 0;
    uint8_t  m_sr = 0;        // status register: C N Z I . . . .
    uint8_t  m_op = 0;
    int      m_icount = 0;

    bool     m_irq_state[2] = { false, false };
    bool     m_idle_state = false;

    uint8_t  m_io_control[2] = { 0, 0 };   // IOCNT0 (only [0] used by TMS7040)

    // one hardware timer (TMS7040 family only implements timer 1)
    uint8_t  m_timer_data = 0;
    uint8_t  m_timer_control = 0;
    int      m_timer_decrementer = 0;
    int      m_timer_prescaler = 0;
    uint16_t m_timer_capture_latch = 0;
    bool     m_timer_running = false;
    long     m_timer_ticks_remaining = 0;  // icount ticks until next prescaler decrement

    uint8_t  m_port_latch[4] = { 0, 0, 0, 0 };  // A,B,C,D output latches
    uint8_t  m_port_ddr[4]   = { 0, 0, 0, 0 };  // A,B,C,D direction regs

    // ---- memory access ----
    uint8_t  Read8(uint16_t address);
    void     Write8(uint16_t address, uint8_t data);
    uint8_t  ReadR8(uint8_t r) { return Read8(r); }
    void     WriteR8(uint8_t r, uint8_t v) { Write8(r, v); }
    uint16_t ReadR16(uint8_t r) { return (uint16_t(Read8((r - 1) & 0xff)) << 8) | Read8(r); }
    void     WriteR16(uint8_t r, uint16_t v) { Write8((r - 1) & 0xff, v >> 8); Write8(r, v & 0xff); }

    uint8_t  ReadP(uint8_t offset);
    void     WriteP(uint8_t offset, uint8_t data);

    uint8_t  Imm8() { return Read8(m_pc++); }
    uint16_t Imm16() { uint16_t hi = Read8(m_pc++); return (hi << 8) | Read8(m_pc++); }

    uint8_t  Pull8() { return Read8(m_sp--); }
    void     Push8(uint8_t data) { Write8(++m_sp, data); }
    uint16_t Pull16() { uint16_t lo = Read8(m_sp--); return lo | (uint16_t(Read8(m_sp--)) << 8); }
    void     Push16(uint16_t data) { Write8(++m_sp, data >> 8 & 0xff); Write8(++m_sp, data & 0xff); }

    // ---- status flags ----
    enum { SR_C = 0x80, SR_N = 0x40, SR_Z = 0x20, SR_I = 0x10 };

    // ---- interrupts ----
    void FlagExtInterrupt(int extline);
    void CheckInterrupts();
    void DoInterrupt(int irqline);

    // ---- timer ----
    void TimerRun();
    void TimerReload();
    void TimerTickLow();
    void TickTimers(int elapsed_icount);

    // ---- opcode dispatch ----
    void ExecuteOne(uint8_t op);

    typedef int (Tms7000::*OpFunc)(uint8_t, uint8_t);
    int OpClr(uint8_t, uint8_t);
    int OpDec(uint8_t, uint8_t);
    int OpInc(uint8_t, uint8_t);
    int OpInv(uint8_t, uint8_t);
    int OpRl(uint8_t, uint8_t);
    int OpRlc(uint8_t, uint8_t);
    int OpRr(uint8_t, uint8_t);
    int OpRrc(uint8_t, uint8_t);
    int OpSwap(uint8_t, uint8_t);
    int OpXchb(uint8_t, uint8_t);
    int OpAdc(uint8_t, uint8_t);
    int OpAdd(uint8_t, uint8_t);
    int OpAnd(uint8_t, uint8_t);
    int OpCmp(uint8_t, uint8_t);
    int OpDac(uint8_t, uint8_t);
    int OpDsb(uint8_t, uint8_t);
    int OpMpy(uint8_t, uint8_t);
    int OpMov(uint8_t, uint8_t);
    int OpOr(uint8_t, uint8_t);
    int OpSbb(uint8_t, uint8_t);
    int OpSub(uint8_t, uint8_t);
    int OpXor(uint8_t, uint8_t);
    int OpDjnz(uint8_t, uint8_t);
    int OpBtjo(uint8_t, uint8_t);
    int OpBtjz(uint8_t, uint8_t);

    void AmA(OpFunc op);
    void AmB(OpFunc op);
    void AmR(OpFunc op);
    void AmA2a(OpFunc op);
    void AmA2b(OpFunc op);
    void AmA2r(OpFunc op);
    void AmA2p(OpFunc op);
    void AmB2a(OpFunc op);
    void AmB2b(OpFunc op);
    void AmB2r(OpFunc op);
    void AmB2p(OpFunc op);
    void AmR2a(OpFunc op);
    void AmR2b(OpFunc op);
    void AmR2r(OpFunc op);
    void AmI2a(OpFunc op);
    void AmI2b(OpFunc op);
    void AmI2r(OpFunc op);
    void AmI2p(OpFunc op);
    void AmP2a(OpFunc op);
    void AmP2b(OpFunc op);

    void ShortBranch(bool check);
    void Jmp(bool check);
    void BrDir(); void BrInx(); void BrInd();
    void CallDir(); void CallInx(); void CallInd();
    void CmpaDir(); void CmpaInx(); void CmpaInd();
    void DecdA(); void DecdB(); void DecdR();
    void Dint(); void Eint(); void Idle();
    void LdaDir(); void LdaInx(); void LdaInd();
    void Ldsp();
    void MovdDir(); void MovdInx(); void MovdInd();
    void Nop();
    void PopA(); void PopB(); void PopR(); void PopSt();
    void PushA(); void PushB(); void PushR(); void PushSt();
    void Reti(); void Rets();
    void Setc();
    void StaDir(); void StaInx(); void StaInd();
    void Stsp();
    void Trap(uint8_t address);
    void Illegal(uint8_t op);
};

} // namespace ssc
