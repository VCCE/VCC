// psgharness.cpp -- loads psg.dll and drives its exports without VCC.
//
//   psgharness <path-to-psg.dll> <out-dir>
//
// Writes a test FLASH image and ini into <out-dir>, runs the checks, and
// writes psg_demo.wav (a short tune) for listening.

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

#include <vcc/bus/cpak_cartridge_definitions.h>

namespace {

PakInitializeModuleFunction      Init;
PakResetModuleFunction           Reset;
PakResetModuleFunction           PowerOn;
PakWritePortModuleFunction       WritePort;
PakReadPortModuleFunction        ReadPort;
PakReadMemoryByteModuleFunction  ReadMem;
PakHeartBeatModuleFunction       HSync;
PakWriteMemoryByteModuleFunction CpuWrite;
PakTerminateModuleFunction       Terminate;
PakSampleAudioModuleFunction     Sample;
PakGetStatusModuleFunction       Status;

int  gFailures = 0;
bool gCartLine = false;
int  gCartLineCalls = 0;

constexpr double kLineHz = 14318180.0 / 912.0;   // matches psg_cart.h
constexpr double kRate = 44100.0;   // VCC pulls PakSampleAudio at this rate

void Check(bool ok, const char* name, const std::string& detail = "")
{
    std::printf("%s  %s%s%s\n", ok ? "PASS" : "FAIL", name,
                detail.empty() ? "" : "  -- ", detail.c_str());
    if (!ok) gFailures++;
}

void Ym(uint8_t reg, uint8_t val)
{
    WritePort(0x5E, reg);
    WritePort(0x5F, val);
}

uint8_t YmRead(uint8_t reg)
{
    WritePort(0x5E, reg);
    return ReadPort(0x5F);
}

// Like VCC: one HSYNC per scanline, PakSampleAudio at 44.1kHz in between.
std::vector<uint8_t> Run(double seconds)
{
    static double acc = 0.0;
    std::vector<uint8_t> out;
    const int lines = int(seconds * kLineHz);
    out.reserve(size_t(seconds * kRate) + 4);
    for (int i = 0; i < lines; i++) {
        HSync();
        for (acc += kRate / kLineHz; acc >= 1.0; acc -= 1.0)
            out.push_back(uint8_t(Sample() >> 8));
    }
    return out;
}

// Loudest component that is not an odd harmonic of f0, in dB relative to
// the fundamental (Hann-windowed Goertzel, 50Hz-20kHz in 5Hz steps).
double WorstInharmonicDb(const std::vector<uint8_t>& s, double f0)
{
    const size_t n = s.size();
    std::vector<double> x(n);
    for (size_t i = 0; i < n; i++)
        x[i] = (double(s[i]) - 128.0) * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * i / (n - 1)));
    auto mag = [&](double f) {
        const double w = 2.0 * 3.14159265358979 * f / kRate, c = 2.0 * std::cos(w);
        double s1 = 0, s2 = 0;
        for (size_t i = 0; i < n; i++) { const double s0 = x[i] + c * s1 - s2; s2 = s1; s1 = s0; }
        return std::sqrt(s1 * s1 + s2 * s2 - c * s1 * s2);
    };
    const double fund = mag(f0);
    double worst = 0.0;
    for (double f = 50.0; f < 20000.0; f += 5.0) {
        const double k = f / f0;
        const double nearest = std::floor(k + 0.5);
        if (std::fabs(k - nearest) * f0 < 20.0 && (int(nearest) % 2 == 1)) continue;
        worst = (std::max)(worst, mag(f));
    }
    return 20.0 * std::log10(worst / fund);
}

// Frequency from upward crossings of the midpoint, with hysteresis so
// dither and slow slopes don't count as extra crossings.
double MeasureHz(const std::vector<uint8_t>& s)
{
    int crossings = 0, first = -1, last = -1;
    bool low = false;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] < 0x80 - 6) low = true;
        else if (low && s[i] >= 0x80 + 6) {
            low = false;
            if (first < 0) first = int(i);
            last = int(i);
            crossings++;
        }
    }
    if (crossings < 2) return 0.0;
    return (crossings - 1) * kRate / double(last - first);
}

int PeakToPeak(const std::vector<uint8_t>& s, size_t skip = 0)
{
    int lo = 255, hi = 0;
    for (size_t i = skip; i < s.size(); i++) {
        lo = std::min<int>(lo, s[i]);
        hi = std::max<int>(hi, s[i]);
    }
    return hi - lo;
}

