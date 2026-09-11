// real_sp0256_speech_test.cpp
//
// Focused validation of the SP0256 core against the REAL sp0256-al2.bin
// ROM, feeding it actual non-pause allophone codes directly (bypassing
// the TMS7040 firmware entirely) to check whether the core can produce
// non-silent audio at all. This isolates the SP0256 chip-level emulation
// from the still-unknown TMS7040 firmware command protocol -- if this
// test shows real audio for ordinary allophones, the SP0256 core itself
// is not the problem, and whatever's silencing the real cartridge in
// VCC is either the AY/SP0256 mix downstream or something specific to
// how the real firmware drives ALD in its speech routine.
//
// Usage: real_sp0256_speech_test <sp0256-al2.bin>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>

#include "../sp0256.h"

namespace {
std::vector<uint8_t> ReadFile(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "error: can't open %s\n", path); std::exit(2); }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <sp0256-al2.bin>\n", argv[0]);
        return 2;
    }
    auto rom = ReadFile(argv[1]);
    std::printf("speech ROM: %zu bytes\n", rom.size());

    ssc::Sp0256 spo;
    spo.LoadRom(rom.data(), rom.size());
    spo.Reset();

    // SP0256-AL2 standard allophone table: 0 = PA1 (pause, legitimately
    // silent), 1-63 are real phonemes (/AA/, /IY/, /EY/, /S/, etc.). We
    // drive codes 1 through 20 back-to-back -- a representative sample,
    // not the full 63 -- and measure whether real audio energy comes out
    // for each one.
    std::vector<int16_t> all_samples;
    int failures = 0;
    int16_t global_min = 0, global_max = 0;

    const int kNumCodes = 63;
    for (int code = 1; code <= kNumCodes; code++) {
        if (!spo.Standby()) {
            // shouldn't happen between codes, but drain just in case
            for (int i = 0; i < 20000 && !spo.Standby(); i++) {
                int16_t s;
                spo.GenerateSamples(&s, 1);
            }
        }
        spo.Ald(uint8_t(code));

        std::vector<int16_t> samples;
        samples.reserve(20000);
        int guard = 0;
        while (!spo.Standby() && guard < 30000) {
            int16_t s;
            spo.GenerateSamples(&s, 1);
            samples.push_back(s);
            guard++;
        }

        int16_t minv = 0, maxv = 0;
        double sumsq = 0;
        for (int16_t s : samples) {
            minv = std::min(minv, s);
            maxv = std::max(maxv, s);
            sumsq += double(s) * s;
        }
        double rms = samples.empty() ? 0.0 : std::sqrt(sumsq / samples.size());

        std::printf("allophone %2d: %5zu samples, min=%6d max=%6d rms=%8.1f %s\n",
                    code, samples.size(), minv, maxv, rms,
                    rms > 50.0 ? "OK (audible)" : "SILENT/near-silent");

        if (rms <= 50.0) failures++;
        global_min = std::min(global_min, minv);
        global_max = std::max(global_max, maxv);

        all_samples.insert(all_samples.end(), samples.begin(), samples.end());
    }

    std::printf("\nGLOBAL peak across all %d allophones: min=%d max=%d (int16 full scale is +/-32767)\n",
                kNumCodes, global_min, global_max);

    // Write a WAV of the whole run (10kHz mono, matching the SP0256's
    // native sample rate at the SSC's 3.12MHz clock) so it can actually
    // be listened to.
    {
        std::ofstream wav("real_sp0256_speech.wav", std::ios::binary);
        uint32_t sr = 10000, data_bytes = uint32_t(all_samples.size() * 2);
        uint32_t riff_size = 36 + data_bytes;
        uint16_t channels = 1, bits = 16;
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
        wav.write(reinterpret_cast<const char*>(all_samples.data()), std::streamsize(data_bytes));
        std::printf("\nwrote real_sp0256_speech.wav (%u bytes payload, %d allophones concatenated)\n",
                    data_bytes, kNumCodes);
    }

    std::printf("\n%s (%d of %d allophones came out silent/near-silent)\n",
                failures == 0 ? "PASS" : (failures < kNumCodes ? "PARTIAL" : "FAIL"), failures, kNumCodes);
    return failures == kNumCodes ? 1 : 0;
}
