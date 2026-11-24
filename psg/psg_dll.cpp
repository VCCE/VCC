// psg_dll.cpp
//
// VCC cartridge entry points for the CoCo PSG. The emulation itself is in
// psg_cart.* and ym2149.*; this file only adapts it to VCC's plugin ABI.
//
// The FLASH image is chosen from the cartridge menu and remembered in the
// VCC ini file ([CoCo-PSG] FlashImage=...). With no image loaded the FLASH
// reads as erased ($FF) and the cart does not autostart.
//
// FLASH is non-volatile, so programming and erasing done by CoCo software
// is written back to the image file, half a second after the last change
// (and on unload). Before the first write-back a copy of the original is
// kept as <image>.bak, unless one already exists. SRAM is not saved.
//
// Gameports: each port's input (none / numeric keypad / a gamepad) and pad
// type (Atari / Genesis 3- or 6-button) are set from the cart menu, stored
// as [CoCo-PSG] PortA/PortB and PadA/PadB, and polled once per frame.

#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>

#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/util/DialogOps.h>
#include <vcc/util/settings.h>

#include "psg_cart.h"
#include "psg_input.h"

namespace {

constexpr const char* kIniSection = "CoCo-PSG";
constexpr const char* kIniFlashKey = "FlashImage";

// IDs 1-4 are VCC's own boot-slot items (Load DLL, Eject, Load MPI, Load
// ROM), so cart items must start above them; other carts use 10 and up.
enum MenuItems { SelectFlash = 10, EjectFlash = 11,
                 PortASource = 12, PortAType = 13, PortBSource = 14, PortBType = 15 };

HINSTANCE                          gModuleInstance = nullptr;
slot_id_type                       gSlotId {};
PakAssertCartridgeLineHostCallback gAssertCartLine = nullptr;
std::string                        gIniPath;
std::string                        gFlashPath;
bool                               gCartLine = false;
uint32_t                           gFlashSaved = 0;   // change count last saved
uint32_t                           gFlashSeen = 0;    // change count last seen
int                                gFlashIdleLines = 0;
bool                               gFlashSaveFailed = false;
constexpr int                      kSaveDelayLines = 15700 / 2;
psg::PsgCart                       gCart;
HWND                               gVccWnd = nullptr;
psg::InputSource                   gPortSource[2];
psg::PadType                       gPortType[2] = { psg::PadType::Atari, psg::PadType::Atari };
int                                gPollLines = 0;
constexpr const char*              kPortKey[2] = { "PortA", "PortB" };
constexpr const char*              kPadKey[2] = { "PadA", "PadB" };
VCC::Bus::cartridge_menu           gMenu;

std::string FileNameOnly(const std::string& path)
{
    const auto pos = path.find_last_of("\\/");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

void UpdateCartLine()
{
    const bool line = gCart.CartLine();
    if (line != gCartLine && gAssertCartLine)
        gAssertCartLine(gSlotId, line);
    gCartLine = line;
}

void SaveFlashImage()
{
    gFlashSaved = gFlashSeen = gCart.FlashChangeCount();
    if (gFlashPath.empty() || !gCart.HasFlash())
        return;

    const std::string backup = gFlashPath + ".bak";
    CopyFileA(gFlashPath.c_str(), backup.c_str(), TRUE); // keeps an existing .bak

    FILE* f = std::fopen(gFlashPath.c_str(), "wb");
    const auto& data = gCart.FlashData();
    gFlashSaveFailed = !f || std::fwrite(data.data(), 1, data.size(), f) != data.size();
    if (f) std::fclose(f);
}

void FlushFlashImage()
{
    if (gCart.FlashChangeCount() != gFlashSaved)
        SaveFlashImage();
}

bool LoadFlashImage(const std::string& path)
{
    gCart.ClearFlash();
    gFlashSaved = gFlashSeen = gCart.FlashChangeCount();
    gFlashSaveFailed = false;
    if (path.empty())
        return false;

    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;

    std::vector<uint8_t> data(psg::PsgCart::kChipSize);
    const size_t got = std::fread(data.data(), 1, data.size(), f);
    std::fclose(f);

    gCart.LoadFlash(data.data(), got);
    gFlashSaved = gFlashSeen = gCart.FlashChangeCount();
    return got > 0;
}

std::string SelectFlashFile()
{
    FileDialog dlg;
    dlg.setFilter("Flash Images\0*.ROM;*.BIN\0All Files\0*.*\0\0");
    dlg.setDefExt("rom");
    dlg.setTitle("Select CoCo PSG Flash image");
    return dlg.show() ? std::string(dlg.path()) : std::string();
}

void LoadPortSettings()
{
    if (gIniPath.empty())
        return;
    VCC::Util::settings ini(gIniPath);
    for (int p = 0; p < 2; p++) {
        gPortSource[p] = psg::InputSource::FromConfig(ini.read(kIniSection, kPortKey[p]));
        gPortType[p] = psg::PadTypeFromConfig(ini.read(kIniSection, kPadKey[p]));
    }
}

void SavePortSettings()
{
    if (gIniPath.empty())
        return;
    VCC::Util::settings ini(gIniPath);
    for (int p = 0; p < 2; p++) {
        ini.write(kIniSection, kPortKey[p], gPortSource[p].ToConfig());
        ini.write(kIniSection, kPadKey[p], psg::PadTypeConfig(gPortType[p]));
    }
}

void PollPorts()
{
    for (int p = 0; p < 2; p++)
        gCart.SetPad(p, gPortType[p], psg::PollInput(gPortSource[p]));
}

void SaveFlashSetting()
{
    if (!gIniPath.empty())
        VCC::Util::settings(gIniPath).write(kIniSection, kIniFlashKey, gFlashPath);
}

} // namespace

extern "C"
{
    __declspec(dllexport) const char* PakGetName()
    {
        return "CoCo PSG";
    }

    __declspec(dllexport) const char* PakGetCatalogId()
    {
        return "CoCo PSG";
    }

    __declspec(dllexport) const char* PakGetDescription()
    {
        return "CoCo PSG: YM2149 sound, 512K FLASH, 512K SRAM";
    }

    __declspec(dllexport) void PakInitialize(
        slot_id_type SlotId,
        const char* const configuration_path,
        HWND hVccWnd,
        const cpak_callbacks* const callbacks)
    {
        gSlotId = SlotId;
        gAssertCartLine = callbacks->assert_cartridge_line;
        gIniPath = configuration_path ? configuration_path : "";
        gVccWnd = hVccWnd;
        LoadPortSettings();

        if (!gIniPath.empty())
            gFlashPath = VCC::Util::settings(gIniPath).read(kIniSection, kIniFlashKey);
        LoadFlashImage(gFlashPath);
    }

    // Reset button (VCC soft reset): bank and control registers survive,
    // which the firmware menu relies on to boot HDB-DOS after RESET.
    __declspec(dllexport) void PakReset()
    {
        gCart.ResetButton();
        gCartLine = !gCart.CartLine(); // force the line to be re-sent
        UpdateCartLine();
    }

    // Power cycle (VCC hard reset; needs a VCC that calls PakPowerOnReset).
    __declspec(dllexport) void PakPowerOnReset()
    {
        gCart.PowerOn();
        gCartLine = !gCart.CartLine();
        UpdateCartLine();
    }

    __declspec(dllexport) void PakWritePort(unsigned char Port, unsigned char Data)
    {
        if (!gCart.OwnsPort(Port))
            return;
        gCart.WritePort(Port, Data);
        if (Port == 0x5C || Port == 0x5D)
            UpdateCartLine();
    }

    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
    {
        return gCart.OwnsPort(Port) ? gCart.ReadPort(Port) : 0;
    }

    __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short Address)
    {
        return gCart.ReadMemory(Address);
    }