void SilenceAll()
{
    Ym(7, 0x3F);
    Ym(8, 0); Ym(9, 0); Ym(10, 0);
}

void WriteWav(const std::string& path, const std::vector<uint8_t>& s)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    const uint32_t rate = uint32_t(kRate), n = uint32_t(s.size());
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + n); std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(1); u32(rate); u32(rate); u16(1); u16(8);
    std::fwrite("data", 1, 4, f); u32(n);
    std::fwrite(s.data(), 1, n, f);
    std::fclose(f);
}

void AssertCartLine(slot_id_type, bool state) { gCartLine = state; gCartLineCalls++; }
void AssertInterrupt(slot_id_type, Interrupt, InterruptSource) {}
void WriteMemHost(slot_id_type, unsigned char, unsigned short) {}
unsigned char ReadMemHost(slot_id_type, unsigned short) { return 0; }

template <typename T> bool Get(HMODULE m, T& fn, const char* name)
{
    fn = reinterpret_cast<T>(GetProcAddress(m, name));
    if (!fn) std::printf("missing export %s\n", name);
    return fn != nullptr;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) { std::printf("usage: psgharness <psg.dll> <out-dir>\n"); return 2; }
    const std::string out = std::string(argv[2]) + "\\";

    // Test FLASH image: every byte of segment N holds N.
    const std::string flash = out + "testflash.rom";
    {
        std::vector<uint8_t> img(0x80000);
        for (size_t i = 0; i < img.size(); i++) img[i] = uint8_t(i / 0x2000);
        FILE* f = std::fopen(flash.c_str(), "wb");
        std::fwrite(img.data(), 1, img.size(), f);
        std::fclose(f);
    }
    DeleteFileA((flash + ".bak").c_str());
    const std::string ini = out + "psgtest.ini";
    DeleteFileA(ini.c_str());
    WritePrivateProfileStringA("CoCo-PSG", "FlashImage", flash.c_str(), ini.c_str());

    HMODULE m = LoadLibraryA(argv[1]);
    if (!m) { std::printf("cannot load %s (%lu)\n", argv[1], GetLastError()); return 2; }
    bool ok = Get(m, Init, "PakInitialize") & Get(m, Reset, "PakReset") & Get(m, PowerOn, "PakPowerOnReset")
            & Get(m, WritePort, "PakWritePort") & Get(m, ReadPort, "PakReadPort")
            & Get(m, ReadMem, "PakReadMemoryByte") & Get(m, HSync, "PakProcessHorizontalSync")
            & Get(m, Sample, "PakSampleAudio") & Get(m, CpuWrite, "PakWriteMemoryByte") & Get(m, Terminate, "PakTerminate") & Get(m, Status, "PakGetStatus");
    if (!ok) return 2;

    static const cpak_callbacks cb { AssertInterrupt, AssertCartLine, WriteMemHost, ReadMemHost };
    Init(0, ini.c_str(), nullptr, &cb);
    PowerOn();

    char status[256];
    Status(status, sizeof status);
    std::printf("status: %s\n\n", status);

    // T1 autostart: image loaded + control bit 4 clear -> CART asserted.
    Check(gCartLine, "T1 CART asserted after reset with flash image");
    WritePort(0x5D, 0x10);
    Check(!gCartLine, "T1 CART released when control bit 4 set");
    WritePort(0x5D, 0x00);
    Check(gCartLine, "T1 CART re-asserted when bit 4 cleared");

    // T2 banking.
    WritePort(0x5A, 5); WritePort(0x5B, 63);
    Check(ReadMem(0x0000) == 5 && ReadMem(0x1FFF) == 5, "T2 bank 0 -> FLASH segment 5");
    Check(ReadMem(0x2000) == 63 && ReadMem(0x3EFF) == 63, "T2 bank 1 -> FLASH segment 63");
    Check(ReadMem(0x4000) == 5 && ReadMem(0x6000) == 63, "T2 32K-mode offsets mirror on A13");
    WritePort(0x5A, 0x80);
    Check(ReadMem(0x0000) == 0x00, "T2 bank 0 -> SRAM segment 128 (cleared)");
    Check(ReadPort(0x5A) == 0x80 && ReadPort(0x5B) == 63, "T2 bank registers read back");
    Check(ReadPort(0x40) == 0 && ReadPort(0x7F) == 0, "T2 foreign ports read 0");
    WritePort(0x5A, 0);

    // T3 YM registers.
    Ym(1, 0xFF);
    Check(YmRead(1) == 0xFF, "T3 R1 reads back all 8 bits (as real cart)");
    Ym(7, 0x3F);
    Check(YmRead(14) == 0xFF && YmRead(15) == 0xFF, "T3 I/O ports as inputs read idle $FF");
    Ym(7, 0xFF); Ym(14, 0x5A);
    Check(YmRead(14) == 0x5A, "T3 I/O port A as output reads latch");
    Ym(7, 0x3F);

    // T11 measured on real hardware: $FF5C mirrors control, $FF5E reads $FF.
    WritePort(0x5D, 0x11);
    Check(ReadPort(0x5C) == 0x11 && ReadPort(0x5D) == 0x11, "T11 $FF5C reads control register");
    Check(ReadPort(0x5E) == 0xFF, "T11 $FF5E reads $FF");
    WritePort(0x5D, 0x00);

    // T12 reset button keeps bank/control (firmware menu relies on it);
    // power-on clears them.
    WritePort(0x5A, 4); WritePort(0x5B, 1); WritePort(0x5D, 0x10);
    Ym(8, 15);
    Reset();
    Check(ReadPort(0x5A) == 4 && ReadPort(0x5B) == 1 && ReadPort(0x5D) == 0x10 && !gCartLine,
          "T12 reset button keeps banks/control, autostart stays off");
    Check(YmRead(8) == 0, "T12 reset button resets the YM2149");
    PowerOn();
    Check(ReadPort(0x5A) == 0 && ReadPort(0x5B) == 0 && ReadPort(0x5D) == 0 && gCartLine,
          "T12 power-on clears registers and re-arms autostart");

    // T13 SRAM writes need control bit 3.
    PowerOn();
    WritePort(0x5A, 0x80); WritePort(0x5B, 0x81);
    WritePort(0x5D, 0x10);
    CpuWrite(0xC000, 0x55);
    Check(ReadMem(0x0000) == 0x00, "T13 SRAM write ignored with bit 3 clear");
    WritePort(0x5D, 0x18);
    CpuWrite(0xC000, 0x55); CpuWrite(0xE123, 0x66); CpuWrite(0xFEFF, 0x77);
    WritePort(0x5D, 0x10);
    Check(ReadMem(0x0000) == 0x55 && ReadMem(0x2123) == 0x66 && ReadMem(0x3EFF) == 0x77,
          "T13 SRAM writes land in bank 0 and bank 1 segments");
    WritePort(0x5B, 0x80);
    Check(ReadMem(0x2000) == 0x55, "T13 same SRAM segment visible in both banks");

    // T14 replica of the firmware's bank test (seg 1 $211D-$2180).
    {
        WritePort(0x5D, 0x18);
        for (int seg = 0x80; seg < 0xC0; seg++) {
            WritePort(0x5A, uint8_t(seg));
            for (int a = 0xC000; a < 0xE000; a++) CpuWrite(uint16_t(a), uint8_t(seg));
        }
        WritePort(0x5D, 0x10);
        bool ok1 = true, ok2 = true;
        for (int seg = 0x80; seg < 0xC0; seg++) {
            WritePort(0x5A, uint8_t(seg));
            if (ReadMem(0) != seg) ok1 = false;
            for (int a = 0; a < 0x2000; a++) if (ReadMem(uint16_t(a)) != seg) { ok2 = false; break; }
        }
        Check(ok1, "T14 firmware BANK test: byte 0 of SRAM 128-191");
        Check(ok2, "T14 firmware RAM test: every byte of SRAM 128-191");
    }

    // T15 FLASH software ID through $BFFF/$BAAA (bits 3+5).
    WritePort(0x5A, 0); WritePort(0x5D, 0x28);
    CpuWrite(0xBFFF, 0xAA); CpuWrite(0xBAAA, 0x55); CpuWrite(0xBFFF, 0x90);
    Check(ReadMem(0) == 0xBF && ReadMem(1) == 0xB7, "T15 FLASH ID reads $BF/$B7");
    CpuWrite(0xC000, 0xF0);
    Check(ReadMem(0) == 0x00, "T15 F0 exits ID mode");
    WritePort(0x5D, 0x08);
    CpuWrite(0xBFFF, 0xAA); CpuWrite(0xBAAA, 0x55); CpuWrite(0xBFFF, 0x90);
    Check(ReadMem(1) == 0x00, "T15 $BFFF/$BAAA ignored without bit 5");

    // T16 sector erase + byte program in segment 10 (bytes = $0A).
    WritePort(0x5A, 10); WritePort(0x5D, 0x28);
    auto cmd = [](uint8_t c) { CpuWrite(0xBFFF, 0xAA); CpuWrite(0xBAAA, 0x55); CpuWrite(0xBFFF, c); };
    cmd(0x80); CpuWrite(0xBFFF, 0xAA); CpuWrite(0xBAAA, 0x55); CpuWrite(0xC000, 0x30);
    Check(ReadMem(0) == 0xFF && ReadMem(0x0FFF) == 0xFF && ReadMem(0x1000) == 0x0A,
          "T16 sector erase clears one 4K sector");
    cmd(0xA0); CpuWrite(0xC000, 0x42);
    cmd(0xA0); CpuWrite(0xC001, 0x0F);
    Check(ReadMem(0) == 0x42 && ReadMem(1) == 0x0F, "T16 byte program");
    cmd(0xA0); CpuWrite(0xC000, 0xFF);
    Check(ReadMem(0) == 0x42, "T16 programming can't set bits back to 1");
    CpuWrite(0xC002, 0x00);
    Check(ReadMem(2) == 0xFF, "T16 plain write without command is ignored");
    WritePort(0x5D, 0x10);
    cmd(0xA0); CpuWrite(0xC003, 0x00);
    Check(ReadMem(3) == 0xFF, "T16 FLASH ignores writes with bit 3 clear");

    // T17 changes are written back to the image after 0.5s, original kept.
    Run(0.6);
    {
        auto readall = [](const std::string& path) {
            std::vector<uint8_t> v(0x80000);
            FILE* f = std::fopen(path.c_str(), "rb");
            size_t n = f ? std::fread(v.data(), 1, v.size(), f) : 0;
            if (f) std::fclose(f);
            v.resize(n);
            return v;
        };
        auto img = readall(flash), bak = readall(flash + ".bak");
        Check(img.size() == 0x80000 && img[10 * 0x2000] == 0x42 && img[10 * 0x2000 + 1] == 0x0F,
              "T17 programmed FLASH saved to image file");
        Check(bak.size() == 0x80000 && bak[10 * 0x2000] == 0x0A, "T17 original kept as .bak");
    }

    // T18 chip erase.
    WritePort(0x5D, 0x28);
    cmd(0x80); CpuWrite(0xBFFF, 0xAA); CpuWrite(0xBAAA, 0x55); CpuWrite(0xBFFF, 0x10);
    bool erased = true;
    for (int seg = 0; seg < 64 && erased; seg += 9) {
        WritePort(0x5A, uint8_t(seg));
        if (ReadMem(0) != 0xFF || ReadMem(0x1FFF) != 0xFF) erased = false;
    }
    Check(erased, "T18 chip erase");
    WritePort(0x5D, 0x10);
    Terminate();  // flushes the erased image

    // T4 silence.
    PowerOn();
    SilenceAll();
    auto s = Run(0.2);
    Check(PeakToPeak(s) == 0 && s.back() == 0x80, "T4 silent chip outputs steady $80");

    // T5 tone A 440Hz at 2MHz: TP = 2e6 / (16*440) = 284.
    Ym(0, 284 & 0xFF); Ym(1, 284 >> 8); Ym(8, 15); Ym(7, 0x3E);
    s = Run(1.0);
    double hz = MeasureHz(s);
    Check(std::fabs(hz - 440.1) < 2.0, "T5 tone A 440Hz", std::to_string(hz) + "Hz");
    const int pp = PeakToPeak(s, 20000);   // let the DC blocker settle
    Check(pp > 55 && pp < 90, "T5 full-volume amplitude", "p-p " + std::to_string(pp));

    // T6 clock divider bit 0 halves the pitch.
    WritePort(0x5D, 0x01);
    s = Run(1.0);
    hz = MeasureHz(s);
    Check(std::fabs(hz - 220.0) < 2.0, "T6 control bit 0 -> 1MHz halves pitch", std::to_string(hz) + "Hz");
    WritePort(0x5D, 0x00);

    // T7 volume steps are monotonic.
    // (0.5s per level so the DC blocker settles after each step.)
    int prev = -1; bool mono = true; std::string levels;
    for (int v = 1; v <= 15; v++) {
        Ym(8, uint8_t(v));
        s = Run(0.5);
        const int p = PeakToPeak(s, 16000);
        if (p < prev) mono = false;
        prev = p;
        levels += std::to_string(p) + " ";
    }
    Check(mono, "T7 amplitude rises with volume 1..15", levels);

    // T8 noise on C only.
    SilenceAll();
    Ym(6, 8); Ym(10, 15); Ym(7, 0x1F); // noise C enabled (bit5 = 0)
    s = Run(0.5);
    Check(PeakToPeak(s, 500) > 40 && MeasureHz(s) > 100.0, "T8 noise produces broadband output");

    // T9 envelope: shape $0D (attack, hold at max) ramps up and stays.
    SilenceAll();
    Ym(11, 0x00); Ym(12, 0x08);  // EP = $0800: ramp ~ 2e6/8/2048*... ~0.26s
    Ym(0, 0); Ym(1, 0); Ym(7, 0x3F); // tone/noise off -> constant level
    Ym(8, 0x10); Ym(13, 0x0D);
    auto ramp = Run(0.5);
    // DC blocker makes a step-up look like a positive bump; check rise.
    int early = 0, late = 0;
    for (int i = 0; i < 560; i++) early += ramp[i];
    for (int i = 10600; i < 11160; i++) late += ramp[i];
    Check(late > early, "T9 envelope attack ramps up", std::to_string(early / 560) + " -> " + std::to_string(late / 560));

    // T10 sawtooth envelope $08 on a constant channel repeats.
    Ym(11, 0x40); Ym(12, 0x00); // EP=64: 32 steps * 64 / 250kHz = 8.2ms -> 122Hz
    Ym(13, 0x08);
    s = Run(1.0);
    hz = MeasureHz(s);
    Check(std::fabs(hz - 122.07) < 3.0, "T10 sawtooth envelope period", std::to_string(hz) + "Hz");

    // T19 no aliasing: a high square wave has no inharmonic components.
    {
        PowerOn(); SilenceAll();
        const int tp = 47;                       // 2659.6Hz (E7)
        Ym(0, tp); Ym(1, 0); Ym(8, 15); Ym(7, 0x3E);
        Run(0.2);
        auto t = Run(1.0);
        const double db = WorstInharmonicDb(t, 2000000.0 / (16.0 * tp));
        char buf[64]; std::snprintf(buf, sizeof buf, "%.1f dB (scanline-rate version: -13.6 dB)", db);
        Check(db < -40.0, "T19 E7 square wave: worst inharmonic component", buf);
        SilenceAll();
    }

    // T21 cost of the audio path (informational limit).
    {
        Ym(0, 100); Ym(1, 0); Ym(8, 15); Ym(2, 200); Ym(9, 12); Ym(6, 5); Ym(10, 10); Ym(7, 0x30);
        const DWORD t0 = GetTickCount();
        Run(5.0);
        const DWORD ms = GetTickCount() - t0;
        char buf[64]; std::snprintf(buf, sizeof buf, "%.1f ms CPU per emulated second", ms / 5.0);
        Check(ms / 5.0 < 250.0, "T21 audio path cost", buf);
        SilenceAll();
    }

    // T20 host rate measured from the sample calls.
    {
        char st[256]; Status(st, sizeof st);
        Check(std::strstr(st, "| 44.1kHz") != nullptr, "T20 host sample rate measured", st);
    }

    // Demo: C major arpeggio on A, bass on B, hi-hat noise on C.
    PowerOn();
    SilenceAll();
    std::vector<uint8_t> demo;
    const double notes[] = { 261.63, 329.63, 392.00, 523.25, 392.00, 329.63 };
    Ym(6, 2);
    for (int bar = 0; bar < 4; bar++) {
        for (double f : notes) {
            const int tp = int(2000000.0 / (16.0 * f) + 0.5);
            const int bp = int(2000000.0 / (16.0 * 65.41) + 0.5);
            Ym(0, tp & 0xFF); Ym(1, tp >> 8);
            Ym(2, bp & 0xFF); Ym(3, bp >> 8);
            Ym(8, 13); Ym(9, 11);
            Ym(10, 0x10); Ym(11, 0x00); Ym(12, 0x04); Ym(13, 0x00); // decaying hat
            Ym(7, 0x1C); // tone A, tone B, noise C
            auto part = Run(0.18);
            demo.insert(demo.end(), part.begin(), part.end());
        }
    }
    SilenceAll();
    auto tail = Run(0.3);
    demo.insert(demo.end(), tail.begin(), tail.end());
    WriteWav(out + "psg_demo.wav", demo);
    std::printf("\nwrote %spsg_demo.wav\n", out.c_str());

    std::printf("\n%s (%d failure%s)\n", gFailures ? "FAILED" : "ALL PASSED", gFailures, gFailures == 1 ? "" : "s");
    return gFailures ? 1 : 0;
}
