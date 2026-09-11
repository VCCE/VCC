// ay8913.h
//
// Standalone emulation of the General Instrument AY-3-8913 Programmable
// Sound Generator, as used inside the Tandy Speech/Sound Cartridge (the
// SSC uses the single-ended-output '8913 variant, which is functionally
// the tone/noise/envelope core of the AY-3-8910 minus the two 8-bit
// parallel I/O ports and minus the three separate channel pins -- the
// '8913 mixes all three channels internally and brings out one analog
// output).
//
// This is an original clean-room implementation written from the
// published AY-3-8910/8912/8913 datasheet register model (16 registers,
// three 12-bit tone generators, one 5-bit noise generator, one 16-bit
// hardware envelope generator with the ten standard shapes, one 8-bit
// mixer/enable register, three 4/5-bit channel amplitude registers) --
// it is not a port of MAME's ay8910.cpp. The tone/noise/envelope timing
// follows the datasheet's documented divide ratios (tone/noise: input
// clock/8 internal step, toggle every period counts, giving the
// datasheet's f = clock/(16*N); envelope: same /8 step, one of 32 ramp
// positions advanced every period counts, giving f = clock/(8*16*N)).
// The per-level output amplitude table is a logarithmic approximation
// (roughly -2dB/step) rather than a measured-silicon table, since no
// authoritative published table was available while writing this --
// see the comment on kVolumeTable below if you want to substitute a
// more precisely measured table later.
//
// Port for VCC SSC cartridge: 2026

#pragma once
#include <cstdint>

namespace ssc {

class Ay8913 {
public:
    Ay8913();

    void Reset();

    // Latch a register number (0-15; only 0-13 are meaningful on the
    // '8913, since it has no I/O ports).
    void SelectRegister(uint8_t reg);

    // Write to the currently-selected register.
    void WriteData(uint8_t data);

    // Read back the currently-selected register (used by the SSC's PIC
    // firmware to poll status via the D-port bus turnaround).
    uint8_t ReadData() const;

    // Sets the chip's input clock (Hz) -- the SSC drives this at the
    // doubled CoCo E-clock, ~1.789772MHz.
    void SetClock(double clock_hz) { m_clock_hz = clock_hz; }

    // Advances the internal generators by `seconds` of wall-clock time
    // and returns one mixed output sample, roughly normalized to
    // [-32767, 32767] (silence = 0). Intended to be called once per
    // host audio tick (e.g. once per CoCo scanline).
    int16_t Render(double seconds);

private:
    static constexpr int kNumRegs = 16;

    uint8_t m_regs[kNumRegs] = {0};
    uint8_t m_selected = 0;

    double m_clock_hz = 1789772.0;
    double m_step_accum = 0.0; // fractional internal (/8) steps owed

    // tone generators (A,B,C)
    int m_tone_counter[3] = {0, 0, 0};
    int m_tone_output[3]  = {1, 1, 1};

    // noise generator
    int      m_noise_counter = 0;
    uint32_t m_noise_lfsr = 1;
    int      m_noise_output = 1;

    // envelope generator
    int  m_env_counter = 0;
    int  m_env_step = 0;      // 0-31 ramp position
    bool m_env_dir_up = true; // current ramp direction
    bool m_env_holding = false;

    int TonePeriod(int ch) const;
    int NoisePeriod() const;
    int EnvPeriod() const;
    void TickOneEnvelopeStep();
    int CurrentEnvelopeLevel() const; // 0-31 shape position -> 0-15 vol index

    static const int16_t kVolumeTable[16];
};

} // namespace ssc
