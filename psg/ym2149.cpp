// ym2149.cpp -- see ym2149.h for design notes.

#include "ym2149.h"
#include <cmath>

namespace psg {

namespace {

// Bits implemented in each register. Only the sound generators use the
// masked values: on the CoCo PSG a write of $FF to R1 reads back $FF
// (measured on real hardware), so read-back returns what was written.
constexpr uint8_t kRegMask[16] = {
    0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x0F, 0x1F, 0xFF,
    0x1F, 0x1F, 0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF
};

// 32-level DAC, normalised to 1.0 at full scale. These are measured
// YM2149 output levels (the table used by Peter Sovietov's "ayumi"
// AY/YM emulator), not an ideal log curve: levels 0 and 1 are both
// silent, and the low levels are louder than an ideal 1.5dB/step curve.
// A fixed amplitude N plays at level 2N+1.
struct DacTable {
    double v[32] = {
        0.0, 0.0,
        0.00465400167849, 0.00772106507973, 0.0109559777218, 0.0139620050355,
        0.0169985503929, 0.0200198367285, 0.024368657969, 0.029694056611,
        0.0350652323186, 0.0403906309606, 0.0485389486534, 0.0583352407111,
        0.0680552376593, 0.0777752346075, 0.0925154497597, 0.111085679408,
        0.129747463188, 0.148485542077, 0.17666895552, 0.211551079576,
        0.246387426566, 0.281101701381, 0.333730067903, 0.400427252613,
        0.467383840696, 0.53443198291, 0.635172045472, 0.75800717174,
        0.879926756695, 1.0 };
};
const DacTable kDac;

} // namespace

double Ym2149::DacLevel(int level)
{
    return kDac.v[level & 31];
}

Ym2149::Ym2149()
{
    Reset();
}

void Ym2149::Reset()
{
    for (auto& r : m_regs) r = 0;
    for (auto& r : m_raw) r = 0;
    m_selected = 0;
    m_tick_accum = 0.0;
    for (int i = 0; i < 3; i++) { m_tone_count[i] = 0; m_tone_out[i] = 0; }
    m_noise_count = 0;
    m_noise_prescale = false;
    m_noise_lfsr = 1;
    m_env_count = 0;
    m_env_step = 0;
    m_env_attack = false;
    m_env_holding = false;
    m_env_hold_level = 0;
}

void Ym2149::WriteData(uint8_t data)
{
    if (m_selected > 15)
        return; // only R0-R15 exist

    const int r = m_selected;
    m_raw[r] = data;
    m_regs[r] = data & kRegMask[r];

    if (r == 13) {
        // Writing the shape restarts the envelope.
        m_env_count = 0;
        m_env_step = 0;
        m_env_attack = (data & 0x04) != 0;
        m_env_holding = false;
    }
}

uint8_t Ym2149::ReadData() const
{
    if (m_selected > 15)
        return 0xFF;

    // I/O ports: an input reads the pins, an output reads its latch.
    if (m_selected == 14)
        return (m_regs[7] & 0x40) ? m_raw[14] : m_port_in[0];
    if (m_selected == 15)
        return (m_regs[7] & 0x80) ? m_raw[15] : m_port_in[1];

    return m_raw[m_selected];
}

int Ym2149::EnvelopeLevel() const
{
    if (m_env_holding)
        return m_env_hold_level;
    return m_env_attack ? m_env_step : 31 - m_env_step;
}

void Ym2149::Tick()
{
    // Tone generators.
    for (int ch = 0; ch < 3; ch++) {
        int period = m_regs[ch * 2] | (m_regs[ch * 2 + 1] << 8);
        if (period == 0) period = 1;
        if (++m_tone_count[ch] >= period) {
            m_tone_count[ch] = 0;
            m_tone_out[ch] ^= 1;
        }
    }

    // Noise: clocked at half the tone rate.
    m_noise_prescale = !m_noise_prescale;
    if (m_noise_prescale) {
        int period = m_regs[6];
        if (period == 0) period = 1;
        if (++m_noise_count >= period) {
            m_noise_count = 0;
            // 17-bit LFSR, feedback = bit0 XOR bit3.
            uint32_t bit = (m_noise_lfsr ^ (m_noise_lfsr >> 3)) & 1;
            m_noise_lfsr = (m_noise_lfsr >> 1) | (bit << 16);
        }
    }

    // Envelope.
    if (!m_env_holding) {
        int period = m_regs[11] | (m_regs[12] << 8);
        if (period == 0) period = 1;
        if (++m_env_count >= period) {
            m_env_count = 0;
            if (++m_env_step > 31) {
                const uint8_t shape = m_regs[13];
                const bool cont = (shape & 0x08) != 0;
                const bool alt  = (shape & 0x02) != 0;
                const bool hold = (shape & 0x01) != 0;

                if (!cont) {
                    // Shapes 0-7: one ramp, then silence.
                    m_env_holding = true;
                    m_env_hold_level = 0;
                } else if (hold) {
                    // Hold at the end of the ramp, or at its start if
                    // ALT is also set.
                    m_env_holding = true;
                    bool end_high = m_env_attack;
                    if (alt) end_high = !end_high;
                    m_env_hold_level = end_high ? 31 : 0;
                } else {
                    m_env_step = 0;
                    if (alt) m_env_attack = !m_env_attack;
                }
                if (m_env_step > 31) m_env_step = 31;
            }
        }
    }
}

double Ym2149::CurrentOutput() const
{
    const uint8_t mixer = m_regs[7];
    const int noise = int(m_noise_lfsr & 1);
    double sum = 0.0;

    for (int ch = 0; ch < 3; ch++) {
        // A disabled source reads as 1, so with tone and noise both off
        // the channel outputs a constant level (used for sample playback).
        const int tone_on = (mixer >> ch) & 1 ? 1 : m_tone_out[ch];
        const int noise_on = (mixer >> (ch + 3)) & 1 ? 1 : noise;
        if (!(tone_on & noise_on))
            continue;

        const uint8_t amp = m_regs[8 + ch];
        const int level = (amp & 0x10) ? EnvelopeLevel() : ((amp & 0x0F) << 1) | 1;
        // Fixed amplitude 0 is DAC level 1 on the YM: effectively silent,
        // but treat it as exactly silent so idle channels add no offset.
        if (!(amp & 0x10) && (amp & 0x0F) == 0)
            continue;
        sum += kDac.v[level];
    }
    return sum;
}

double Ym2149::Render(double seconds)
{
    if (m_clock_hz <= 0.0 || seconds <= 0.0)
        return CurrentOutput();

    const double ticks = seconds * (m_clock_hz / 8.0) + m_tick_accum;
    long whole = long(ticks);
    m_tick_accum = ticks - double(whole);
    if (whole > 1000000) whole = 1000000;

    if (whole == 0)
        return CurrentOutput();

    double acc = 0.0;
    for (long i = 0; i < whole; i++) {
        Tick();
        acc += CurrentOutput();
    }
    return acc / double(whole);
}

} // namespace psg
