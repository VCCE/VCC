// ssc_dll_universal.cpp
//
// A single SSC.DLL that exports BOTH VCC pak ABI generations at once,
// so one file works whichever loader the host VCC build actually has:
//
//   - the classic/legacy ABI (ModuleName, PackPortRead, PackPortWrite,
//     ModuleReset, SetCart, PakMemRead8, ModuleAudioSample, ModuleStatus)
//     used by VCC's own pakinterface.c through at least tag vcc-2.1.9.2,
//   - the newer "cpak" ABI (PakInitialize, PakGetName, PakGetCatalogId,
//     PakGetDescription, PakReset, PakWritePort, PakReadPort,
//     PakReadMemoryByte, PakProcessHorizontalSync, PakSampleAudio,
//     PakGetStatus) used by VCC's libcommon/cartridge_loader.cpp
//     starting at tag vcc-2.1.9.3.
//
// This works safely because of how each VCC generation actually probes
// a cartridge DLL (confirmed directly against VCC's own source for both
// generations, not guessed):
//
//   - pakinterface.c (<= 2.1.9.2) does GetProcAddress(handle,
//     "ModuleName"); if that's present it uses ONLY the legacy-named
//     exports above and never looks for anything named "Pak*".
//   - libcommon/src/bus/cartridge_loader.cpp (>= 2.1.9.3) does
//     GetProcAddress(handle, "PakInitialize"); if that's present it
//     uses ONLY the cpak-named exports above and never looks for
//     "ModuleName" et al.
//
// Neither loader is confused by the *other* ABI's exports also being
// present in the same DLL -- they only ever look up the handful of
// names they know about. The two export name sets don't collide (one
// side is "ModuleName"/"PackPort*"/etc., the other is all "Pak*" but
// with different names -- e.g. PakMemRead8 vs PakReadMemoryByte), so
// there's nothing to resolve at build time either. This file is just
// ssc_dll.cpp and ssc_dll_legacy.cpp's exports combined into one
// translation unit, sharing one SscCore instance and one DllMain
// (each of those files has its own copy of both, which is what a
// naive concatenation would collide on).
//
// Whichever entry point the host actually calls first (PakInitialize
// vs ModuleReset) is the one that ends up computing the ROM directory
// and setting SscCore's tick rate -- see the comment on SetTickRateHz
// below. Since only one loader generation is ever driving a given
// process, the other entry point is simply never called, so sharing
// state between the two ABIs is safe.

#include <windows.h>
#include <cstdio>
#include <cstring>

#include <vcc/bus/cpak_cartridge_definitions.h>

#include "ssc_core.h"

namespace {

using DynamicMenuCallback = void (*)(char*, int, int);
using SetCartCallback     = void (*)(unsigned char);

HINSTANCE                          gModuleInstance = nullptr;
ssc::SscCore                       gSsc;
bool                                gRomsOk = false;
char                                gRomDir[MAX_PATH] = "";

// cpak-ABI-specific state
slot_id_type                       gSlotId {};
PakAssertCartridgeLineHostCallback gAssertCartLine = nullptr;

// legacy-ABI-specific state
SetCartCallback gHostSetCart = nullptr;

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
    // ---------------------------------------------------------------
    // cpak ABI (VCC >= vcc-2.1.9.3) -- required export: PakInitialize
    // ---------------------------------------------------------------

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

        // PakProcessHorizontalSync fires at the real ~15720Hz CoCo
        // scanline rate in this ABI generation (confirmed against
        // VCC's own orch90.cpp) -- a different cadence than the
        // classic ABI's ModuleAudioSample (44100Hz, SscCore's
        // default, set below under "legacy ABI").
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
        return 0xff;
    }

    __declspec(dllexport) void PakProcessHorizontalSync()
    {
        gSsc.Tick();
    }

    __declspec(dllexport) unsigned short PakSampleAudio()
    {
        unsigned char s = gSsc.LatchedSample();
        return (static_cast<unsigned short>(s) << 8) | s;
    }

    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        if (!gRomsOk) {
            std::snprintf(text_buffer, buffer_size, "S/SC: ROMs MISSING (see README.md)");
            return;
        }
        std::snprintf(text_buffer, buffer_size,
                       "S/SC: PC=$%04X cmd=%u spk=%u ALD=%u illeg=%u busy=%s",
                       gSsc.CpuPc(), gSsc.CommandCount(), gSsc.AldSpeechCount(), gSsc.AldCount(),
                       gSsc.CpuIllegalCount(), gSsc.Busy() ? "1" : "0");
    }

    // ---------------------------------------------------------------
    // legacy ABI (VCC <= vcc-2.1.9.2) -- required export: ModuleName
    // ---------------------------------------------------------------

    __declspec(dllexport) void ModuleName(char* ModName, char* CatNumber, DynamicMenuCallback /*Temp*/)
    {
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
        // No separate heartbeat export in this ABI generation --
        // SscCore's default tick rate (44100Hz) is correct here and is
        // never overridden (contrast PakInitialize above, which sets
        // 15720Hz for the *other* ABI's different callback cadence).
        gSsc.Tick();
        unsigned char s = gSsc.LatchedSample();
        return (static_cast<unsigned short>(s) << 8) | s;
    }

    __declspec(dllexport) void ModuleStatus(char* StatusLine)
    {
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
