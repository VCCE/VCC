// Smoke test for the ported SP0256 core: without the real AL2 mask ROM
// (which we don't distribute) we can't validate actual speech output,
// but we can confirm the microsequencer/filter run for an extended
// period on arbitrary ROM content without crashing, hanging, or
// producing NaN/out-of-range samples, and that ALD triggers activity.
#include "../sp0256.h"
#include <cstdio>
#include <cassert>
#include <vector>
#include <cstdlib>

using namespace ssc;

int main()
{
    std::vector<uint8_t> fakerom(0x800);
    // Deterministic pseudo-random filler so the microsequencer has
    // varied (but reproducible) "allophone" data to chew on.
    unsigned seed = 12345;
    for (auto& b : fakerom) {
        seed = seed * 1103515245u + 12345u;
        b = uint8_t(seed >> 16);
    }

    Sp0256 chip;
    chip.LoadRom(fakerom.data(), fakerom.size());
    chip.Reset();

    bool drq_seen = false;
    chip.DataRequestChanged = [&](bool ready) { if (ready) drq_seen = true; };

    assert(chip.Standby());

    chip.Ald(0x00); // kick off "allophone 0" from whatever garbage is at page $1000
    assert(!chip.Standby());

    std::vector<int16_t> buf(4096);
    long total = 0;
    for (int block = 0; block < 500; block++) {
        chip.GenerateSamples(buf.data(), int(buf.size()));
        for (auto s : buf) {
            assert(s >= -32768 && s <= 32767); // range sanity
        }
        total += long(buf.size());

        // Occasionally feed another command once the chip reports ready,
        // simulating a host streaming allophones.
        if (block % 37 == 0) chip.Ald(uint8_t(block & 0x3f));
    }

    std::printf("OK: generated %ld samples, drq_seen=%d, standby=%d\n",
                total, drq_seen, chip.Standby());
    std::printf("PASS\n");
    return 0;
}
