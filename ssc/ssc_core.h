// ssc_core.h
//
// Top-level glue for the Tandy Speech/Sound Cartridge emulation: wires
// together the TMS7040 (PIC7040) microcontroller, the AY-3-8913 PSG, the
// SP0256-AL2 speech chip and 2KB of static RAM exactly as the real
// hardware -- and MAME's coco_ssc.cpp reference model -- connect them:
//
//   Port A: host -> chip command byte (from CoCo $FF7E write)
//   Port B: RAM address bits A0-A7
//   Port C: bus controller (RAM A8-A10/R-W/CS, AY BC1/BDIR/CS, SP0256
//           ALD strobe, BUSY handshake line back to the host)
//   Port D: 8-bit data bus shared by RAM, the AY, and port-C latch data
//
// This class is framework-agnostic (no VCC/Windows types) so it can be
// unit-tested standalone; ssc_dll.cpp adapts it to VCC's cpak DLL ABI.
//
// Provenance: the port/bit wiring is a port of the logic in MAME's
// src/devices/bus/coco/coco_ssc.cpp (license: BSD-3-Clause, copyright
// tim lindner).
//
// license:BSD-3-Clause / port for VCC SSC cartridge: 2026

#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include "tms7000.h"
#include "ay8913.h"
#include "sp0256.h"

namespace ssc {

class SscCore {
public:
    SscCore();

    // ROM images: pic-7040-510.bin (4096 bytes) and sp0256-al2.bin
    // (2048 bytes). Returns false if a buffer is the wrong size (still
    // loads what it can -- see LoadRoms in ssc_core.cpp).
    bool LoadPicRom(const uint8_t* data, size_t len);
    bool LoadSpeechRom(const uint8_t* data, size_t len);
    bool RomsLoaded() const { return m_pic_rom_loaded && m_speech_rom_loaded; }

    void Reset();

    // CoCo-side I/O, matching coco_ssc.cpp's ff7d_read/ff7d_write:
    //   port 0x7D: control/reset
    //   port 0x7E: command byte / status
    uint8_t ReadPort(uint8_t port);
    void WritePort(uint8_t port, uint8_t data);

    // Advance emulation by exactly one Tick()-call's worth of real
    // elapsed time and latch a new output audio sample. IMPORTANT: how
    // much real time that is depends entirely on how often the *host*
    // actually calls Tick(), which is a per-ABI-generation detail, not
    // a universal constant -- see SetTickRateHz() below. Defaults to
    // 44100Hz (one call per audio sample), which is what real VCC does
    // for the classic ModuleAudioSample() pak ABI (confirmed directly
    // against VCC's own source, both current main and the vcc-2.1.9.1
    // tag: audio.cpp's AudioOut()/GetDACSample() pulls the cartridge's
    // audio export at a real, wall-clock-accurate 44100Hz with no
    // batching in between -- NOT the ~15720Hz video scanline rate an
    // earlier version of this code assumed here).
    void Tick();

    // Overrides the real-world rate (in Hz) that one Tick() call is
    // assumed to represent. Only needed if your VCC generation's pak
    // ABI ticks this differently than the 44100Hz default above -- e.g.
    // the newer "cpak" ABI's PakProcessHorizontalSync callback really
    // does appear to fire at the classic ~15720Hz CoCo scanline rate
    // (per an embedded comment in VCCE/VCC's own current orch90.cpp:
    // "called every scan line 262 Lines * 60 Frames = 15780 Hz 15720"),
    // which is a genuinely different cadence than the old ABI's
    // 44100Hz audio-sample pull -- see ssc_dll.cpp.
    void SetTickRateHz(double hz) { m_tick_hz = hz; }
    double TickRateHz() const { return m_tick_hz; }

    // Returns the most recently latched output sample as an unsigned
    // 8-bit DAC-style code (0-255, ~128 = silence) -- the same
    // convention VCC's other sound paks (e.g. Orchestra-90) use for
    // PakSampleAudio(), so ssc_dll.cpp can pack it straight into both
    // channels of the returned unsigned short.
    uint8_t LatchedSample() const { return m_latched_sample; }

    // Debug/introspection, mainly for real-ROM validation and bug
    // reports -- not used by ssc_dll.cpp in normal operation.
    uint16_t CpuPc() const { return m_cpu.PC(); }
    uint32_t CpuIllegalCount() const { return m_cpu.IllegalCount(); }
    uint32_t AyWriteCount() const { return m_ay_write_count; }
    uint32_t AldCount() const { return m_ald_count; }
    // Subset of AldCount() where the strobed code was a genuine
    // phoneme (5-63), not one of the PA1-PA5 pause variants (0-4) the
    // firmware appears to spam continuously on its own as idle-loop
    // housekeeping. Unlike AldCount(), this stays at 0 unless the
    // firmware is actually asking the SP0256 to say a real sound.
    uint32_t AldSpeechCount() const { return m_ald_speech_count; }
    bool     Busy() const { return m_tms_busy; }
    // Count of command bytes the HOST has written to $FF7E since reset.
    // Unlike AyWriteCount()/AldCount() (which climb on their own from
    // the firmware's internal idle-loop bus chatter, even with zero
    // host commands), this increments ONLY on a genuine write from the
    // CoCo side -- the cleanest available signal for "is the program
    // actually talking to this cartridge".
    uint32_t CommandCount() const { return m_command_count; }

private:
    Tms7000 m_cpu;
    Ay8913  m_ay;
    Sp0256  m_spo;
    uint8_t m_staticram[2048] = {0};

    bool m_pic_rom_loaded = false;
    bool m_speech_rom_loaded = false;

    // $FF7D/$FF7E glue state (mirrors coco_ssc_device's members)
    uint8_t m_reset_line = 1;
    bool    m_tms_busy = false;
    uint8_t m_porta_latch = 0;

    // last value driven on ports B/C/D, needed for edge detection and
    // for combinational read-back (port B/D are also readable)
    uint8_t m_portb_latch = 0;
    uint8_t m_portc_latch = 0xff;
    uint8_t m_portd_latch = 0;

    // Sound Activity Circuit (SAC): a simple envelope-follower + high
    // pass filter on the AY's output, used only to derive the $FF7E
    // "sound active" status bit -- ported from cocossc_sac_device.
    float m_sac_hpf_prev_in = 0.0f;
    float m_sac_hpf_prev_out = 0.0f;
    float m_sac_envelope = 0.0f;
    bool  m_sac_active = false;

    uint8_t m_latched_sample = 128;

    // diagnostic counters (see AyWriteCount()/AldCount() above)
    uint32_t m_ay_write_count = 0;
    uint32_t m_ald_count = 0;
    uint32_t m_ald_speech_count = 0;
    uint32_t m_command_count = 0;

    // fractional accumulators for cross-clock-domain ticking
    double m_cpu_cycle_accum = 0.0;
    double m_spo_sample_accum = 0.0;

    // see SetTickRateHz() above
    double m_tick_hz = 44100.0;
    int16_t m_last_spo_sample = 0;

    // port callbacks wired to m_cpu
    uint8_t OnPortARead();
    void    OnPortBWrite(uint8_t data);
    uint8_t OnPortCRead();
    void    OnPortCWrite(uint8_t data);
    uint8_t OnPortDRead();
    void    OnPortDWrite(uint8_t data);

    void OnSpeechDrqChanged(bool ready);

    bool SoundActivityOutput(float ay_sample_normalized);
};

} // namespace ssc
