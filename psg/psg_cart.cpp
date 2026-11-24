// psg_cart.cpp -- see psg_cart.h.

#include "psg_cart.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace psg {

namespace {
// Output gain: one channel at full volume swings about +/-32 after DC
// removal (half the CoCo 6-bit DAC's swing), all three about +/-96.
constexpr double kGain = 64.0;
// Fixed step rate: 2MHz / 8. In 1MHz mode the chip ticks every other step.
constexpr double kTickRate = PsgCart::kMasterClock / 8.0;
constexpr double kTicksPerLine = kTickRate / PsgCart::kScanlineHz;
// Rate measurement window (~0.25s of scanlines).
constexpr int kRateWindowLines = 3934;
constexpr double kPi = 3.14159265358979323846;
}

PsgCart::PsgCart()
    : m_bl(kTickRate)
    , m_sram(kChipSize, 0x00)
{
    UpdateRate(44100.0);
    PowerOn();
}

void PsgCart::PowerOn()
{
    m_bank[0] = m_bank[1] = 0;
    m_control = 0;
    for (auto& p : m_pad) { p.falls = 0; p.idle_lines = 0; }
    m_flash.ResetCommandState();
    ResetButton();
}

void PsgCart::ResetButton()
{
    m_ym.Reset();
    UpdatePortInputs();
    m_ym.SetClock((m_control & 0x01) ? kMasterClock / 2.0 : kMasterClock);
    ResetAudio();
}

void PsgCart::ResetAudio()
{
    m_bl.Reset();
    m_half_phase = false;
    m_emu_ticks = 0.0;
    m_t_out = -double(m_bl.HalfWidth()) - kTicksPerLine;
    m_win_lines = m_win_samples = 0;
    m_dc_x = m_dc_y = 0.0;
}

void PsgCart::UpdateRate(double rate)
{
    m_ticks_per_sample = kTickRate / rate;
    m_bl.SetOutputRate(rate);
    m_dc_pole = 1.0 - 2.0 * kPi * 3.0 / rate;   // ~3Hz DC blocker
}

void PsgCart::ClockChip()
{
    bool tick = true;
    if (m_control & 0x01) {             // 1MHz: every other step
        m_half_phase = !m_half_phase;
        tick = m_half_phase;
    }
    if (tick)
        m_ym.Clock();
    m_bl.Push(float(m_ym.Output()));
}

void PsgCart::LoadFlash(const uint8_t* data, size_t len)
{
    m_flash.Load(data, len);
    m_flash_loaded = len > 0;
}

void PsgCart::ClearFlash()
{
    m_flash.Erase();
    m_flash_loaded = false;
}

void PsgCart::WritePort(uint8_t port, uint8_t data)
{
    switch (port) {
    case 0x5A: m_bank[0] = data; break;
    case 0x5B: m_bank[1] = data; break;
    case 0x5C: // mirror of $FF5D (A0 not decoded for control)
    case 0x5D: {
        const uint8_t old = m_control;
        m_control = data;
        m_ym.SetClock((data & 0x01) ? kMasterClock / 2.0 : kMasterClock);
        for (int p = 0; p < 2; p++) {
            const uint8_t bit = uint8_t(2 << p);
            if ((old ^ data) & bit) {
                m_pad[p].idle_lines = 0;
                if (!(data & bit))
                    m_pad[p].falls++;
            }
        }
        UpdatePortInputs();
        break;
    }
    case 0x5E: m_ym.SelectRegister(data); break;
    case 0x5F: m_ym.WriteData(data); break;
    default: break;
    }
}

uint8_t PsgCart::ReadPort(uint8_t port) const
{
    // Read-back behaviour measured on a real cart, see psg_cart.h.
    switch (port) {
    case 0x5A: return m_bank[0];
    case 0x5B: return m_bank[1];
    case 0x5C:
    case 0x5D: return m_control;
    case 0x5E: return 0xFF;
    case 0x5F: return m_ym.ReadData();
    default:   return 0;
    }
}

// Segment number: bit 7 picks the chip (0 = FLASH, 1 = SRAM), bits 0-5 the
// 8K segment. Bit 6 is assumed not decoded (64-127 mirror 0-63, 192-255
// mirror 128-191); unverified on hardware.
static uint32_t SegmentBase(uint8_t segment)
{
    return uint32_t(segment & 0x3F) * uint32_t(PsgCart::kSegmentSize);
}

uint8_t PsgCart::ReadMemory(uint16_t address) const
{
    const uint8_t segment = m_bank[(address >> 13) & 1];
    const uint32_t offset = SegmentBase(segment) + (address & 0x1FFF);
    return (segment & 0x80) ? m_sram[offset] : m_flash.Read(offset);
}

void PsgCart::CpuWrite(uint16_t address, uint8_t data)
{
    if (!(m_control & 0x08))
        return;                             // write enable off

    if (address >= 0xC000 && address <= 0xFEFF) {
        const uint8_t segment = m_bank[(address >> 13) & 1];
        const uint32_t offset = SegmentBase(segment) + (address & 0x1FFF);
        if (segment & 0x80)
            m_sram[offset] = data;
        else
            m_flash.Write(offset, data);
        return;
    }

    // FLASH command addresses, only with programming enabled (bit 5).
    if (m_control & 0x20) {
        if (address == 0xBFFF)
            m_flash.Write(0x5555, data);
        else if (address == 0xBAAA)
            m_flash.Write(0x2AAA, data);
    }
}

