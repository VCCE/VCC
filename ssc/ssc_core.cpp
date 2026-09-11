// ssc_core.cpp -- see ssc_core.h for design notes and provenance.

#include "ssc_core.h"
#include <algorithm>
#include <cmath>

namespace ssc {

// Port C bit assignments, ported from MAME's coco_ssc.cpp.
// (A8/A9/A10 -- the RAM address extension bits -- share their bit
// positions with BC1/BDIR/CS below; they aren't named separately here
// because the RAM address is always derived from the whole port C byte
// masked to 11 bits, matching the original's address computation.)
static constexpr uint8_t C_BC1 = 0x01;
static constexpr uint8_t C_RRW = 0x08;
static constexpr uint8_t C_BDR = 0x08;
static constexpr uint8_t C_RCS = 0x10;
static constexpr uint8_t C_ALD = 0x20;
static constexpr uint8_t C_ACS = 0x40;
static constexpr uint8_t C_BSY = 0x80;

// See SetTickRateHz() / m_tick_hz in ssc_core.h for why this is a runtime
// value (default 44100Hz) rather than a compile-time scanline-rate
// constant -- it's driven by how often the host ABI actually calls Tick().
static constexpr double kTmsInputClockHz = 1789772.0; // CoCo E-clock (~894886Hz) doubled on-cartridge
static constexpr double kSpoClockHz   = 3120000.0;     // SP0256's own ceramic-resonator clock
static const double kSpoSampleRate    = Sp0256::SampleRateForClock(int(kSpoClockHz)); // 10000Hz

// SAC (Sound Activity Circuit) constants, ported from cocossc_sac_device
static constexpr float kSacHpfAlpha    = 0.99f;
static constexpr float kSacAttackCoeff = 0.0026f;
static constexpr float kSacDecayCoeff  = 0.0003f;
static constexpr float kSacThreshOn    = 0.05f;
static constexpr float kSacThreshOff   = 0.01f;

static constexpr float kSp0256Gain = 1.75f;
static constexpr float kAy8913Gain = 2.0f;

SscCore::SscCore()
{
    m_cpu.InPortA  = [this]() { return OnPortARead(); };
    m_cpu.OutPortB = [this](uint8_t d) { OnPortBWrite(d); };
    m_cpu.InPortC  = [this]() { return OnPortCRead(); };
    m_cpu.OutPortC = [this](uint8_t d) { OnPortCWrite(d); };
    m_cpu.InPortD  = [this]() { return OnPortDRead(); };
    m_cpu.OutPortD = [this](uint8_t d) { OnPortDWrite(d); };

    m_spo.DataRequestChanged = [this](bool ready) { OnSpeechDrqChanged(ready); };

    m_ay.SetClock(kTmsInputClockHz); // AY is clocked from the same doubled-E line as the PIC
}

bool SscCore::LoadPicRom(const uint8_t* data, size_t len)
{
    m_cpu.LoadRom(data, len);
    m_pic_rom_loaded = (len == 4096);
    return m_pic_rom_loaded;
}

bool SscCore::LoadSpeechRom(const uint8_t* data, size_t len)
{
    m_spo.LoadRom(data, len);
    m_speech_rom_loaded = (len == 0x800);
    return m_speech_rom_loaded;
}

void SscCore::Reset()
{
    m_reset_line = 1;
    m_tms_busy = false;
    m_porta_latch = 0;
    m_portb_latch = 0;
    m_portc_latch = 0xff;
    m_portd_latch = 0;
    m_cpu_cycle_accum = 0.0;
    m_spo_sample_accum = 0.0;
    m_last_spo_sample = 0;
    m_latched_sample = 128;
    m_sac_hpf_prev_in = m_sac_hpf_prev_out = m_sac_envelope = 0.0f;
    m_sac_active = false;
    std::memset(m_staticram, 0, sizeof(m_staticram));

    m_cpu.Reset();
    m_ay.Reset();
    m_spo.Reset();
}

uint8_t SscCore::ReadPort(uint8_t port)
{
    switch (port)
    {
        case 0x7d:
            return 0xff;

        case 0x7e:
        {
            uint8_t data = 0x1f;
            if (!m_tms_busy)     data |= 0x80;
            if (m_spo.Standby()) data |= 0x40;
            if (m_sac_active == false) data |= 0x20; // bit set = "not currently making sound"
            return data;
        }

        // Any port that isn't ours must return 0, not 0xff. VCC's MPI
        // arbitrates PackPortRead() across all four slots by taking the
        // first NON-ZERO response from any loaded cartridge (mirroring
        // real open-collector bus behavior, where a device that isn't
        // being addressed stays silent rather than driving the bus).
        // Returning 0xff here for ports we don't own -- e.g. the FD-502
        // disk controller's own I/O ports -- would hijack every other
        // cartridge's port reads out from under it the moment this
        // cartridge is inserted, corrupting status bytes read from
        // unrelated hardware (this was reported as a spurious "?WP
        // ERROR" from DECB's DIR command while this cartridge was
        // plugged into an MPI slot alongside a disk controller).
        default:
            return 0x00;
    }
}

void SscCore::WritePort(uint8_t port, uint8_t data)
{
    switch (port)
    {
        case 0x7d:
            if (data & 1) m_spo.Reset();
            if (((m_reset_line & 1) == 1) && ((data & 1) == 0)) {
                m_cpu.Reset();
                m_ay.Reset();
                m_tms_busy = false;
            }
            m_reset_line = data;
            break;

        case 0x7e:
            m_porta_latch = data;
            m_tms_busy = true;
            m_command_count++;
            m_cpu.SetIrqLine(Tms7000Irq::Int3, true);
            break;

        default:
            break;
    }
}

uint8_t SscCore::OnPortARead()
{
    uint8_t v = m_porta_latch;
    m_cpu.SetIrqLine(Tms7000Irq::Int3, false);
    return v;
}

void SscCore::OnPortBWrite(uint8_t data)
{
    m_portb_latch = data;
}

uint8_t SscCore::OnPortCRead()
{
    return m_portc_latch;
}

void SscCore::OnPortCWrite(uint8_t data)
{
    if ((data & C_RCS) == 0 && (data & C_RRW) == 0) // static RAM write
    {
        uint16_t address = uint16_t((uint16_t(data) << 8) | m_portb_latch) & 0x7ff;
        m_staticram[address] = m_portd_latch;
    }

    if ((data & C_ACS) == 0) // AY chip selected
    {
        if ((data & (C_BDR | C_BC1)) == (C_BDR | C_BC1)) // BDIR=1, BC1=1: latch address
            m_ay.SelectRegister(m_portd_latch);

        if (((data & C_BDR) == C_BDR) && ((data & C_BC1) == 0)) { // BDIR=1, BC1=0: write data
            m_ay.WriteData(m_portd_latch);
            m_ay_write_count++;
        }
    }

    if (((m_portc_latch & C_ALD) == C_ALD) && ((data & C_ALD) == 0) && (m_portd_latch < 64)) {
        m_spo.Ald(m_portd_latch); // ALD strobe (falling edge)
        m_ald_count++;
        // Codes 0-4 are the standard SP0256-AL2 pause variants (PA1-PA5)
        // -- legitimately silent, and the firmware appears to re-strobe
        // one of these continuously as part of its own idle-loop
        // housekeeping (confirmed: AldCount() climbs at thousands/sec
        // even with zero host commands, same as AyWriteCount()). Track
        // strobes of a genuine speech code (5-63) separately so this
        // stops being a false-positive "is it really talking" signal.
        if (m_portd_latch >= 5) m_ald_speech_count++;
    }

    if (((m_portc_latch & C_BSY) == 0) && ((data & C_BSY) == C_BSY))
        m_tms_busy = false; // BSY handshake released (rising edge)

    m_portc_latch = data;
}

uint8_t SscCore::OnPortDRead()
{
    if (((m_portc_latch & C_RCS) == 0) && ((m_portc_latch & C_RRW) == C_RRW))
    {
        uint16_t address = uint16_t((uint16_t(m_portc_latch) << 8) | m_portb_latch) & 0x7ff;
        m_portd_latch = m_staticram[address];
    }

    if ((m_portc_latch & C_ACS) == 0)
    {
        if (((m_portc_latch & C_BDR) == 0) && ((m_portc_latch & C_BC1) == C_BC1))
            m_portd_latch = m_ay.ReadData();
    }

    return m_portd_latch;
}

void SscCore::OnPortDWrite(uint8_t data)
{
    m_portd_latch = data;
}

void SscCore::OnSpeechDrqChanged(bool ready)
{
    m_cpu.SetIrqLine(Tms7000Irq::Int1, ready);
}

bool SscCore::SoundActivityOutput(float x)
{
    // High-pass filter to remove DC offset, then an asymmetric-attack/
    // decay envelope follower with hysteresis -- ported directly from
    // cocossc_sac_device::sound_stream_update / sound_activity_circuit_output.
    float y = kSacHpfAlpha * (m_sac_hpf_prev_out + x - m_sac_hpf_prev_in);
    m_sac_hpf_prev_in = x;
    m_sac_hpf_prev_out = y;

    float rect = std::fabs(y);
    if (rect > m_sac_envelope) m_sac_envelope += (rect - m_sac_envelope) * kSacAttackCoeff;
    else                       m_sac_envelope += (rect - m_sac_envelope) * kSacDecayCoeff;

    if (m_sac_active && m_sac_envelope < kSacThreshOff) m_sac_active = false;
    else if (m_sac_envelope > kSacThreshOn) m_sac_active = true;

    return m_sac_active;
}

void SscCore::Tick()
{
    // --- advance the PIC7040 ---
    m_cpu_cycle_accum += (kTmsInputClockHz / 2.0) / m_tick_hz;
    int cpu_cycles = int(m_cpu_cycle_accum);
    m_cpu_cycle_accum -= cpu_cycles;
    if (cpu_cycles > 0) m_cpu.Execute(cpu_cycles);

    // --- advance the AY-3-8913 ---
    int16_t ay_sample = m_ay.Render(1.0 / m_tick_hz);

    // --- advance the SP0256 (its own, independent, ~10kHz clock) ---
    m_spo_sample_accum += kSpoSampleRate / m_tick_hz;
    while (m_spo_sample_accum >= 1.0) {
        m_spo.GenerateSamples(&m_last_spo_sample, 1);
        m_spo_sample_accum -= 1.0;
    }

    // --- Sound Activity Circuit status (feeds the $FF7E status bit) ---
    SoundActivityOutput(ay_sample / 32768.0f);

    // --- mix and latch the output sample as an unsigned DAC-style byte ---
    float mix = (ay_sample / 32768.0f) * kAy8913Gain + (m_last_spo_sample / 32768.0f) * kSp0256Gain;
    mix = std::max(-1.0f, std::min(1.0f, mix));
    m_latched_sample = uint8_t(std::lround(mix * 127.0f) + 128);
}

} // namespace ssc
