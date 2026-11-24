// ym2149.h
//
// Yamaha YM2149 SSG (Software-controlled Sound Generator) emulation for
// the CoCo PSG cartridge. Portable C++, no VCC or Windows dependency.
//
// Register model (from the AY-3-8910 / YM2149 datasheets):
//   R0-R5   tone period, channels A/B/C (12 bits: fine + 4-bit coarse)
//   R6      noise period (5 bits)
//   R7      mixer: bits 0-2 tone disable A/B/C, bits 3-5 noise disable
//           A/B/C, bit 6 I/O port A direction, bit 7 I/O port B direction
//           (0 = input, 1 = output)
//   R8-R10  channel amplitude (bits 0-3 level, bit 4 = use envelope)
//   R11-R12 envelope period (16 bits)
//   R13     envelope shape (CONT/ATT/ALT/HOLD)
//   R14-R15 I/O port A/B data
//
// YM2149-specific behaviour modelled here (differences from the AY):
//   - 32-step envelope (the AY has 16), stepped at clock/8 per period
//     count, so a full ramp is still clock/(256*EP) like the AY.
//   - 5-bit DAC: a fixed amplitude N plays at DAC level 2N+1, the
//     envelope uses all 32 levels.
//   - The *SEL pin halves the master clock. The CoCo PSG drives it from
//     control register bit 0 (0 = 2MHz, 1 = 1MHz); this class just
//     takes the effective clock via SetClock().
//
// Timing: everything runs from one internal tick at clock/8.
//   tone     toggles every TP ticks        -> f = clock / (16*TP)
//   noise    LFSR shifts every 2*NP ticks  -> f = clock / (16*NP)
//   envelope one of 32 steps every EP ticks
//
// Output: Render() box-filters the three channels over the elapsed
// ticks, so tones above the host sample rate average out instead of
// aliasing.

#pragma once
#include <cstdint>

namespace psg {

class Ym2149 {
public:
    Ym2149();

    void Reset();

    void SetClock(double clock_hz) { m_clock_hz = clock_hz; }
    double Clock() const { return m_clock_hz; }

    void SelectRegister(uint8_t reg) { m_selected = reg; }
    uint8_t SelectedRegister() const { return m_selected; }

    void WriteData(uint8_t data);
    uint8_t ReadData() const;

    // Direct register access (used by tests/diagnostics).
    uint8_t Reg(int r) const { return m_regs[r & 15]; }

    // Levels presented on the I/O port pins when a port is an input.
    // Defaults to 0xFF (pulled up, nothing pressed).
    void SetPortInput(int port, uint8_t value) { m_port_in[port & 1] = value; }

    // Advance by `seconds` and return the mean summed channel output over
    // that span, in [0, 3] (each channel contributes 0..1).
    double Render(double seconds);

    // One internal tick (clock/8) and the summed output after it, in
    // [0, 3]. The cart drives the chip with these (see psg_cart.cpp).
    void Clock() { Tick(); }
    double Output() const { return CurrentOutput(); }

    // Normalized amplitude of DAC level 0..31.
    static double DacLevel(int level);

private:
    void Tick();
    double CurrentOutput() const;
    int  EnvelopeLevel() const;

    uint8_t m_regs[16] {};   // masked to implemented bits (used for sound)
    uint8_t m_raw[16] {};    // as written (read back; the YM2149 does not mask)
    uint8_t m_selected = 0;
    uint8_t m_port_in[2] { 0xFF, 0xFF };

    double m_clock_hz = 2000000.0;
    double m_tick_accum = 0.0;

    int m_tone_count[3] {};
    int m_tone_out[3] {};

    int      m_noise_count = 0;
    bool     m_noise_prescale = false;
    uint32_t m_noise_lfsr = 1;

    int  m_env_count = 0;
    int  m_env_step = 0;     // 0..31 within the current ramp
    bool m_env_attack = false;
    bool m_env_holding = false;
    int  m_env_hold_level = 0;
};

} // namespace psg