void PsgCart::SetPad(int port, PadType type, uint16_t buttons)
{
    Pad& p = m_pad[port & 1];
    p.type = type;
    p.buttons = buttons;
    UpdatePortInputs();
}

uint8_t PsgCart::PortPins(int port) const
{
    const Pad& p = m_pad[port & 1];
    const uint16_t b = p.buttons;
    // Build the pins as "pressed" bits, then invert (active low).
    uint8_t low = 0;
    auto dirs = [&](uint16_t up, uint16_t down, uint16_t left, uint16_t right) {
        if (b & up) low |= 0x01;
        if (b & down) low |= 0x02;
        if (b & left) low |= 0x04;
        if (b & right) low |= 0x08;
    };

    if (p.type == PadType::Atari) {
        dirs(PadUp, PadDown, PadLeft, PadRight);
        if (b & PadA) low |= 0x10;
        if (b & PadB) low |= 0x20;
        return uint8_t(~low);
    }

    const bool sel = Sel(port);
    const bool six = p.type == PadType::Genesis6;
    if (six && p.falls == 3 && !sel) {
        low |= 0x0F;                            // 6-button ID: pins 1-4 low
        if (b & PadA) low |= 0x10;
        if (b & PadStart) low |= 0x20;
    } else if (six && p.falls == 3 && sel) {
        dirs(PadZ, PadY, PadX, PadMode);        // extra buttons
        if (b & PadB) low |= 0x10;
        if (b & PadC) low |= 0x20;
    } else if (six && p.falls == 4 && !sel) {
        if (b & PadA) low |= 0x10;              // pins 1-4 high
        if (b & PadStart) low |= 0x20;
    } else if (sel) {
        dirs(PadUp, PadDown, PadLeft, PadRight);
        if (b & PadB) low |= 0x10;
        if (b & PadC) low |= 0x20;
    } else {
        dirs(PadUp, PadDown, 0, 0);
        low |= 0x0C;                            // pins 3-4 low: 3-button ID
        if (b & PadA) low |= 0x10;
        if (b & PadStart) low |= 0x20;
    }
    return uint8_t(~low);
}

void PsgCart::UpdatePortInputs()
{
    m_ym.SetPortInput(0, PortPins(0));
    m_ym.SetPortInput(1, PortPins(1));
}

void PsgCart::TickScanline()
{
    m_emu_ticks += kTicksPerLine;

    // 6-button pads drop back to the start of their sequence ~1.5ms
    // (24 scanlines) after the last SEL change.
    for (auto& p : m_pad) {
        if (p.falls && ++p.idle_lines >= 24) {
            p.falls = 0;
            UpdatePortInputs();
        }
    }

    // Measure the host sample rate from how often NextSample() is called.
    if (++m_win_lines >= kRateWindowLines) {
        if (m_win_samples > 0) {
            const double rate = m_win_samples * kScanlineHz / m_win_lines;
            if (m_rate_measured == 0.0 || std::fabs(rate / m_rate_measured - 1.0) > 0.02)
                UpdateRate(rate);
            m_rate_measured = rate;
        }
        m_win_lines = m_win_samples = 0;
    }

    // Audio muted (no sample calls): keep the chip running anyway.
    const double behind = m_emu_ticks - double(m_bl.Count());
    if (behind > 8.0 * kTicksPerLine) {
        while (double(m_bl.Count()) < m_emu_ticks - 2.0 * kTicksPerLine)
            ClockChip();
        m_t_out = double(m_bl.Count()) - m_bl.HalfWidth() - 1.0;
    }
}

uint8_t PsgCart::NextSample()
{
    m_win_samples++;
    const double half = m_bl.HalfWidth();

    // Advance the output clock, servoed to trail emulated time by one
    // filter half-width plus a scanline (the newest time we know about).
    m_t_out += m_ticks_per_sample;
    const double target = m_emu_ticks - half - kTicksPerLine;
    const double err = target - m_t_out;
    if (std::fabs(err) > 6.0 * kTicksPerLine)
        m_t_out = target;
    else
        m_t_out += err * 0.002;
    m_t_out = std::min(m_t_out, m_emu_ticks - half - 1.0);

    // Run the chip far enough for the filter window around m_t_out.
    while (double(m_bl.Count()) <= m_t_out + half + 1.0)
        ClockChip();

    const double x = m_bl.At(m_t_out);
    const double y = x - m_dc_x + m_dc_pole * m_dc_y;
    m_dc_x = x;
    m_dc_y = y;

    double v = y * kGain;
    if (std::fabs(v) > 0.25) {
        // TPDF dither (+/-1 LSB) so quiet passages and fades come out as
        // faint noise rather than coarse, signal-correlated steps.
        m_rng = m_rng * 1664525u + 1013904223u;
        const double r1 = double(m_rng >> 8) / 16777216.0;
        m_rng = m_rng * 1664525u + 1013904223u;
        const double r2 = double(m_rng >> 8) / 16777216.0;
        v += r1 - r2;
    }
    const int s = 0x80 + int(std::floor(v + 0.5));
    return uint8_t(std::clamp(s, 0, 255));
}

} // namespace psg
