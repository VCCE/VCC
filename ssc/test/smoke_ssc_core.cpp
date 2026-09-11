// End-to-end smoke test for SscCore: without the real PIC7040/SP0256
// ROM dumps (not distributed here -- see README.md) this can't validate
// actual firmware behavior, but it exercises the full port-glue wiring,
// timing accumulators, and audio mixing path against synthetic ROM data
// for an extended run to catch crashes, hangs, and out-of-range output.
#include "../ssc_core.h"
#include <cstdio>
#include <cassert>
#include <vector>

using namespace ssc;

int main()
{
    std::vector<uint8_t> pic_rom(4096);
    std::vector<uint8_t> spo_rom(0x800);
    unsigned seed = 42;
    auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return uint8_t(seed >> 16); };
    for (auto& b : pic_rom) b = rnd();
    for (auto& b : spo_rom) b = rnd();

    // Reset vector -> $F000, matching the smoke_cpu test, so the CPU
    // has a defined (if arbitrary) place to start executing synthetic
    // "firmware".
    pic_rom[0xffe] = 0xf0;
    pic_rom[0xfff] = 0x00;

    SscCore ssc;
    assert(ssc.LoadPicRom(pic_rom.data(), pic_rom.size()));
    assert(ssc.LoadSpeechRom(spo_rom.data(), spo_rom.size()));
    assert(ssc.RomsLoaded());
    ssc.Reset();

    // $FF7D read should always be 0xFF.
    assert(ssc.ReadPort(0x7d) == 0xff);

    // Writing $FF7E should raise the busy flag (bit7 clear) immediately.
    ssc.WritePort(0x7e, 0x55);
    uint8_t status = ssc.ReadPort(0x7e);
    assert((status & 0x80) == 0); // busy -> bit7 clear

    // Run for a few seconds' worth of Tick() calls (each representing
    // 1/ssc.TickRateHz() sec of real time, 44100Hz by default -- see
    // SetTickRateHz() in ssc_core.h) and make sure nothing crashes/hangs
    // and the output sample stays in range.
    const double tick_hz = ssc.TickRateHz();
    const int scanlines = int(tick_hz * 3);
    for (int i = 0; i < scanlines; i++) {
        ssc.Tick();
        uint8_t s = ssc.LatchedSample();
        (void)s; // uint8_t is inherently in [0,255]; this just confirms the call is safe

        // Occasionally poke the host-facing ports as real 6809 code would.
        if (i % 977 == 0) ssc.WritePort(0x7e, uint8_t(i));
        if (i % 5000 == 0) { uint8_t p = ssc.ReadPort(0x7e); (void)p; }
        if (i % 100000 == 99999) { ssc.WritePort(0x7d, 0x00); ssc.WritePort(0x7d, 0x01); } // pulse cart reset line
    }

    std::printf("OK: ran %d ticks (%.1f seconds of CoCo time) without crashing/hanging\n",
                scanlines, scanlines / tick_hz);
    std::printf("PASS\n");
    return 0;
}
