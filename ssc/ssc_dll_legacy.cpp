// ssc_dll_legacy.cpp
//
// VCC cartridge DLL adapter targeting the CLASSIC (pre-refactor) VCC pak
// ABI used by VCC releases up through at least 2.1.9.1 (Dec 2024) --
// this is a DIFFERENT, older interface than the "Pak*"-named cpak ABI
// in ssc_dll.cpp (which targets the newer VCCE/VCC GitHub main-branch
// libcommon refactor). Use THIS file if your VCC's own orch90.dll
// exports ModuleName/PackPortRead/PackPortWrite/ModuleReset/SetCart/
// PakMemRead8/ModuleAudioSample (check with a PE export viewer, or
// just try this build if ssc_dll.cpp's build gives you a "Not a valid
// Module" error or silently won't show as inserted in an MPI slot).
//
// Confirmed against VCCE/VCC tag vcc-2.1.9.1 (pakinterface.c / mpi.cpp
// at that tag): the loader requires the DLL to export "ModuleName" --
// GetProcAddress(handle, "ModuleName") == NULL is the exact condition
// that produces the literal MessageBox "Not a valid Module". Every
// other export below is optional (bound if present, harmless if not),
// but all are implemented here for a fully-featured cartridge:
//
//   ModuleName(ModName, CatNumber, MenuCallback)  -- REQUIRED. Called
//       once at insert time to populate the cartridge menu label.
//   PackPortWrite(Port, Data) / PackPortRead(Port) -- host I/O ports,
//       broadcast to every MPI slot on every access (not SCS-gated in
//       this VCC generation), matching $FF7D/$FF7E exactly like the
//       new-ABI build.
//   ModuleReset(void) -- called once right after insert (and again on
//       any VCC reset). This is where ROM loading + chip reset happens
//       (there's no separate "Initialize" call in this ABI generation).
//   SetCart(HostCallback) -- called once by the host right after insert
//       to hand the DLL a callback for asserting the cartridge line;
//       we store it and invoke it from ModuleReset() once ROMs load.
//   PakMemRead8(Address) -- CPU-side ROM byte read; always 0xFF, same
//       reasoning as the new-ABI build (S/SC has no CoCo-addressable
//       ROM of its own).
//   ModuleAudioSample(void) -- called once per AUDIO SAMPLE, at a real,
//       wall-clock-accurate 44100Hz (confirmed directly against VCC's
//       own source for this exact tag: audio.cpp's AudioOut()/
//       GetDACSample() pulls PackAudioSample() -> this export every
//       SoundInterupt = NANOSECOND/44100 nanoseconds, with no batching
//       or resampling in between -- NOT the ~15720Hz video scanline
//       rate an earlier version of this file assumed). This is ALSO
//       where we advance the emulated PIC7040/AY/SP0256 by one tick
//       (there's no separate horizontal-sync callback in this ABI --
//       real orch90.dll from this era does the same thing: all of its
//       periodic work happens inside this one function).
//   ModuleStatus(StatusLine) -- fills a 256-byte buffer (SystemState::
//       StatusLine in this VCC generation) with the same live PC/AY/ALD/
//       busy diagnostics as the new-ABI build's PakGetStatus.
//
// Everything else (ROM loading, chip cores, SscCore glue) is shared
// unchanged with ssc_dll.cpp via ssc_core.h -- this file differs from
// it only in which Windows-facing export names/signatures it presents.

#include <windows.h>
#include <cstdio>
#include <cstring>

#include "ssc_core.h"

namespace {

using DynamicMenuCallback = void (*)(char*, int, int);
using SetCartCallback     = void (*)(unsigned char);

HINSTANCE       gModuleInstance = nullptr;
SetCartCallback gHostSetCart = nullptr;
ssc::SscCore    gSsc;
bool            gRomsOk = false;
char            gRomDir[MAX_PATH] = "";

// Directory containing this DLL (not the host EXE's CWD).
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
    __declspec(dllexport) void ModuleName(char* ModName, char* CatNumber, DynamicMenuCallback /*Temp*/)
    {
        // MAX_LOADSTRING in this VCC generation is 100 -- both strings
        // below are well under that.
        std::strcpy(ModName, "Tandy Speech/Sound Cartridge");
        std::strcpy(CatNumber, "26-3144");
    }

    __declspec(dllexport) void PackPortWrite(unsigned char Port, unsigned char Data)
    {
        gSsc.WritePort(Port, Data);
    }

    __declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
    {
        return gSsc.ReadPort(Port);
    }

    __declspec(dllexport) void ModuleReset(void)
    {
        ComputeRomDir();
        gRomsOk = LoadRoms();
        gSsc.Reset();
        if (gHostSetCart) gHostSetCart(gRomsOk ? 1 : 0);
    }

    __declspec(dllexport) void SetCart(SetCartCallback Pointer)
    {
        gHostSetCart = Pointer;
    }

    __declspec(dllexport) unsigned char PakMemRead8(unsigned short /*Address*/)
    {
        return 0xff;
    }

    __declspec(dllexport) unsigned short ModuleAudioSample(void)
    {
        // No separate heartbeat export in this ABI generation -- advance
        // the emulated chips once per call, same as real orch90.dll does
        // in its own ModuleAudioSample for this same VCC generation.
        gSsc.Tick();
        unsigned char s = gSsc.LatchedSample();
        return (static_cast<unsigned short>(s) << 8) | s; // mono -> both channels
    }

    __declspec(dllexport) void ModuleStatus(char* StatusLine)
    {
        // StatusLine points at SystemState::StatusLine, a 256-byte
        // buffer in this VCC generation.
        if (!gRomsOk) {
            std::snprintf(StatusLine, 256, "S/SC: ROMs MISSING (see README.md)");
            return;
        }
        std::snprintf(StatusLine, 256,
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
