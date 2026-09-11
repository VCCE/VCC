// ssc_dll.cpp
//
// VCC "cpak" DLL cartridge entry points for the Tandy Speech/Sound
// Cartridge emulation. This file is the thin, Windows/VCC-specific
// adapter; all of the actual chip emulation lives in ssc_core.* /
// tms7000.* / sp0256.* / ay8913.*, which are plain portable C++ with no
// VCC or Windows dependency and can be (and are, see test/) unit tested
// standalone.
//
// Build layout: this file is designed to live in a folder (e.g. "SSC/")
// dropped in as a sibling of VCC's own orch90/, FD502/, GMC/ folders
// inside a checkout of https://github.com/VCCE/VCC -- see README.md.
// That gives it the same libcommon include path and project-reference
// setup every other pak in the tree already uses.
//
// ROM files: the SSC's two internal ROMs (the PIC7040 firmware and the
// SP0256-AL2 speech mask ROM) are NOT included with this source -- they
// are copyrighted dumps of the real cartridge's chips, same as how VCC
// itself expects the user to supply their own CoCo BASIC ROM, and how
// orch90.dll expects ORCH90.ROM to already exist. At startup this DLL
// looks for two files next to SSC.DLL itself:
//     pic-7040-510.bin   (4096 bytes)
//     sp0256-al2.bin      (2048 bytes)
// using the same filenames MAME's coco_ssc.cpp ROM_LOAD lines use, so a
// ROM set already collected for MAME can be copied over unmodified. See
// README.md for where these are documented/archived.

#include <windows.h>
#include <cstdio>
#include <cstring>

#include <vcc/bus/cpak_cartridge_definitions.h>

#include "ssc_core.h"

namespace {

HINSTANCE                          gModuleInstance = nullptr;
slot_id_type                       gSlotId {};
PakAssertCartridgeLineHostCallback gAssertCartLine = nullptr;
ssc::SscCore                       gSsc;
bool                                gRomsOk = false;
char                                gRomDir[MAX_PATH] = "";

// Directory containing this DLL (not the host EXE's CWD -- see the
// header comment above for why that matters).
void ComputeRomDir()
{
    char path[MAX_PATH] = "";
    GetModuleFileNameA(gModuleInstance, path, MAX_PATH);

    char* slash = strrchr(path, '\\');
    if (!slash) slash = strrchr(path, '/');
    if (slash) {
        *(slash + 1) = '\0';
        std::snprintf(gRomDir, sizeof(gRomDir), "%s", path);
    } else {
        gRomDir[0] = '\0';
    }
}

bool ReadWholeFile(const char* filename, uint8_t* buf, size_t expected_len)
{
    char full[MAX_PATH * 2];
    std::snprintf(full, sizeof(full), "%s%s", gRomDir, filename);

    FILE* f = std::fopen(full, "rb");
    if (!f) return false;

    size_t got = std::fread(buf, 1, expected_len, f);
    std::fclose(f);
    return got == expected_len;
}

bool LoadRoms()
{
    static uint8_t pic_rom[4096];
    static uint8_t spo_rom[0x800];

    bool pic_ok = ReadWholeFile("pic-7040-510.bin", pic_rom, sizeof(pic_rom));
    bool spo_ok = ReadWholeFile("sp0256-al2.bin", spo_rom, sizeof(spo_rom));

    if (pic_ok) gSsc.LoadPicRom(pic_rom, sizeof(pic_rom));
    if (spo_ok) gSsc.LoadSpeechRom(spo_rom, sizeof(spo_rom));

    return pic_ok && spo_ok;
}

} // namespace

