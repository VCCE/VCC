// padtest.cpp -- gameport wiring checks against the portable PSG core.
//   cl /EHsc /std:c++17 padtest.cpp ..\psg_cart.cpp ..\ym2149.cpp ..\sst39sf040.cpp ..\bandlimiter.cpp

#include <cstdio>
#include <string>
#include "../psg_cart.h"

using namespace psg;

static int gFail = 0;
static void Check(bool ok, const char* name, int got = -1, int want = -1)
{
    std::printf("%s  %s", ok ? "PASS" : "FAIL", name);
    if (got >= 0) std::printf("  -- got $%02X want $%02X", got, want);
    std::printf("\n");
    if (!ok) gFail++;
}

static uint8_t Read(PsgCart& c, int reg)
{
    c.WritePort(0x5E, uint8_t(reg));
    return c.ReadPort(0x5F);
}

static void Expect(PsgCart& c, int reg, int want, const char* name)
{
    const int got = Read(c, reg);
    Check(got == want, name, got, want);
}

int main()
{
    PsgCart c;
    c.WritePort(0x5E, 7); c.WritePort(0x5F, 0x3F);     // both ports input

    // Atari stick (the firmware's STICK A/B test reads these).
    Expect(c, 14, 0xFF, "Atari idle reads $FF");
    c.SetPad(0, PadType::Atari, PadUp | PadA);
    Expect(c, 14, 0xEE, "Atari up + fire");
    Check((Read(c, 14) & 0xF0) == 0xE0, "firmware fire test ((v & $F0) == $E0)");
    c.SetPad(0, PadType::Atari, PadDown | PadRight | PadB);
    Expect(c, 14, 0xD5, "Atari down-right + pin 9");
    c.SetPad(1, PadType::Atari, PadLeft);
    Expect(c, 15, 0xFB, "port B on R15");
    Expect(c, 14, 0xD5, "port A unaffected by port B");

    // Port as output reads its latch, not the pad.
    c.WritePort(0x5E, 7); c.WritePort(0x5F, 0x7F);
    c.WritePort(0x5E, 14); c.WritePort(0x5F, 0x12);
    Expect(c, 14, 0x12, "output port reads latch");
    c.WritePort(0x5E, 7); c.WritePort(0x5F, 0x3F);

    // Genesis 3-button: SEL = control bit 1 (port A) / bit 2 (port B).
    c.SetPad(0, PadType::Genesis3, PadRight | PadB | PadC | PadA | PadStart | PadUp);
    c.WritePort(0x5D, 0x12);                            // SEL A high
    Expect(c, 14, 0xC6, "Genesis3 SEL high: U R B C");
    c.WritePort(0x5D, 0x10);                            // SEL A low
    Expect(c, 14, 0xC2, "Genesis3 SEL low: U, pins 3-4 low, A, Start");
    c.SetPad(0, PadType::Genesis3, 0);
    Expect(c, 14, 0xF3, "Genesis3 SEL low idle: pins 3-4 low (pad ID)");
    c.SetPad(1, PadType::Genesis3, PadB);
    c.WritePort(0x5D, 0x14);                            // SEL B high
    Expect(c, 15, 0xEF, "port B SEL is control bit 2");

    // Genesis 6-button sequence.
    c.SetPad(0, PadType::Genesis6, PadX);
    for (int i = 0; i < 30; i++) c.TickScanline();      // let the count time out
    c.WritePort(0x5D, 0x12);                            // high
    Expect(c, 14, 0xFF, "Genesis6 normal high");
    c.WritePort(0x5D, 0x10); c.WritePort(0x5D, 0x12);   // fall 1
    c.WritePort(0x5D, 0x10); c.WritePort(0x5D, 0x12);   // fall 2
    c.WritePort(0x5D, 0x10);                            // fall 3 (low)
    Expect(c, 14, 0xF0, "Genesis6 3rd low: pins 1-4 low");
    c.WritePort(0x5D, 0x12);                            // high after 3rd
    Expect(c, 14, 0xFB, "Genesis6 extra buttons: X on pin 3");
    c.WritePort(0x5D, 0x10);                            // fall 4
    Expect(c, 14, 0xFF, "Genesis6 4th low: pins 1-4 high");
    for (int i = 0; i < 30; i++) c.TickScanline();
    c.WritePort(0x5D, 0x12);
    Expect(c, 14, 0xFF, "Genesis6 sequence resets after 1.5ms");

    // Reset button keeps control (SEL) but power-on clears it.
    c.SetPad(0, PadType::Genesis3, 0);
    c.ResetButton();
    c.WritePort(0x5E, 7); c.WritePort(0x5F, 0x3F);
    Expect(c, 14, 0xFF, "after reset button: SEL still high");
    c.PowerOn();
    c.WritePort(0x5E, 7); c.WritePort(0x5F, 0x3F);
    Expect(c, 14, 0xF3, "after power-on: SEL low");

    std::printf("\n%s (%d failure%s)\n", gFail ? "FAILED" : "ALL PASSED", gFail, gFail == 1 ? "" : "s");
    return gFail ? 1 : 0;
}
