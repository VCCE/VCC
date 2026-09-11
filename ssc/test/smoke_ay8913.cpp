// Functional test for the AY-3-8913 core: program channel A with a known
// tone period, render audio, and verify the measured frequency (by
// counting zero-crossings) matches the datasheet formula
// f = clock / (16 * TP).
#include "../ay8913.h"
#include <cstdio>
#include <cmath>
#include <cassert>
#include <vector>

using namespace ssc;

static double MeasureFrequency(Ay8913& ay, double sample_rate, int nsamples)
{
    std::vector<int16_t> buf(nsamples);
    for (int i = 0; i < nsamples; i++)
        buf[i] = ay.Render(1.0 / sample_rate);

    // The AY's tone output is a unipolar square wave (0 or +vol), not a
    // bipolar one, so measure by counting level *transitions* rather
    // than zero crossings.
    int transitions = 0;
    for (int i = 1; i < nsamples; i++)
        if (buf[i - 1] != buf[i]) transitions++;

    double seconds = nsamples / sample_rate;
    // Each full cycle produces 2 transitions (low->high, high->low).
    return (transitions / 2.0) / seconds;
}

int main()
{
    const double clock_hz = 1789772.0; // doubled CoCo E-clock, as the SSC uses

    // Test a few tone periods across the range.
    int periods[] = { 100, 255, 512, 1000 };
    bool all_ok = true;

    for (int tp : periods) {
        Ay8913 ay;
        ay.SetClock(clock_hz);
        ay.SelectRegister(0); ay.WriteData(uint8_t(tp & 0xff));       // tone A fine
        ay.SelectRegister(1); ay.WriteData(uint8_t((tp >> 8) & 0x0f)); // tone A coarse
        ay.SelectRegister(8); ay.WriteData(0x0f);                      // channel A full volume, no envelope
        ay.SelectRegister(7); ay.WriteData(0b111110);                  // tone A enabled, others disabled (active-low bits)

        double expected = clock_hz / (16.0 * tp);
        // Sample well above Nyquist for the tone under test so our simple
        // zero-crossing counter is accurate.
        double sample_rate = std::max(50000.0, expected * 40.0);
        double measured = MeasureFrequency(ay, sample_rate, int(sample_rate * 0.2));

        double err_pct = std::fabs(measured - expected) / expected * 100.0;
        std::printf("TP=%4d expected=%8.2fHz measured=%8.2fHz err=%.2f%%\n",
                    tp, expected, measured, err_pct);

        if (err_pct > 2.0) all_ok = false;
    }

    // Sanity: mixer disabled AND amplitude 0 should produce true silence.
    {
        Ay8913 ay;
        ay.SetClock(clock_hz);
        ay.SelectRegister(0); ay.WriteData(50);
        ay.SelectRegister(8); ay.WriteData(0x00); // amplitude 0
        ay.SelectRegister(7); ay.WriteData(0xff); // everything disabled
        bool silent = true;
        for (int i = 0; i < 1000; i++)
            if (ay.Render(1.0 / 50000.0) != 0) silent = false;
        std::printf("amplitude-0 silence check: %s\n", silent ? "PASS" : "FAIL");
        if (!silent) all_ok = false;
    }

    // Sanity: full mixer disable with nonzero amplitude should produce a
    // *constant* (non-oscillating) output -- disabling both tone and
    // noise for a channel ties that mixer input high, per the real AY's
    // AND-gate mixer, so the channel still contributes a constant level.
    {
        Ay8913 ay;
        ay.SetClock(clock_hz);
        ay.SelectRegister(0); ay.WriteData(50);
        ay.SelectRegister(8); ay.WriteData(0x0f);
        ay.SelectRegister(7); ay.WriteData(0xff);
        int16_t first = ay.Render(1.0 / 50000.0);
        bool constant = true;
        for (int i = 0; i < 1000; i++)
            if (ay.Render(1.0 / 50000.0) != first) constant = false;
        std::printf("mixer-all-off constant-level check: %s\n", constant ? "PASS" : "FAIL");
        if (!constant) all_ok = false;
    }

    std::printf(all_ok ? "PASS\n" : "FAIL\n");
    return all_ok ? 0 : 1;
}