extern "C"
{
    __declspec(dllexport) const char* PakGetName()
    {
        return "Tandy Speech/Sound Cartridge";
    }

    __declspec(dllexport) const char* PakGetCatalogId()
    {
        return "26-3144";
    }

    __declspec(dllexport) const char* PakGetDescription()
    {
        return gRomsOk
            ? "Tandy Speech/Sound Cartridge (SSC): PIC7040 + AY-3-8913 + SP0256-AL2 emulation"
            : "Tandy Speech/Sound Cartridge (SSC): ROMs not found -- see README.md";
    }

    __declspec(dllexport) void PakInitialize(
        slot_id_type SlotId,
        const char* const /*configuration_path*/,
        HWND /*hVccWnd*/,
        const cpak_callbacks* const callbacks)
    {
        gSlotId = SlotId;
        gAssertCartLine = callbacks->assert_cartridge_line;

        // This "cpak" ABI generation ticks us from PakProcessHorizontalSync
        // (a real per-scanline callback) rather than from the audio-sample
        // export itself -- a genuinely different cadence than the classic
        // ModuleAudioSample-driven ABI in ssc_dll_legacy.cpp, which SscCore
        // defaults to (44100Hz). Per an embedded comment in VCCE/VCC's own
        // current orch90.cpp ("called every scan line 262 Lines * 60
        // Frames = 15780 Hz 15720"), PakProcessHorizontalSync really does
        // fire at the classic ~15720Hz CoCo scanline rate, so tell SscCore
        // to interpret each Tick() call accordingly.
        gSsc.SetTickRateHz(15720.0);

        ComputeRomDir();
        gRomsOk = LoadRoms();
    }

    __declspec(dllexport) void PakReset()
    {
        gSsc.Reset();
        if (gAssertCartLine) gAssertCartLine(gSlotId, gRomsOk ? 1 : 0);
    }

    __declspec(dllexport) void PakWritePort(unsigned char Port, unsigned char Data)
    {
        gSsc.WritePort(Port, Data);
    }

    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
    {
        return gSsc.ReadPort(Port);
    }

    __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short /*Address*/)
    {
        // The real S/SC has no cartridge ROM of its own (its only ROMs
        // are the PIC7040's internal program memory and the SP0256's
        // mask ROM, neither of which is CPU-addressable from the CoCo
        // side) -- see the service manual excerpt in README.md.
        return 0xff;
    }

    __declspec(dllexport) void PakProcessHorizontalSync()
    {
        // Called once per scanline (~15720Hz on NTSC), matching the
        // same cadence orch90.cpp documents for PakSampleAudio(). This
        // is where we advance the PIC7040/AY/SP0256 emulation and latch
        // the next output sample.
        gSsc.Tick();
    }

    __declspec(dllexport) unsigned short PakSampleAudio()
    {
        unsigned char s = gSsc.LatchedSample();
        return (static_cast<unsigned short>(s) << 8) | s; // mono -> both channels
    }

    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        if (!gRomsOk) {
            std::snprintf(text_buffer, buffer_size, "S/SC: ROMs MISSING (see README.md)");
            return;
        }
        // Live diagnostics -- reopen VCC's cartridge menu (or whatever
        // shows this status string) while a program is driving the SSC
        // to see whether the firmware is actually receiving I/O:
        //   pc      = TMS7040 program counter (should move around within
        //             $F000-$FFFF; if it's frozen at one address for a
        //             long time, or leaves that range, something's wrong)
        //   ay/ald  = cumulative counts of AY register writes and SP0256
        //             allophone-load strobes since reset (both should
        //             keep climbing while a test program is actively
        //             using sound/speech; if they're stuck at whatever
        //             they were after the boot-time AY init, the host
        //             program's writes to $FF7D/$FF7E aren't reaching
        //             the firmware, or the firmware isn't acting on them)
        //   illeg   = illegal-opcode fetch count (should always be 0;
        //             nonzero means the CPU core has derailed)
        //   busy    = current $FF7E busy handshake state
        std::snprintf(text_buffer, buffer_size,
                       "S/SC: PC=$%04X cmd=%u spk=%u ALD=%u illeg=%u busy=%s",
                       gSsc.CpuPc(), gSsc.CommandCount(), gSsc.AldSpeechCount(), gSsc.AldCount(),
                       gSsc.CpuIllegalCount(), gSsc.Busy() ? "1" : "0");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        gModuleInstance = hinstDLL;
    return TRUE;
}
