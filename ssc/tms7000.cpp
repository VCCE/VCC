// tms7000.cpp
// See tms7000.h for provenance/license notes. This is a mechanical port of
// MAME's tms7000.cpp / tms7000op.cpp opcode semantics and cycle counts to a
// standalone class with no emulator framework dependency.

#include "tms7000.h"

namespace ssc {

Tms7000::Tms7000()
{
    std::memset(m_rf, 0, sizeof(m_rf));
    std::memset(m_rom, 0, sizeof(m_rom));
}

void Tms7000::LoadRom(const uint8_t* data, size_t len)
{
    std::memset(m_rom, 0xff, sizeof(m_rom));
    if (data && len) {
        size_t n = len < sizeof(m_rom) ? len : sizeof(m_rom);
        std::memcpy(m_rom, data, n);
    }
    m_rom_loaded = true;
}

// ---------------------------------------------------------------------
// memory map:
//   $0000-$007F  128 bytes internal register-file RAM
//   $0080-$00FF  unmapped register-file addresses (reads as 0, writes ignored)
//   $0100-$010B  peripheral file (IOCNT0, timer 1, ports A-D)
//   $010C-$EFFF  unused / open bus (reads as 0xFF, writes ignored)
//   $F000-$FFFF  4KB internal ROM
// ---------------------------------------------------------------------

uint8_t Tms7000::Read8(uint16_t address)
{
    if (address < 0x0080) return m_rf[address];
    if (address < 0x0100) return 0x00;                 // unmapped rf
    if (address < 0x010c) return ReadP(uint8_t(address - 0x0100));
    if (address >= 0xf000) return m_rom[address - 0xf000];
    return 0xff;                                        // open bus
}

void Tms7000::Write8(uint16_t address, uint8_t data)
{
    if (address < 0x0080) { m_rf[address] = data; return; }
    if (address < 0x0100) { return; }                    // unmapped rf
    if (address < 0x010c) { WriteP(uint8_t(address - 0x0100), data); return; }
    // ROM and open bus: writes ignored
}

// ---------------------------------------------------------------------
// peripheral file
// ---------------------------------------------------------------------

uint8_t Tms7000::ReadP(uint8_t offset)
{
    switch (offset)
    {
        case 0x00: return m_io_control[0];

        case 0x02: return uint8_t(m_timer_decrementer);

        case 0x03: return uint8_t(m_timer_capture_latch);

        case 0x04: // port A data (input-only from host)
            return (InPortA ? InPortA() : 0xff);

        case 0x06: // port B data (write-only; reads back output latch)
            return m_port_latch[1];

        case 0x08: // port C data
            return (InPortC ? (InPortC() & ~m_port_ddr[2]) : 0) | (m_port_latch[2] & m_port_ddr[2]);

        case 0x09: return m_port_ddr[2];

        case 0x0a: // port D data
            return (InPortD ? (InPortD() & ~m_port_ddr[3]) : 0) | (m_port_latch[3] & m_port_ddr[3]);

        case 0x0b: return m_port_ddr[3];

        default: return 0;
    }
}

void Tms7000::WriteP(uint8_t offset, uint8_t data)
{
    switch (offset)
    {
        case 0x00: // IOCNT0
            // d0,d2,d4: INT1,2,3 enable    d1,d3,d5: INT1,2,3 flag (write-1-clears)
            m_io_control[0] = (m_io_control[0] & (~data & 0x2a)) | (data & 0xd5);
            if (data & 0x02) FlagExtInterrupt(0);
            if (data & 0x20) FlagExtInterrupt(1);
            CheckInterrupts();
            break;

        case 0x02: // timer data (reload value)
            m_timer_data = data;
            break;

        case 0x03: // timer control
            m_timer_control = data;
            TimerReload();
            break;

        case 0x04: // port A: no write / no ddr on TMS7000
            break;

        case 0x06: // port B data (write only, ddr fixed to 0xff)
            if (OutPortB) OutPortB(data);
            m_port_latch[1] = data;
            break;

        case 0x08: // port C data
            if (OutPortC) OutPortC(data & m_port_ddr[2]);
            m_port_latch[2] = data;
            break;

        case 0x09: // port C ddr
            m_port_ddr[2] = data;
            break;

        case 0x0a: // port D data
            if (OutPortD) OutPortD(data & m_port_ddr[3]);
            m_port_latch[3] = data;
            break;

        case 0x0b: // port D ddr
            m_port_ddr[3] = data;
            break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------
// reset / interrupts
// ---------------------------------------------------------------------

void Tms7000::Reset()
{
    if (m_idle_state) { m_pc++; m_idle_state = false; }

    WriteP(0x04, 0xff); // port a (no-op, kept for parity with original)
    WriteP(0x06, 0xff); // port b -> drives OutPortB(0xff)
    m_port_ddr[0] = 0x00;
    m_port_ddr[2] = 0x00;
    m_port_ddr[3] = 0x00;
    WriteP(0x08, 0xff); // port c -> drives OutPortC(0)  (ddr==0 so masked to 0)
    WriteP(0x0a, 0xff); // port d -> drives OutPortD(0)

    m_sr = 0;
    WriteP(0x00, 0x00); // IOCNT0

    m_sp = 0xff;
    m_op = 0xff;
    ExecuteOne(m_op);
    m_icount -= 3; // 17 total, matching MAME's device_reset()

    m_irq_state[0] = m_irq_state[1] = false;
    m_timer_running = false;
    m_timer_ticks_remaining = 0;
}

void Tms7000::SetIrqLine(Tms7000Irq line, bool asserted)
{
    int extline = (line == Tms7000Irq::Int1) ? 0 : 1;

    if (m_irq_state[extline] != asserted)
    {
        m_irq_state[extline] = asserted;
        FlagExtInterrupt(extline);

        if (m_irq_state[extline])
        {
            if (extline == 1) // INT3 latches timer capture
                m_timer_capture_latch = uint16_t(m_timer_decrementer);
            CheckInterrupts();
        }
    }
}

void Tms7000::FlagExtInterrupt(int extline)
{
    if (extline != 0 && extline != 1) return;
    if (m_irq_state[extline])
        m_io_control[0] |= (0x02 << (4 * extline));
    else
        m_io_control[0] &= ~(0x02 << (4 * extline));
}

void Tms7000::CheckInterrupts()
{
    if (!(m_sr & SR_I)) return;

    // INT1 (bits 0-1), INT2 (bits 2-3, timer), INT3 (bits 4-5) all live in
    // IOCNT0 on the 70x0 family; INT4/5 (IOCNT1) don't exist on TMS7040.
    for (int irqline = 0; irqline < 3; irqline++)
    {
        int shift = irqline * 2;
        if (((m_io_control[0] >> shift) & 3) == 3)
        {
            m_io_control[0] &= ~(0x02 << shift);
            if (irqline == 0 || irqline == 2)
                FlagExtInterrupt(irqline / 2);
            DoInterrupt(irqline);
            return;
        }
    }
}

void Tms7000::DoInterrupt(int irqline)
{
    if (m_idle_state) { m_icount -= 17; m_pc++; m_idle_state = false; }
    else m_icount -= 19;

    Push8(m_sr);
    Push16(m_pc);
    m_sr = 0;
    m_pc = (uint16_t(Read8(0xfffc - irqline * 2)) << 8) | Read8(0xfffd - irqline * 2);
}

// ---------------------------------------------------------------------
// timer (TMS7040: one internal timer, "automatic reload" mode only —
// this is the mode the SSC firmware uses)
// ---------------------------------------------------------------------

void Tms7000::TimerRun()
{
    m_timer_prescaler = m_timer_control & 0x1f;
    if ((m_timer_control & 0xe0) == 0x80)
    {
        // period, in icount ticks: 16*(prescaler+1) input-clock cycles,
        // divided by the /2 instruction clock divider == 8*(prescaler+1)
        m_timer_ticks_remaining = 8L * (m_timer_prescaler + 1);
        m_timer_running = true;
    }
    else
    {
        m_timer_running = false;
    }
}

void Tms7000::TimerReload()
{
    m_timer_running = false;
    if (m_timer_control & 0x80)
    {
        m_timer_decrementer = m_timer_data;
        TimerRun();
    }
}

void Tms7000::TimerTickLow()
{
    if (--m_timer_decrementer < 0)
    {
        TimerReload();
        m_io_control[0] |= 0x08; // INT2 flag
        CheckInterrupts();
    }
}

void Tms7000::TickTimers(int elapsed_icount)
{
    if (!m_timer_running) return;
    m_timer_ticks_remaining -= elapsed_icount;
    while (m_timer_ticks_remaining <= 0 && m_timer_running)
    {
        TimerTickLow();
        if (m_timer_running)
            m_timer_ticks_remaining += 8L * (m_timer_prescaler + 1);
    }
}

// ---------------------------------------------------------------------
// execute
// ---------------------------------------------------------------------

int Tms7000::Execute(int cycles)
{
    m_icount = cycles;
    CheckInterrupts();

    do
    {
        int before = m_icount;
        m_op = Read8(m_pc++);
        ExecuteOne(m_op);
        TickTimers(before - m_icount);
    } while (m_icount > 0);

    return cycles - m_icount;
}

// ---------------------------------------------------------------------
// flag helpers (kept as free functions operating on m_sr via macros,
// mirroring the original for auditability)
// ---------------------------------------------------------------------

#define GET_C()     (m_sr >> 7 & 1)
#define SET_C(x)    m_sr = (m_sr & 0x7f) | ((x) >> 1 & 0x80)
#define SET_NZ(x)   m_sr = (m_sr & 0x9f) | ((x) >> 1 & 0x40) | (((x) & 0xff) ? 0 : 0x20)
#define SET_CNZ(x)  m_sr = (m_sr & 0x1f) | ((x) >> 1 & 0xc0) | (((x) & 0xff) ? 0 : 0x20)

#define WB_NO -1
#define AM_WB(write_func, address, param1, param2) \
    int result = (this->*op)(param1, param2); \
    if (result > WB_NO) write_func(address, uint8_t(result))

void Tms7000::AmA(OpFunc op)   { m_icount -= 5;  AM_WB(WriteR8, 0, ReadR8(0), 0); }
void Tms7000::AmB(OpFunc op)   { m_icount -= 5;  AM_WB(WriteR8, 1, ReadR8(1), 0); }
void Tms7000::AmR(OpFunc op)   { m_icount -= 7;  uint8_t r = Imm8(); AM_WB(WriteR8, r, ReadR8(r), 0); }
void Tms7000::AmA2a(OpFunc op) { m_icount -= 6;  AM_WB(WriteR8, 0, ReadR8(0), ReadR8(0)); }
void Tms7000::AmA2b(OpFunc op) { m_icount -= 6;  AM_WB(WriteR8, 1, ReadR8(1), ReadR8(0)); }
void Tms7000::AmA2p(OpFunc op) { m_icount -= 10; uint8_t r = Imm8(); AM_WB(WriteP, r, ReadP(r), ReadR8(0)); }
void Tms7000::AmA2r(OpFunc op) { m_icount -= 8;  uint8_t r = Imm8(); AM_WB(WriteR8, r, ReadR8(r), ReadR8(0)); }
void Tms7000::AmB2a(OpFunc op) { m_icount -= 5;  AM_WB(WriteR8, 0, ReadR8(0), ReadR8(1)); }
void Tms7000::AmB2b(OpFunc op) { m_icount -= 6;  AM_WB(WriteR8, 1, ReadR8(1), ReadR8(1)); }
void Tms7000::AmB2r(OpFunc op) { m_icount -= 7;  uint8_t r = Imm8(); AM_WB(WriteR8, r, ReadR8(r), ReadR8(1)); }
void Tms7000::AmB2p(OpFunc op) { m_icount -= 9;  uint8_t r = Imm8(); AM_WB(WriteP, r, ReadP(r), ReadR8(1)); }
void Tms7000::AmR2a(OpFunc op) { m_icount -= 8;  AM_WB(WriteR8, 0, ReadR8(0), ReadR8(Imm8())); }
void Tms7000::AmR2b(OpFunc op) { m_icount -= 8;  AM_WB(WriteR8, 1, ReadR8(1), ReadR8(Imm8())); }
void Tms7000::AmR2r(OpFunc op) { m_icount -= 10; uint8_t param2 = ReadR8(Imm8()); uint8_t r = Imm8(); AM_WB(WriteR8, r, ReadR8(r), param2); }
void Tms7000::AmI2a(OpFunc op) { m_icount -= 7;  AM_WB(WriteR8, 0, ReadR8(0), Imm8()); }
void Tms7000::AmI2b(OpFunc op) { m_icount -= 7;  AM_WB(WriteR8, 1, ReadR8(1), Imm8()); }
void Tms7000::AmI2r(OpFunc op) { m_icount -= 9;  uint8_t param2 = Imm8(); uint8_t r = Imm8(); AM_WB(WriteR8, r, ReadR8(r), param2); }
void Tms7000::AmI2p(OpFunc op) { m_icount -= 11; uint8_t param2 = Imm8(); uint8_t r = Imm8(); AM_WB(WriteP, r, ReadP(r), param2); }
void Tms7000::AmP2a(OpFunc op) { m_icount -= 9;  AM_WB(WriteR8, 0, ReadR8(0), ReadP(Imm8())); }
void Tms7000::AmP2b(OpFunc op) { m_icount -= 8;  AM_WB(WriteR8, 1, ReadR8(1), ReadP(Imm8())); }

#undef AM_WB

// common opcodes -- 1 param
int Tms7000::OpClr(uint8_t, uint8_t) { uint8_t t = 0; SET_CNZ(t); return t; }
int Tms7000::OpDec(uint8_t p1, uint8_t) { uint16_t t = p1 - 1; SET_NZ(t); SET_C(~t); return t; }
int Tms7000::OpInc(uint8_t p1, uint8_t) { uint16_t t = p1 + 1; SET_CNZ(t); return t; }
int Tms7000::OpInv(uint8_t p1, uint8_t) { uint8_t t = ~p1; SET_CNZ(t); return t; }
int Tms7000::OpRl(uint8_t p1, uint8_t)  { uint16_t t = (p1 << 1) | (p1 >> 7); SET_CNZ(t); return t; }
int Tms7000::OpRlc(uint8_t p1, uint8_t) { uint16_t t = (p1 << 1) | GET_C(); SET_CNZ(t); return t; }
int Tms7000::OpRr(uint8_t p1, uint8_t)  { uint16_t t = (p1 >> 1) | (p1 << 8) | (p1 << 7 & 0x80); SET_CNZ(t); return t; }
int Tms7000::OpRrc(uint8_t p1, uint8_t) { uint16_t t = (p1 >> 1) | (p1 << 8) | (GET_C() << 7); SET_CNZ(t); return t; }
int Tms7000::OpSwap(uint8_t p1, uint8_t) { m_icount -= 3; uint16_t t = (p1 >> 4) | (p1 << 4); SET_CNZ(t); return t; }
int Tms7000::OpXchb(uint8_t p1, uint8_t) { m_icount -= 1; uint8_t t = ReadR8(1); SET_CNZ(t); WriteR8(1, p1); return t; }

// 2 params
int Tms7000::OpAdc(uint8_t p1, uint8_t p2) { uint16_t t = p1 + p2 + GET_C(); SET_CNZ(t); return t; }
int Tms7000::OpAdd(uint8_t p1, uint8_t p2) { uint16_t t = p1 + p2; SET_CNZ(t); return t; }
int Tms7000::OpAnd(uint8_t p1, uint8_t p2) { uint8_t t = p1 & p2; SET_CNZ(t); return t; }
int Tms7000::OpCmp(uint8_t p1, uint8_t p2) { uint16_t t = p1 - p2; SET_NZ(t); SET_C(~t); return WB_NO; }
int Tms7000::OpMpy(uint8_t p1, uint8_t p2)
{
    m_icount -= 39;
    uint16_t t = uint16_t(p1) * uint16_t(p2);
    SET_CNZ(t >> 8);
    // Result always lands in the A:B register pair (A=high byte, B=low byte).
    // WriteR16(r, v) writes the high byte to (r-1) and the low byte to r,
    // so r=1 (B) places the high byte in A(0) and the low byte in B(1).
    WriteR16(1, t);
    return WB_NO;
}
int Tms7000::OpMov(uint8_t, uint8_t p2) { uint8_t t = p2; SET_CNZ(t); return t; }
int Tms7000::OpOr(uint8_t p1, uint8_t p2) { uint8_t t = p1 | p2; SET_CNZ(t); return t; }
int Tms7000::OpSbb(uint8_t p1, uint8_t p2) { uint16_t t = p1 - p2 - (!GET_C()); SET_NZ(t); SET_C(~t); return t; }
int Tms7000::OpSub(uint8_t p1, uint8_t p2) { uint16_t t = p1 - p2; SET_NZ(t); SET_C(~t); return t; }
int Tms7000::OpXor(uint8_t p1, uint8_t p2) { uint8_t t = p1 ^ p2; SET_CNZ(t); return t; }

static const uint8_t lut_bcd_out[6] = { 0x00, 0x06, 0x00, 0x66, 0x60, 0x66 };

int Tms7000::OpDac(uint8_t p1, uint8_t p2)
{
    m_icount -= 2;
    int c = GET_C();
    uint8_t h1 = p1 >> 4 & 0xf, l1 = p1 & 0xf;
    uint8_t h2 = p2 >> 4 & 0xf, l2 = p2 & 0xf;
    uint8_t d = ((l1 + l2 + c) < 10) ? 0 : 1;
    if ((h1 + h2) == 9) d |= 2; else if ((h1 + h2) > 9) d |= 4;
    uint8_t t = p1 + p2 + c + lut_bcd_out[d];
    SET_CNZ(t);
    if (d > 2) m_sr |= SR_C;
    return t;
}

int Tms7000::OpDsb(uint8_t p1, uint8_t p2)
{
    m_icount -= 2;
    int c = !GET_C();
    uint8_t h1 = p1 >> 4 & 0xf, l1 = p1 & 0xf;
    uint8_t h2 = p2 >> 4 & 0xf, l2 = p2 & 0xf;
    uint8_t d = ((l1 - c) >= l2) ? 0 : 1;
    if (h1 == h2) d |= 2; else if (h1 < h2) d |= 4;
    uint8_t t = p1 - p2 - c - lut_bcd_out[d];
    SET_CNZ(t);
    if (d <= 2) m_sr |= SR_C;
    return t;
}

void Tms7000::ShortBranch(bool check)
{
    m_icount -= 2;
    int8_t d = int8_t(Imm8());
    if (check) { m_pc += d; m_icount -= 2; }
}

void Tms7000::Jmp(bool check) { m_icount -= 3; ShortBranch(check); }

int Tms7000::OpDjnz(uint8_t p1, uint8_t) { uint16_t t = p1 - 1; ShortBranch(t != 0); return t; }
int Tms7000::OpBtjo(uint8_t p1, uint8_t p2) { uint8_t t = p1 & p2; SET_CNZ(t); ShortBranch(t != 0); return WB_NO; }
int Tms7000::OpBtjz(uint8_t p1, uint8_t p2) { uint8_t t = uint8_t(~p1) & p2; SET_CNZ(t); ShortBranch(t != 0); return WB_NO; }

void Tms7000::DecdA() { m_icount -= 9; uint32_t t = ReadR16(0) - 1; WriteR16(0, uint16_t(t)); SET_NZ(t >> 8); SET_C(~(t >> 8)); }
void Tms7000::DecdB() { m_icount -= 9; uint32_t t = ReadR16(1) - 1; WriteR16(1, uint16_t(t)); SET_NZ(t >> 8); SET_C(~(t >> 8)); }
void Tms7000::DecdR() { m_icount -= 11; uint8_t r = Imm8(); uint32_t t = ReadR16(r) - 1; WriteR16(r, uint16_t(t)); SET_NZ(t >> 8); SET_C(~(t >> 8)); }

void Tms7000::CmpaDir() { m_icount -= 12; uint16_t t = ReadR8(0) - Read8(Imm16()); SET_NZ(t); SET_C(~t); }
void Tms7000::CmpaInx() { m_icount -= 14; uint16_t t = ReadR8(0) - Read8(Imm16() + ReadR8(1)); SET_NZ(t); SET_C(~t); }
void Tms7000::CmpaInd() { m_icount -= 11; uint16_t t = ReadR8(0) - Read8(ReadR16(Imm8())); SET_NZ(t); SET_C(~t); }

void Tms7000::LdaDir() { m_icount -= 11; uint8_t t = Read8(Imm16()); WriteR8(0, t); SET_CNZ(t); }
void Tms7000::LdaInx() { m_icount -= 13; uint8_t t = Read8(Imm16() + ReadR8(1)); WriteR8(0, t); SET_CNZ(t); }
void Tms7000::LdaInd() { m_icount -= 10; uint8_t t = Read8(ReadR16(Imm8())); WriteR8(0, t); SET_CNZ(t); }

void Tms7000::StaDir() { m_icount -= 11; uint8_t t = ReadR8(0); Write8(Imm16(), t); SET_CNZ(t); }
void Tms7000::StaInx() { m_icount -= 13; uint8_t t = ReadR8(0); Write8(Imm16() + ReadR8(1), t); SET_CNZ(t); }
void Tms7000::StaInd() { m_icount -= 10; uint8_t t = ReadR8(0); Write8(ReadR16(Imm8()), t); SET_CNZ(t); }

void Tms7000::MovdDir() { m_icount -= 15; uint16_t t = Imm16(); WriteR16(Imm8(), t); SET_CNZ(t >> 8); }
void Tms7000::MovdInx() { m_icount -= 17; uint16_t t = Imm16() + ReadR8(1); WriteR16(Imm8(), t); SET_CNZ(t >> 8); }
void Tms7000::MovdInd() { m_icount -= 14; uint16_t t = ReadR16(Imm8()); WriteR16(Imm8(), t); SET_CNZ(t >> 8); }

void Tms7000::BrDir() { m_icount -= 10; m_pc = Imm16(); }
void Tms7000::BrInx() { m_icount -= 12; m_pc = Imm16() + ReadR8(1); }
void Tms7000::BrInd() { m_icount -= 9; m_pc = ReadR16(Imm8()); }

void Tms7000::CallDir() { m_icount -= 14; uint16_t t = Imm16(); Push16(m_pc); m_pc = t; }
void Tms7000::CallInx() { m_icount -= 16; uint16_t t = Imm16() + ReadR8(1); Push16(m_pc); m_pc = t; }
void Tms7000::CallInd() { m_icount -= 13; uint16_t t = ReadR16(Imm8()); Push16(m_pc); m_pc = t; }

void Tms7000::Trap(uint8_t address)
{
    m_icount -= 14;
    Push16(m_pc);
    uint16_t vec = 0xff00 | address;
    m_pc = (uint16_t(Read8(vec)) << 8) | Read8(uint16_t(vec + 1));
}

void Tms7000::Reti() { m_icount -= 9; m_pc = Pull16(); m_sr = Pull8() & 0xf0; CheckInterrupts(); }
void Tms7000::Rets() { m_icount -= 7; m_pc = Pull16(); }

void Tms7000::PopA()  { m_icount -= 6; uint8_t t = Pull8(); WriteR8(0, t); SET_CNZ(t); }
void Tms7000::PopB()  { m_icount -= 6; uint8_t t = Pull8(); WriteR8(1, t); SET_CNZ(t); }
void Tms7000::PopR()  { m_icount -= 8; uint8_t t = Pull8(); WriteR8(Imm8(), t); SET_CNZ(t); }
void Tms7000::PopSt() { m_icount -= 6; m_sr = Pull8() & 0xf0; CheckInterrupts(); }

void Tms7000::PushA() { m_icount -= 6; uint8_t t = ReadR8(0); Push8(t); SET_CNZ(t); }
void Tms7000::PushB() { m_icount -= 6; uint8_t t = ReadR8(1); Push8(t); SET_CNZ(t); }
void Tms7000::PushR() { m_icount -= 8; uint8_t t = ReadR8(Imm8()); Push8(t); SET_CNZ(t); }
void Tms7000::PushSt(){ m_icount -= 6; Push8(m_sr); }

void Tms7000::Nop()  { m_icount -= 5; }
void Tms7000::Idle() { m_icount -= 6; m_pc--; m_idle_state = true; }
void Tms7000::Dint() { m_icount -= 5; m_sr &= ~(SR_N | SR_Z | SR_C | SR_I); }
void Tms7000::Eint() { m_icount -= 5; m_sr |= (SR_N | SR_Z | SR_C | SR_I); CheckInterrupts(); }
void Tms7000::Ldsp() { m_icount -= 5; m_sp = ReadR8(1); }
void Tms7000::Stsp() { m_icount -= 6; WriteR8(1, m_sp); }
void Tms7000::Setc() { m_icount -= 5; m_sr = (m_sr & ~SR_N) | SR_C | SR_Z; }

void Tms7000::Illegal(uint8_t) { m_icount -= 5; m_illegal_count++; }

#undef GET_C
#undef SET_C
#undef SET_NZ
#undef SET_CNZ
#undef WB_NO

// ---------------------------------------------------------------------
// dispatch (opcode map identical to MAME's tms7000_device::execute_one)
// ---------------------------------------------------------------------

void Tms7000::ExecuteOne(uint8_t op)
{
    switch (op)
    {
        case 0x00: Nop(); break;
        case 0x01: Idle(); break;
        case 0x05: Eint(); break;
        case 0x06: Dint(); break;
        case 0x07: Setc(); break;
        case 0x08: PopSt(); break;
        case 0x09: Stsp(); break;
        case 0x0a: Rets(); break;
        case 0x0b: Reti(); break;
        case 0x0d: Ldsp(); break;
        case 0x0e: PushSt(); break;

        case 0x12: AmR2a(&Tms7000::OpMov); break;
        case 0x13: AmR2a(&Tms7000::OpAnd); break;
        case 0x14: AmR2a(&Tms7000::OpOr); break;
        case 0x15: AmR2a(&Tms7000::OpXor); break;
        case 0x16: AmR2a(&Tms7000::OpBtjo); break;
        case 0x17: AmR2a(&Tms7000::OpBtjz); break;
        case 0x18: AmR2a(&Tms7000::OpAdd); break;
        case 0x19: AmR2a(&Tms7000::OpAdc); break;
        case 0x1a: AmR2a(&Tms7000::OpSub); break;
        case 0x1b: AmR2a(&Tms7000::OpSbb); break;
        case 0x1c: AmR2a(&Tms7000::OpMpy); break;
        case 0x1d: AmR2a(&Tms7000::OpCmp); break;
        case 0x1e: AmR2a(&Tms7000::OpDac); break;
        case 0x1f: AmR2a(&Tms7000::OpDsb); break;

        case 0x22: AmI2a(&Tms7000::OpMov); break;
        case 0x23: AmI2a(&Tms7000::OpAnd); break;
        case 0x24: AmI2a(&Tms7000::OpOr); break;
        case 0x25: AmI2a(&Tms7000::OpXor); break;
        case 0x26: AmI2a(&Tms7000::OpBtjo); break;
        case 0x27: AmI2a(&Tms7000::OpBtjz); break;
        case 0x28: AmI2a(&Tms7000::OpAdd); break;
        case 0x29: AmI2a(&Tms7000::OpAdc); break;
        case 0x2a: AmI2a(&Tms7000::OpSub); break;
        case 0x2b: AmI2a(&Tms7000::OpSbb); break;
        case 0x2c: AmI2a(&Tms7000::OpMpy); break;
        case 0x2d: AmI2a(&Tms7000::OpCmp); break;
        case 0x2e: AmI2a(&Tms7000::OpDac); break;
        case 0x2f: AmI2a(&Tms7000::OpDsb); break;

        case 0x32: AmR2b(&Tms7000::OpMov); break;
        case 0x33: AmR2b(&Tms7000::OpAnd); break;
        case 0x34: AmR2b(&Tms7000::OpOr); break;
        case 0x35: AmR2b(&Tms7000::OpXor); break;
        case 0x36: AmR2b(&Tms7000::OpBtjo); break;
        case 0x37: AmR2b(&Tms7000::OpBtjz); break;
        case 0x38: AmR2b(&Tms7000::OpAdd); break;
        case 0x39: AmR2b(&Tms7000::OpAdc); break;
        case 0x3a: AmR2b(&Tms7000::OpSub); break;
        case 0x3b: AmR2b(&Tms7000::OpSbb); break;
        case 0x3c: AmR2b(&Tms7000::OpMpy); break;
        case 0x3d: AmR2b(&Tms7000::OpCmp); break;
        case 0x3e: AmR2b(&Tms7000::OpDac); break;
        case 0x3f: AmR2b(&Tms7000::OpDsb); break;

        case 0x42: AmR2r(&Tms7000::OpMov); break;
        case 0x43: AmR2r(&Tms7000::OpAnd); break;
        case 0x44: AmR2r(&Tms7000::OpOr); break;
        case 0x45: AmR2r(&Tms7000::OpXor); break;
        case 0x46: AmR2r(&Tms7000::OpBtjo); break;
        case 0x47: AmR2r(&Tms7000::OpBtjz); break;
        case 0x48: AmR2r(&Tms7000::OpAdd); break;
        case 0x49: AmR2r(&Tms7000::OpAdc); break;
        case 0x4a: AmR2r(&Tms7000::OpSub); break;
        case 0x4b: AmR2r(&Tms7000::OpSbb); break;
        case 0x4c: AmR2r(&Tms7000::OpMpy); break;
        case 0x4d: AmR2r(&Tms7000::OpCmp); break;
        case 0x4e: AmR2r(&Tms7000::OpDac); break;
        case 0x4f: AmR2r(&Tms7000::OpDsb); break;

        case 0x52: AmI2b(&Tms7000::OpMov); break;
        case 0x53: AmI2b(&Tms7000::OpAnd); break;
        case 0x54: AmI2b(&Tms7000::OpOr); break;
        case 0x55: AmI2b(&Tms7000::OpXor); break;
        case 0x56: AmI2b(&Tms7000::OpBtjo); break;
        case 0x57: AmI2b(&Tms7000::OpBtjz); break;
        case 0x58: AmI2b(&Tms7000::OpAdd); break;
        case 0x59: AmI2b(&Tms7000::OpAdc); break;
        case 0x5a: AmI2b(&Tms7000::OpSub); break;
        case 0x5b: AmI2b(&Tms7000::OpSbb); break;
        case 0x5c: AmI2b(&Tms7000::OpMpy); break;
        case 0x5d: AmI2b(&Tms7000::OpCmp); break;
        case 0x5e: AmI2b(&Tms7000::OpDac); break;
        case 0x5f: AmI2b(&Tms7000::OpDsb); break;

        case 0x62: AmB2a(&Tms7000::OpMov); break;
        case 0x63: AmB2a(&Tms7000::OpAnd); break;
        case 0x64: AmB2a(&Tms7000::OpOr); break;
        case 0x65: AmB2a(&Tms7000::OpXor); break;
        case 0x66: AmB2a(&Tms7000::OpBtjo); break;
        case 0x67: AmB2a(&Tms7000::OpBtjz); break;
        case 0x68: AmB2a(&Tms7000::OpAdd); break;
        case 0x69: AmB2a(&Tms7000::OpAdc); break;
        case 0x6a: AmB2a(&Tms7000::OpSub); break;
        case 0x6b: AmB2a(&Tms7000::OpSbb); break;
        case 0x6c: AmB2a(&Tms7000::OpMpy); break;
        case 0x6d: AmB2a(&Tms7000::OpCmp); break;
        case 0x6e: AmB2a(&Tms7000::OpDac); break;
        case 0x6f: AmB2a(&Tms7000::OpDsb); break;

        case 0x72: AmI2r(&Tms7000::OpMov); break;
        case 0x73: AmI2r(&Tms7000::OpAnd); break;
        case 0x74: AmI2r(&Tms7000::OpOr); break;
        case 0x75: AmI2r(&Tms7000::OpXor); break;
        case 0x76: AmI2r(&Tms7000::OpBtjo); break;
        case 0x77: AmI2r(&Tms7000::OpBtjz); break;
        case 0x78: AmI2r(&Tms7000::OpAdd); break;
        case 0x79: AmI2r(&Tms7000::OpAdc); break;
        case 0x7a: AmI2r(&Tms7000::OpSub); break;
        case 0x7b: AmI2r(&Tms7000::OpSbb); break;
        case 0x7c: AmI2r(&Tms7000::OpMpy); break;
        case 0x7d: AmI2r(&Tms7000::OpCmp); break;
        case 0x7e: AmI2r(&Tms7000::OpDac); break;
        case 0x7f: AmI2r(&Tms7000::OpDsb); break;

        case 0x80: AmP2a(&Tms7000::OpMov); break;
        case 0x82: AmA2p(&Tms7000::OpMov); break;
        case 0x83: AmA2p(&Tms7000::OpAnd); break;
        case 0x84: AmA2p(&Tms7000::OpOr); break;
        case 0x85: AmA2p(&Tms7000::OpXor); break;
        case 0x86: AmA2p(&Tms7000::OpBtjo); break;
        case 0x87: AmA2p(&Tms7000::OpBtjz); break;
        case 0x88: MovdDir(); break;
        case 0x8a: LdaDir(); break;
        case 0x8b: StaDir(); break;
        case 0x8c: BrDir(); break;
        case 0x8d: CmpaDir(); break;
        case 0x8e: CallDir(); break;

        case 0x91: AmP2b(&Tms7000::OpMov); break;
        case 0x92: AmB2p(&Tms7000::OpMov); break;
        case 0x93: AmB2p(&Tms7000::OpAnd); break;
        case 0x94: AmB2p(&Tms7000::OpOr); break;
        case 0x95: AmB2p(&Tms7000::OpXor); break;
        case 0x96: AmB2p(&Tms7000::OpBtjo); break;
        case 0x97: AmB2p(&Tms7000::OpBtjz); break;
        // NOTE: opcodes 0x98-0x9e are the *indirect* (register-pair
        // pointer) addressing-mode family, and 0xa8-0xae are the
        // *indexed* (16-bit immediate + B-register offset) family --
        // confirmed byte-for-byte against MAME's tms7000.cpp
        // execute_one() dispatch table. An earlier version of this file
        // had these two families swapped (0x98-0x9e wrongly wired to
        // the Inx handlers, 0xa8-0xae wrongly wired to the Ind
        // handlers), which silently mis-decoded every instruction using
        // either opcode range: since IndX consumes a 16-bit immediate
        // operand and Ind consumes only an 8-bit one, the swap caused
        // the CPU to consume the wrong number of operand bytes and
        // permanently desync from the real instruction stream from that
        // point on, well before this specific SSC firmware ever reaches
        // the code that dispatches a received host command.
        case 0x98: MovdInd(); break;
        case 0x9a: LdaInd(); break;
        case 0x9b: StaInd(); break;
        case 0x9c: BrInd(); break;
        case 0x9d: CmpaInd(); break;
        case 0x9e: CallInd(); break;

        case 0xa2: AmI2p(&Tms7000::OpMov); break;
        case 0xa3: AmI2p(&Tms7000::OpAnd); break;
        case 0xa4: AmI2p(&Tms7000::OpOr); break;
        case 0xa5: AmI2p(&Tms7000::OpXor); break;
        case 0xa6: AmI2p(&Tms7000::OpBtjo); break;
        case 0xa7: AmI2p(&Tms7000::OpBtjz); break;
        case 0xa8: MovdInx(); break;
        case 0xaa: LdaInx(); break;
        case 0xab: StaInx(); break;
        case 0xac: BrInx(); break;
        case 0xad: CmpaInx(); break;
        case 0xae: CallInx(); break;

        case 0xb0: AmA2a(&Tms7000::OpMov); break;
        case 0xb1: AmB2a(&Tms7000::OpMov); break;
        case 0xb2: AmA(&Tms7000::OpDec); break;
        case 0xb3: AmA(&Tms7000::OpInc); break;
        case 0xb4: AmA(&Tms7000::OpInv); break;
        case 0xb5: AmA(&Tms7000::OpClr); break;
        case 0xb6: AmA(&Tms7000::OpXchb); break;
        case 0xb7: AmA(&Tms7000::OpSwap); break;
        case 0xb8: PushA(); break;
        case 0xb9: PopA(); break;
        case 0xba: AmA(&Tms7000::OpDjnz); break;
        case 0xbb: DecdA(); break;
        case 0xbc: AmA(&Tms7000::OpRr); break;
        case 0xbd: AmA(&Tms7000::OpRrc); break;
        case 0xbe: AmA(&Tms7000::OpRl); break;
        case 0xbf: AmA(&Tms7000::OpRlc); break;

        case 0xc0: AmA2b(&Tms7000::OpMov); break;
        case 0xc1: AmB2b(&Tms7000::OpMov); break;
        case 0xc2: AmB(&Tms7000::OpDec); break;
        case 0xc3: AmB(&Tms7000::OpInc); break;
        case 0xc4: AmB(&Tms7000::OpInv); break;
        case 0xc5: AmB(&Tms7000::OpClr); break;
        case 0xc6: AmB(&Tms7000::OpXchb); break;
        case 0xc7: AmB(&Tms7000::OpSwap); break;
        case 0xc8: PushB(); break;
        case 0xc9: PopB(); break;
        case 0xca: AmB(&Tms7000::OpDjnz); break;
        case 0xcb: DecdB(); break;
        case 0xcc: AmB(&Tms7000::OpRr); break;
        case 0xcd: AmB(&Tms7000::OpRrc); break;
        case 0xce: AmB(&Tms7000::OpRl); break;
        case 0xcf: AmB(&Tms7000::OpRlc); break;

        case 0xd0: AmA2r(&Tms7000::OpMov); break;
        case 0xd1: AmB2r(&Tms7000::OpMov); break;
        case 0xd2: AmR(&Tms7000::OpDec); break;
        case 0xd3: AmR(&Tms7000::OpInc); break;
        case 0xd4: AmR(&Tms7000::OpInv); break;
        case 0xd5: AmR(&Tms7000::OpClr); break;
        case 0xd6: AmR(&Tms7000::OpXchb); break;
        case 0xd7: AmR(&Tms7000::OpSwap); break;
        case 0xd8: PushR(); break;
        case 0xd9: PopR(); break;
        case 0xda: AmR(&Tms7000::OpDjnz); break;
        case 0xdb: DecdR(); break;
        case 0xdc: AmR(&Tms7000::OpRr); break;
        case 0xdd: AmR(&Tms7000::OpRrc); break;
        case 0xde: AmR(&Tms7000::OpRl); break;
        case 0xdf: AmR(&Tms7000::OpRlc); break;

        case 0xe0: Jmp(true); break;
        case 0xe1: Jmp((m_sr & SR_N) != 0); break;
        case 0xe2: Jmp((m_sr & SR_Z) != 0); break;
        case 0xe3: Jmp((m_sr & SR_C) != 0); break;
        case 0xe4: Jmp(!(m_sr & (SR_Z | SR_N))); break;
        case 0xe5: Jmp(!(m_sr & SR_N)); break;
        case 0xe6: Jmp(!(m_sr & SR_Z)); break;
        case 0xe7: Jmp(!(m_sr & SR_C)); break;

        case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
            Trap(op << 1); break;

        default: Illegal(op); break;
    }
}

} // namespace ssc
