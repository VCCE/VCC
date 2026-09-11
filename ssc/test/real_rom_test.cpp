// real_rom_test.cpp
//
// Validation harness that runs the ACTUAL S/SC ROM dumps (pic-7040-510.bin,
// sp0256-al2.bin) through the full SscCore glue layer -- as opposed to the
// other smoke_*.cpp tests, which only exercise the cores with synthetic
// (non-real) data to check for crashes/hangs. This is the strongest check
// possible without a real CoCo/VCC: does the *real* TMS7040 firmware boot
// on this CPU core and drive the AY/SP0256 the way coco_ssc.cpp's wiring
// expects, with no illegal-opcode fetches (which would mean the CPU core
// has a decode bug or has run off into data)?
//
// Usage: real_rom_test <pic-7040-510.bin> <sp0256-al2.bin>
//
// Not part of run_tests.sh, since it requires ROM files this project
// cannot ship. Exits nonzero and prints a diagnosis on any anomaly.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>

#include "../ssc_core.h"

namespace {

std::vector<uint8_t> ReadFile(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "error: can't open %s\n", path); std::exit(2); }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

void RunScanlines(ssc::SscCore& core, int n, std::vector<uint8_t>* samples = nullptr)
{
    for (int i = 0; i < n; i++) {
        core.Tick();
        if (samples) samples->push_back(core.LatchedSample());
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <pic-7040-510.bin> <sp0256-al2.bin>\n", argv[0]);
        return 2;
    }

    auto pic_rom = ReadFile(argv[1]);
    auto spo_rom = ReadFile(argv[2]);
    std::printf("pic ROM: %zu bytes, speech ROM: %zu bytes\n", pic_rom.size(), spo_rom.size());

    int failures = 0;

    ssc::SscCore core;
    bool pic_ok = core.LoadPicRom(pic_rom.data(), pic_rom.size());
    bool spo_ok = core.LoadSpeechRom(spo_rom.data(), spo_rom.size());
    std::printf("LoadPicRom -> %s, LoadSpeechRom -> %s\n", pic_ok ? "ok" : "WRONG SIZE", spo_ok ? "ok" : "WRONG SIZE");
    if (!pic_ok || !spo_ok) { std::printf("FAIL: ROM size mismatch\n"); return 1; }

    core.Reset();

    // ---- 1. Real host reset sequence -----------------------------------
    // Mirrors what a CoCo driver does on $FF7D: strobe bit0 low then high
    // (see ssc_core.cpp WritePort's 0x7d case -- the falling edge resets
    // the TMS7040+AY, the rising edge resets the SP0256).
    core.WritePort(0x7d, 0x00);
    core.WritePort(0x7d, 0x01);

    // ---- 2. Let the firmware run its power-on init for ~0.5s of scanlines ----
    // Real firmware should: initialize its stack/RAM, program the AY to a
    // known state, and settle into an idle/poll loop waiting for a command
    // -- all without ever fetching an undefined opcode.
    const double kTickHz = core.TickRateHz(); // real Hz one Tick() call represents (44100 by default)
    const int kSettleScanlines = int(kTickHz * 0.5); // ~0.5s worth of Tick() calls
    RunScanlines(core, kSettleScanlines);

    std::printf("\nAfter %d scanlines of boot/idle:\n", kSettleScanlines);
    std::printf("  CPU PC = $%04X\n", core.CpuPc());
    std::printf("  illegal-opcode fetches = %u\n", core.CpuIllegalCount());
    std::printf("  AY register writes so far = %u\n", core.AyWriteCount());
    std::printf("  ALD (speech) strobes so far = %u\n", core.AldCount());
    std::printf("  busy flag = %s\n", core.Busy() ? "true" : "false");

    if (core.CpuIllegalCount() > 0) {
        std::printf("  FAIL: illegal opcodes were fetched -- CPU core has run off firmware code.\n");
        failures++;
    }
    if (core.CpuPc() < 0xF000) {
        std::printf("  FAIL: PC is outside the internal ROM ($F000-$FFFF) -- firmware jumped to unmapped/RAM space.\n");
        failures++;
    }
    if (core.AyWriteCount() == 0) {
        std::printf("  FAIL: firmware never wrote to the AY-3-8913 during its init sequence (expected -- real S/SC firmware programs the AY at boot).\n");
        failures++;
    }
    if (!failures) std::printf("  OK: firmware appears to have booted and initialized the AY cleanly.\n");

    // ---- 3. Send a command byte and see if the firmware responds -------
    // We don't have the documented command protocol on hand, so this
    // isn't a claim that command 0x00 does anything specific -- it's a
    // check that sending *a* command byte doesn't crash the CPU core and
    // that the busy handshake (host write -> INT3 -> firmware services it
    // -> BSY release) moves at all, which only happens if real code path
    // in the firmware's INT3 handler runs.
    uint32_t pre_illegal = core.CpuIllegalCount();
    core.WritePort(0x7e, 0x00);
    std::printf("\nAfter sending command byte 0x00 to $FF7E, busy=%s\n", core.Busy() ? "true" : "false");
    RunScanlines(core, kSettleScanlines);
    std::printf("After %d more scanlines:\n", kSettleScanlines);
    std::printf("  CPU PC = $%04X\n", core.CpuPc());
    std::printf("  illegal-opcode fetches (delta) = %u\n", core.CpuIllegalCount() - pre_illegal);
    std::printf("  busy flag = %s\n", core.Busy() ? "true" : "false");
    std::printf("  AY register writes (total) = %u\n", core.AyWriteCount());
    std::printf("  ALD (speech) strobes (total) = %u\n", core.AldCount());

    if (core.CpuIllegalCount() > pre_illegal) {
        std::printf("  FAIL: command byte triggered illegal opcode fetches.\n");
        failures++;
    }
    if (core.Busy()) {
        std::printf("  NOTE: still busy after %.2fs -- either this command byte blocks waiting for more\n", kSettleScanlines / kTickHz);
        std::printf("        data (plausible -- many real commands are multi-byte), or the firmware is stuck.\n");
        std::printf("        Not counted as a failure by itself; see PC below for a stuck-loop check.\n");
    } else {
        std::printf("  OK: busy handshake completed -- firmware serviced the command and released BSY.\n");
    }

    // ---- 4. Render a longer stretch to WAV for manual/spectral inspection ----
    std::vector<uint8_t> samples;
    core.WritePort(0x7d, 0x00);
    core.WritePort(0x7d, 0x01); // fresh reset
    RunScanlines(core, int(kTickHz * 2), &samples); // 2 seconds, capturing every sample

    // basic amplitude stats
    int minv = 255, maxv = 0;
    double sum = 0, sumsq = 0;
    for (uint8_t s : samples) {
        minv = std::min(minv, int(s));
        maxv = std::max(maxv, int(s));
        sum += s;
        sumsq += double(s) * s;
    }
    double mean = sum / samples.size();
    double var = sumsq / samples.size() - mean * mean;
    std::printf("\n2-second post-reset audio capture (%zu samples):\n", samples.size());
    std::printf("  min=%d max=%d mean=%.1f stddev=%.2f\n", minv, maxv, mean, std::sqrt(std::max(0.0, var)));
    if (minv == maxv) {
        std::printf("  NOTE: dead flat output -- silence is plausible for an idle cartridge (matches real hardware:\n");
        std::printf("        it's silent until told to speak/play), not necessarily a bug.\n");
    }

    // Write a WAV so a human can actually listen to it.
    {
        std::ofstream wav("real_rom_capture.wav", std::ios::binary);
        uint32_t sr = uint32_t(kTickHz), data_bytes = uint32_t(samples.size());
        uint32_t riff_size = 36 + data_bytes;
        uint16_t channels = 1, bits = 8;
        uint32_t byte_rate = sr * channels * bits / 8;
        uint16_t block_align = channels * bits / 8;
        wav.write("RIFF", 4);
        wav.write(reinterpret_cast<char*>(&riff_size), 4);
        wav.write("WAVE", 4);
        wav.write("fmt ", 4);
        uint32_t fmt_size = 16; wav.write(reinterpret_cast<char*>(&fmt_size), 4);
        uint16_t fmt_tag = 1; wav.write(reinterpret_cast<char*>(&fmt_tag), 2);
        wav.write(reinterpret_cast<char*>(&channels), 2);
        wav.write(reinterpret_cast<char*>(&sr), 4);
        wav.write(reinterpret_cast<char*>(&byte_rate), 4);
        wav.write(reinterpret_cast<char*>(&block_align), 2);
        wav.write(reinterpret_cast<char*>(&bits), 2);
        wav.write("data", 4);
        wav.write(reinterpret_cast<char*>(&data_bytes), 4);
        wav.write(reinterpret_cast<const char*>(samples.data()), std::streamsize(samples.size()));
        std::printf("  wrote real_rom_capture.wav (%u bytes payload)\n", data_bytes);
    }

    std::printf("\n%s (%d failure(s))\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
