// ay8913.cpp -- see ay8913.h for design notes.

#include "ay8913.h"
#include <cmath>
#include <algorithm>

namespace ssc {

// Logarithmic approximation of the AY's per-channel volume table
// (-2dB per step, level 15 = full scale, level 0 = silence). See the
// header comment: this is not a measured-silicon table, just a
// reasonable-sounding approximation of the chip's documented
// logarithmic response.
const int16_t Ay8913::kVolumeTable[16] = {
    0, 1304, 1642, 2067, 2603, 3277, 4125, 5193,
    6538, 8231, 10362, 13045, 16422, 20675, 26028, 32767
};

Ay8913::Ay8913()
{
    Reset();
}

void Ay8913::Reset()
{
    for (auto& r : m_regs) r = 0;
    m_selected = 0;
    m_step_accum = 0.0;
    for (int i = 0; i < 3; i++) { m_tone_counter[i] = 0; m_tone_output[i] = 1; }
    m_noise_counter = 0;
    m_noise_lfsr = 1;
    m_noise_output = 1;
    m_env_counter = 0;
    m_env_step = 0;
    m_env_dir_up = true;
    m_env_holding = false;
}

void Ay8913::SelectRegister(uint8_t reg)
{
    m_selected = reg & 0x0f;
}

void Ay8913::WriteData(uint8_t data)
{
    m_regs[m_selected] = data;

    if (m_selected == 13) {
        // envelope shape write resets the ramp
        m_env_step = 0;
        m_env_holding = false;
        m_env_dir_up = (data & 0x04) != 0; // Attack
    }
}

uint8_t Ay8913::ReadData() const
{
    return m_regs[m_selected];
}

int Ay8913::TonePeriod(int ch) const
{
    int fine = m_regs[ch * 2];
    int coarse = m_regs[ch * 2 + 1] & 0x0f;
    int p = (coarse << 8) | fine;
    return p ? p : 1;
}

int Ay8913::NoisePeriod() const
{
    int p = m_regs[6] & 0x1f;
    return p ? p : 1;
}

int Ay8913::EnvPeriod() const
{
    int fine = m_regs[11];
    int coarse = m_regs[12];
    int p = (coarse << 8) | fine;
    return p ? p : 1;
}

void Ay8913::TickOneEnvelopeStep()
{
    if (m_env_holding) return;

    uint8_t shape = m_regs[13];
    bool cont = (shape & 0x08) != 0;
    bool alt  = (shape & 0x02) != 0;
    bool hold = (shape & 0x01) != 0;

    m_env_step++;
    if (m_env_step >= 32)
    {
        if (!cont)
        {
            m_env_step = 32;
            m_env_holding = true;
        }
        else if (hold)
        {
            m_env_step = 31;
            m_env_holding = true;
            if (alt) m_env_dir_up = !m_env_dir_up;
        }
        else
        {
            m_env_step = 0;
            if (alt) m_env_dir_up = !m_env_dir_up;
        }
    }
}

int Ay8913::CurrentEnvelopeLevel() const
{
    uint8_t shape = m_regs[13];
    bool cont = (shape & 0x08) != 0;

    if (!cont && m_env_holding)
        return 0;

    int step = m_env_step & 31;
    int level = m_env_dir_up ? step : (31 - step);
    return level >> 1; // 0..31 -> 0..15
}

int16_t Ay8913::Render(double seconds)
{
    if (m_clock_hz <= 0.0) return 0;

    double steps = seconds * (m_clock_hz / 8.0) + m_step_accum;
    long whole = (long)steps;
    m_step_accum = steps - double(whole);

    // Safety cap: never iterate an absurd number of internal steps in
    // one call (e.g. if the host passes a huge `seconds` by mistake).
    if (whole > 1'000'000) whole = 1'000'000;

    for (long i = 0; i < whole; i++)
    {
        for (int ch = 0; ch < 3; ch++)
        {
            if (++m_tone_counter[ch] >= TonePeriod(ch)) {
                m_tone_counter[ch] = 0;
                m_tone_output[ch] ^= 1;
            }
        }

        if (++m_noise_counter >= NoisePeriod()) {
            m_noise_counter = 0;
            // 17-bit LFSR, taps at bit0 and bit3 (standard AY polynomial)
            uint32_t bit = ((m_noise_lfsr ^ (m_noise_lfsr >> 3)) & 1);
            m_noise_lfsr = (m_noise_lfsr >> 1) | (bit << 16);
            m_noise_output = int(m_noise_lfsr & 1);
        }

        if (++m_env_counter >= EnvPeriod()) {
            m_env_counter = 0;
            TickOneEnvelopeStep();
        }
    }

    uint8_t mixer = m_regs[7];
    int32_t mix = 0;

    for (int ch = 0; ch < 3; ch++)
    {
        bool tone_enabled  = ((mixer >> ch) & 1) == 0;       // active low
        bool noise_enabled = ((mixer >> (ch + 3)) & 1) == 0; // active low

        bool tone_bit  = tone_enabled  ? (m_tone_output[ch] != 0) : true;
        bool noise_bit = noise_enabled ? (m_noise_output != 0)    : true;

        if (!(tone_bit && noise_bit)) continue; // silent this instant

        uint8_t ampreg = m_regs[8 + ch];
        int volIndex;
        if (ampreg & 0x10) volIndex = CurrentEnvelopeLevel();
        else                volIndex = ampreg & 0x0f;

        mix += kVolumeTable[volIndex & 0x0f];
    }

    // Average the (up to 3) contributing channels down so the mix
    // doesn't clip when all three are active simultaneously.
    mix /= 3;
    if (mix > 32767) mix = 32767;
    if (mix < -32768) mix = -32768;
    return int16_t(mix);
}

} // namespace ssc