    // CPU writes to ROM space (needs a VCC that calls PakWriteMemoryByte).
    __declspec(dllexport) void PakWriteMemoryByte(unsigned short Address, unsigned char Data)
    {
        gCart.CpuWrite(Address, Data);
    }

    __declspec(dllexport) void PakProcessHorizontalSync()
    {
        gCart.TickScanline();

        if (++gPollLines >= 262) {      // read the controllers once a frame
            gPollLines = 0;
            PollPorts();
        }

        // Write FLASH changes back once programming has gone quiet.
        const uint32_t changes = gCart.FlashChangeCount();
        if (changes != gFlashSeen) {
            gFlashSeen = changes;
            gFlashIdleLines = 0;
        } else if (changes != gFlashSaved && ++gFlashIdleLines >= kSaveDelayLines) {
            SaveFlashImage();
        }
    }

    __declspec(dllexport) void PakTerminate()
    {
        FlushFlashImage();
    }

    __declspec(dllexport) unsigned short PakSampleAudio()
    {
        const unsigned short s = gCart.NextSample();
        return static_cast<unsigned short>((s << 8) | s); // mono on both channels
    }

    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        const std::string flash = gCart.HasFlash()
            ? FileNameOnly(gFlashPath)
            : (gFlashPath.empty() ? std::string("no flash image") : "can't load " + FileNameOnly(gFlashPath));
        const char* note = gFlashSaveFailed ? " (SAVE FAILED)"
                         : gCart.FlashChangeCount() != gFlashSaved ? " (modified)" : "";
        std::snprintf(text_buffer, buffer_size, "PSG: %s%s | %sMHz B0=%02X B1=%02X CR=%02X | %.1fkHz",
                      flash.c_str(), note, (gCart.Control() & 1) ? "1" : "2",
                      gCart.Bank0(), gCart.Bank1(), gCart.Control(),
                      gCart.MeasuredSampleRate() / 1000.0);
    }

    __declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
    {
        if (!item) return false;
        if (index == 0) {
            gMenu.clear();
            gMenu.add("", 0, MIT_Seperator);
            gMenu.add("CoCo PSG", 0, MIT_Head);
            gMenu.add("Select Flash Image...", ControlId(SelectFlash), MIT_Slave);
            gMenu.add("Eject Flash Image", ControlId(EjectFlash), MIT_Slave);
            const char* names[2] = { "A", "B" };
            const int src_id[2] = { PortASource, PortBSource };
            const int type_id[2] = { PortAType, PortBType };
            for (int p = 0; p < 2; p++) {
                gMenu.add(std::string("Port ") + names[p] + " input: " + gPortSource[p].Label(),
                          ControlId(src_id[p]), MIT_Slave);
                gMenu.add(std::string("Port ") + names[p] + " pad: " + psg::PadTypeLabel(gPortType[p]),
                          ControlId(type_id[p]), MIT_Slave);
            }
        }
        return gMenu.copy_item(*item, index);
    }

    __declspec(dllexport) void PakMenuItemClicked(unsigned char menuId)
    {
        // Gameport settings: each click cycles to the next choice.
        if (menuId >= PortASource && menuId <= PortBType) {
            const int p = (menuId - PortASource) / 2;
            if ((menuId - PortASource) % 2 == 0)
                gPortSource[p] = gPortSource[p].Next();
            else
                gPortType[p] = psg::NextPadType(gPortType[p]);
            SavePortSettings();
            PollPorts();
            if (gVccWnd)
                SendMenuUpdate(gVccWnd);
            return;
        }

        if (menuId == SelectFlash) {
            const auto path = SelectFlashFile();
            if (path.empty())
                return;
            FlushFlashImage();          // save pending changes to the old image
            gFlashPath = path;
        } else if (menuId == EjectFlash) {
            FlushFlashImage();
            gFlashPath.clear();
        } else {
            return;
        }
        SaveFlashSetting();
        LoadFlashImage(gFlashPath);
        UpdateCartLine();
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        gModuleInstance = hinstDLL;
    return TRUE;
}
