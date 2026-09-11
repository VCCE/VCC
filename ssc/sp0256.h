// sp0256.h
//
// Standalone, portable emulation of the General Instrument SP0256-AL2
// Narrator Speech Processor, adapted for use inside a Tandy Speech/Sound
// Cartridge (SSC) emulation for VCC.
//
// This is a from-source port of the LPC-12 filter, microsequencer and
// allophone data-format tables from MAME's src/devices/sound/sp0256.cpp /
// sp0256.h (license: BSD-3-Clause, copyright Joseph Zbiciak, Tim Lindner,
// originally written by Joe Zbiciak). The MAME device_sound_interface /
// sound_stream / device_t framework has been replaced with a simple
// pull-based sample generator so the core can run standalone inside a
// Windows DLL. The DSP algorithm, quantization table, opcode data-format
// tables and microsequencer logic are preserved unchanged from the
// original.
//
// The chip requires its own 2KB internal mask ROM image (sp0256-al2.bin,
// CRC32 b504ac15) to be supplied by the caller at runtime -- this code
// does not embed or distribute that ROM. It is placed at byte offset
// $1000 within a 64KB address window, matching the physical chip's
// internal addressing and matching how MAME's coco_ssc.cpp loads it
// (ROM_LOAD("sp0256-al2.bin", 0x1000, 0x0800, ...)).
//
// license:BSD-3-Clause
// Original copyright-holders: Joseph Zbiciak, Tim Lindner (MAME sp0256 core)
// Port for VCC SSC cartridge: 2026

#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <functional>

namespace ssc {

class Sp0256 {
public:
    Sp0256();

    // Load the 2KB SP0256-AL2 mask ROM dump at its native offset ($1000)
    // within the chip's 64KB address window.
    void LoadRom(const uint8_t* al2_2k, size_t len);

    // Notified when the chip's DRQ ("ready for next command") state
    // changes: true = ready to accept an ALD write (used to drive the
    // TMS7040's INT1 line in the SSC).
    std::function<void(bool ready)> DataRequestChanged;

    void Reset();

    // ALD (Address LoaD): write an allophone/command byte (6 bits used).
    void Ald(uint8_t data);

    // True while the chip is speaking (not on standby).
    bool Standby() const { return m_sby_line != 0; }

    // Generates `count` mono samples at the chip's native output rate
    // (clock/CLOCK_DIVIDER, ~10.02kHz at the standard 3.12MHz clock) into
    // `out`, running the microsequencer as needed. Samples are signed
    // 16-bit PCM.
    void GenerateSamples(int16_t* out, int count);

    // Native output sample rate for a given input clock, matching the
    // original CLOCK_DIVIDER constant.
    static int SampleRateForClock(int clock_hz) { return clock_hz / kClockDivider; }

private:
    static constexpr int kClockDivider = 6 * 4 * 13;
    static constexpr int kScbufSize = 4096;
    static constexpr int kScbufMask = kScbufSize - 1;
    static constexpr int kPerPause = 64;
    static constexpr int kPerNoise = 64;
    static constexpr int kFifoAddr = (0x1800 << 3);

    struct Lpc12 {
        int      rpt = 0, cnt = 0;
        uint32_t per = 0, rng = 1;
        int      amp = 0;
        int16_t  f_coef[6] = {0};
        int16_t  b_coef[6] = {0};
        int16_t  z_data[6][2] = {{0}};
        uint8_t  r[16] = {0};
        int      interp = 0;

        int Update(int num_samp, int16_t* out, uint32_t* optr);
        void Regdec();
        static int16_t Limit(int16_t s);
    };

    std::unique_ptr<uint8_t[]> m_rom; // 64KB window
    Lpc12 m_filt;

    int      m_sby_line = 1;
    bool     m_silent = 1;

    std::unique_ptr<int16_t[]> m_scratch;
    uint32_t m_sc_head = 0, m_sc_tail = 0;

    int      m_lrq = 1;
    int      m_ald = 0;
    int      m_pc = 0;
    int      m_stack = 0;
    int      m_fifo_sel = 0;
    int      m_halted = 1;
    uint32_t m_mode = 0;
    uint32_t m_page = 0x1000 << 3;

    uint32_t m_fifo_head = 0, m_fifo_tail = 0, m_fifo_bitp = 0;
    uint16_t m_fifo[64] = {0};

    void SetSby(int line_state);
    uint32_t Getb(int len);
    void Micro();
};

} // namespace ssc
