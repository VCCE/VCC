//#define USE_LOGGING
//======================================================================
// SDC simulator.  EJ Jaquay 2025
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================
//
//  SDC Floppy port
//  ---------------
//  The SDC interface shares ports with the FDC floppy emulator.
//
//  $FF40 ; SDC latch / FDC control
//  $FF42 ; flash data register
//  $FF43 ; flash control register
//  $FF48 ; command / status
//  $FF49 ; param register 1 / FDC Track
//  $FF4A ; param register 2 / FDC Sector
//  $FF4B ; param register 3 / FDC Data
//
//  The FDC emulation uses following ports;
//
//  $FF40 ; Control register (write)
//      Bit 7 halt flag 0 = disabled 1 = enabled
//      Bit 6 drive select 3 or side select
//      Bit 5 density flag 0 = single 1 = double
//      Bit 4 write precompensation 0 = no precomp 1 = precomp
//      Bit 3 drive motor enable 0 = motors off 1 = motors on
//      Bit 2 drive select 2
//      Bit 1 drive select 1
//      Bit 0 drive select 0
//
//  SDC latch code is 0x43 which is unlikey to be used for FDC
//  control because that would be multiple drives selected.
//
//  $FF48 ; Command register (write)
//      high order nibble; low order nibble type; command
//      0x0 ;   I ; Restore
//      0x1 ;   I ; Seek
//      0x2 ;   I ; Step
//      0x4 ;   I ; Step in
//      0x5 ;   I ; Step out
//      0x8 ;  II ; Read sector
//      0x9 ;  II ; Read sector mfm
//      0xA ;  II ; write sector
//      0xB ;  II ; write sector mfm
//      0xC ; III ; read address
//      0xD ; III ; force interrupt
//      0xE ; III ; read track
//      0xF ;  IV ; write track
//      low order nibble
//      type ; meanings
//        I ; b3 head loaded, b2 track verify , b1-b0 step rate
//       II ; b3 side compare enable, b2 delay, b1 side, b0 0
//      III ; b2 delay others 0
//       IV ; interrupt control b3 immediate, b2 index pulse, b1 notready, b0 ready
//
//  The becker port (drivewire) uses port $FF41 for status and port $FF42
//  for data so these must be always allowed for becker.dll to work.
//
//  NOTE: Stock SDCDOS does not support the becker ports, it expects
//  drivewire to be on the bitbanger ports. RGBDOS does, however, and will
//  still work with sdc.dll installed.
//
//  MPI control is accomplished by writing $FF7F (65407) multi-pak slot
//  selection. $FF7F bits 1-0 set the SCS slot while bits 5-4 set the
//  CTS/CART slot. Other bits are set if the MPI selector switch is used
//  but these do nothing. SCS selects disk I/O ($FF40-$FF5F) and CTS/CART
//  selects ROM ($C000-$DFFF)
//
//  To select a slot keeping CTS and SCS the same:
//    POKE 65407,0   ($00) for slot 1
//    POKE 65407,17  ($11) for slot 2
//    POKE 65407,34  ($22) for slot 3
//    POLE 65407,51  ($33) for slot 4
//  Vcc will reset if CTS/CART is changed.
//
//  SDC commands
//  ------------
//  A commmand sent to the SDC consists of a single byte followed
//  by 0 to three 3 parameter bytes.  For example Data registers 1-3
//  are used to specify the LSN and Data registers 2-3 contain 16 bit
//  data for disk I/O. Some commands require a 256 byte block of data,
//  typically a null-terminated ASCII command string. File path names
//  must confirm to MS-DOS 8.3 naming.  Forward slashes '/' delimit
//  directories.  All path names are relative, if the first char is
//  a '/' the path is relative to the windows device or directory
//  containing the contents of the SDcard.
//
//  Extended commands are in the data block, example M:<path> mounts a
//  disk. See the SDC User Guide for detailed command descriptions. It
//  is likely that some commands mentioned there are not yet supported
//  by this interface.
//
//  Flash data
//  ----------
//  There are eight 16K banks of programable flash memory. In this
//  simulator the banks are set to files where the ROMs for the
//  bank is stored. When a bank is selected the file is read into ROM.
//  The SDC-DOS RUN@n command can be used to select a bank.
//
//  Data written to the flash data port (0x42) can be read back.
//  When the flash data port (0x43) is written the three low bits
//  select the ROM from the corresponding flash bank (0-7). When
//  read the flash control port returns the currently selected bank
//  in the three low bits and the five bits from the Flash Data port.
//
//  SDC-DOS is typically in bank zero.
//======================================================================

#include <string>
#include <algorithm>
#include <filesystem>
#include <stdio.h>
#include <Shlwapi.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <ShlObj.h>
#pragma warning(pop)
#include <Windows.h>

#include <vcc/devices/cloud9.h>
#include <vcc/util/logger.h>
#include <vcc/util/DialogOps.h>
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/util/limits.h>

#include <vcc/util/fileutil.h>
#include <vcc/util/settings.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>

#include "sdc.h"

static ::VCC::Device::rtc::cloud9 cloud9_rtc;
namespace util = ::VCC::Util;

//======================================================================
// Globals
//======================================================================

static HINSTANCE gModuleInstance; // Dll handle
static int idle_ctr = 0; // Idle Status counter

static slot_id_type gSlotId {};

// Callback pointers
static PakAssertInteruptHostCallback AssertIntCallback = nullptr;

// Settings
static char IniFile[MAX_PATH] = {};

// Window handles
static HWND gVccWindow = nullptr;
static HWND hConfigureDlg = nullptr;
static HWND hSDCardBox = nullptr;
static HWND hStartBankBox = nullptr;

static int ClockEnable = 0;
static char SDC_Status[16] = {};
static char PakRom[0x4000] = {};

static std::string gSDRoot {}; // SD card root directory
static std::string gCurDir {}; // Current directory relative to root

// SDC CoCo Interface
static FileList gFileList {};
static struct FileRecord gDirPage[16] {};
static CocoDisk gCocoDisk[2] {};
static Interface IFace {};

// Flash banks
static char FlashFile[8][MAX_PATH];
static unsigned char BankWriteNum = 0;
static unsigned char BankData = 0;
static unsigned char EnableBankWrite = 0;
static unsigned char BankWriteState = 0;

static unsigned char gStartupBank = 0;
static unsigned char gLoadedBank = 0xff;  // Currently loaded bank
static unsigned char gRomDirty = 0;       // RomDirty flag


// Streaming control
static int streaming;
static unsigned char stream_cmdcode;
static unsigned int stream_lsn;

// Floppy I/O
static char FlopDrive = 0;
static char FlopData = 0;
static char FlopTrack = 0;
static char FlopSector = 0;
static char FlopStatus = 0;
static DWORD FlopWrCnt = 0;
static DWORD FlopRdCnt = 0;
static char FlopWrBuf[256];
static char FlopRdBuf[256];

static int EDBOXES[8] = {ID_TEXT0,ID_TEXT1,ID_TEXT2,ID_TEXT3,
                         ID_TEXT4,ID_TEXT5,ID_TEXT6,ID_TEXT7};
static int UPDBTNS[8] = {ID_UPDATE0,ID_UPDATE1,ID_UPDATE2,ID_UPDATE3,
                         ID_UPDATE4,ID_UPDATE5,ID_UPDATE6,ID_UPDATE7};

static std::string gRomPath {};

static VCC::Bus::cartridge_menu SdcMenu {};
bool get_menu_item(menu_item_entry* item, size_t index);

// Access the settings object
static VCC::Util::settings* gpSettings = nullptr;
VCC::Util::settings& Setting() {return *gpSettings;}

//======================================================================
//  Functions
//======================================================================

LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);
void LoadConfig();
bool SaveConfig(HWND);
void SelectCardBox();
void UpdateFlashItem(int);
void InitCardBox();
void InitFlashBoxes();
void FitEditTextPath(HWND, int, const std::string&);
void InitSDC();
void LoadRom(unsigned char);
void ParseStartup();
bool SearchFile(const std::string&);
void UnloadDisk(int);
void GetFileList(const std::string&);
void SortFileList();
std::string FixFATPath(const std::string&);
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3]);
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn);
template <size_t N> void copy_to_fixed_char(char (&dest)[N], const std::string& src);

void SDCReadSector();
void SDCStreamImage();
void SDCWriteSector();
void SDCGetDriveInfo();
void SDCUpdateSD();
void SDCSetCurDir(const char *);
bool SDCInitiateDir(const char *);
void SDCRenameFile(const char *);
void SDCKillFile(const char *);
void SDCMakeDirectory(const char *);
void SDCGetMountedImageRec();
void SDCGetSectorCount();
void SDCGetDirectoryLeaf();
void SDCMountDisk(int,const char *,int);
void SDCMountNewDisk(int,const char *,int);
bool SDCMountNext(int);
unsigned char SDCRead(unsigned char port);
void SDCOpenFound(int,int);
void SDCOpenNew(int,const char *,int);
void SDCWrite(unsigned char,unsigned char);
void SDCControl();
void SDCCommand();
void SDCBlockReceive(unsigned char);
void SDCFlashControl(unsigned char);
void SDCFloppyCommand(unsigned char);
void SDCFloppyRestore();
void SDCFloppySeek();
void SDCFloppyReadDisk();
void SDCFloppyWriteDisk();
void SDCFloppyTrack(unsigned char);
void SDCFloppySector(unsigned char);
void SDCFloppyWriteData(unsigned char);
unsigned int SDCFloppyLSN(unsigned int,unsigned int,unsigned int);
unsigned char SDCFloppyStatus();
unsigned char SDCFloppyReadData();
unsigned char SDCPickReplyByte(unsigned char);
unsigned char SDCWriteFlashBank(unsigned short);
void SDCLoadReply(const void *, int);
bool SDCSeekSector(unsigned char,unsigned int);
bool SDCReadDrive(unsigned char,unsigned int);
bool SDCLoadNextDirPage();
std::string SDCGetFullPath(const std::string&);

//======================================================================
// DLL interface
//======================================================================
extern "C"
{
    // PakInitialize gets called first, sets up dynamic menues and captures callbacks
    __declspec(dllexport) void PakInitialize(
        slot_id_type SlotId,
        const char* const configuration_path,
        HWND hVccWnd,
        const cpak_callbacks* const callbacks)
    {
        gVccWindow = hVccWnd;
        DLOG_C("SDC %p %p %p %p %p\n",*callbacks);
        gSlotId = SlotId;
        AssertIntCallback = callbacks->assert_interrupt;
        gpSettings = new VCC::Util::settings(configuration_path);
        LoadConfig();
    }

    __declspec(dllexport) const char* PakGetName()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    __declspec(dllexport) const char* PakGetCatalogId()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    __declspec(dllexport) const char* PakGetDescription()
    {
        static char string_buffer[MAX_LOADSTRING];
        LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);
        return string_buffer;
    }

    // Clean up must also be done on DLL_UNLOAD incase VCC is closed!
    __declspec(dllexport) void PakTerminate()
    {
        CloseCartDialog(hConfigureDlg);
        hConfigureDlg = nullptr;
        UnloadDisk(0);
        UnloadDisk(1);
    }

    // Write to port
    __declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
    {
        SDCWrite(Data,Port);
        return;
    }

    // Read from port
    __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
    {
        if (ClockEnable && ((Port==0x78) | (Port==0x79) | (Port==0x7C))) {
            return cloud9_rtc.read_port(Port);
        } else if ((Port > 0x3F) & (Port < 0x60)) {
            return SDCRead(Port);
        } else {
            return 0;
        }
    }

    // Reset module
    __declspec(dllexport) void PakReset()
    {
        DLOG_C("PakReset\n");
        InitSDC();
    }

    //  Dll export run config dialog
    __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
    {
        switch (MenuID)
        {
        case 10:
            if (hConfigureDlg == nullptr)  // Only create dialog once
                hConfigureDlg = CreateDialog(gModuleInstance, (LPCTSTR) IDD_CONFIG,
                         GetActiveWindow(), (DLGPROC) SDC_Configure);
            ShowWindow(hConfigureDlg,1);
            break;
        case 11:
            SDCMountNext (0);
            SendMessage(gVccWindow,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
            break;
        }
        return;
    }

    // Return SDC status.
    __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
    {
        if (idle_ctr < 100) {
            idle_ctr++;
        } else {
            idle_ctr = 0;
            snprintf(SDC_Status,16,"SDC:%d idle",gLoadedBank);
        }
        strncpy(text_buffer,SDC_Status,buffer_size);
    }

    // Return a byte from the current PAK ROM
    __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short adr)
    {
        adr &= 0x3FFF;
        if (EnableBankWrite) {
            return SDCWriteFlashBank(adr);
        } else {
            BankWriteState = 0;  // Any read resets write state
            return(PakRom[adr]);
        }
    }

    // Return SDC menu
	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return get_menu_item(item, index);
	}

}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID rsvd)
{
    if (reason == DLL_PROCESS_ATTACH) {
        gModuleInstance = hinst;
    } else if (reason == DLL_PROCESS_DETACH) {
        PakTerminate();
    }
    return TRUE;
}

//=====================================================================
// User interface
//======================================================================

//-------------------------------------------------------------
// Return items for cartridge menu. Called for each item
//-------------------------------------------------------------
bool get_menu_item(menu_item_entry* item, size_t index)
{
    if (!item) return false;
    if (index == 0) {
        std::string tmp = gCocoDisk[0].name;
        if (tmp.empty()) {
            tmp = "empty";
        } else if (gFileList.nextload_flag) {
            tmp = tmp + " (load next)";
        } else {
            tmp = tmp + " (no next)";
        }
        SdcMenu.clear();
        SdcMenu.add("", 0, MIT_Seperator);
        SdcMenu.add("SDC Drive 0",0,MIT_Head);
        SdcMenu.add(tmp, ControlId(11),MIT_Slave);
        SdcMenu.add("SDC Config", ControlId(10), MIT_StandAlone);
    }
    return SdcMenu.copy_item(*item, index);
}

//------------------------------------------------------------
// Configure the SDC
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Configure(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hDlg);
        hConfigureDlg=nullptr;
        return TRUE;
        break;
    case WM_INITDIALOG:
        hConfigureDlg=hDlg;  // needed for LoadConfig() and Init..()
        CenterDialog(hDlg);
        LoadConfig();
        InitFlashBoxes();
        InitCardBox();
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnable,0);
        hStartBankBox = GetDlgItem(hDlg,ID_STARTUP_BANK);
        char tmp[4];
        snprintf(tmp,4,"%d",(gStartupBank & 7));
        SetWindowText(hStartBankBox,tmp);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_SD_SELECT:
            SelectCardBox();
            break;
        case ID_SD_BOX:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[MAX_PATH];
                GetWindowText(hSDCardBox,tmp,MAX_PATH);
                if (*tmp != '\0') {
                    gCurDir = tmp;
                    util::FixDirSlashes(gCurDir);
                }
            }
            return TRUE;
        case ID_UPDATE0:
            UpdateFlashItem(0);
            return TRUE;
        case ID_UPDATE1:
            UpdateFlashItem(1);
            return TRUE;
        case ID_UPDATE2:
            UpdateFlashItem(2);
            return TRUE;
        case ID_UPDATE3:
            UpdateFlashItem(3);
            return TRUE;
        case ID_UPDATE4:
            UpdateFlashItem(4);
            return TRUE;
        case ID_UPDATE5:
            UpdateFlashItem(5);
            return TRUE;
        case ID_UPDATE6:
            UpdateFlashItem(6);
            return TRUE;
        case ID_UPDATE7:
            UpdateFlashItem(7);
            return TRUE;
        case ID_STARTUP_BANK:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[4];
                GetWindowText(hStartBankBox,tmp,4);
                gStartupBank = atoi(tmp);// & 7;
                if (gStartupBank > 7) {
                    gStartupBank &= 7;
                    char tmp[4];
                    snprintf(tmp,4,"%d",gStartupBank);
                    SetWindowText(hStartBankBox,tmp);
                }
            }
            break;
        case IDOK:
            SaveConfig(hDlg);
            DestroyWindow(hDlg);
            hConfigureDlg=nullptr;
            break;
        }
    }
    return 0;
}

//------------------------------------------------------------
// Get SDC settings from ini file
//------------------------------------------------------------
void LoadConfig()
{
    gRomPath = Setting().read("DefaultPaths", "RomPath", "");
    gSDRoot  = Setting().read("SDC", "SDCardPath", "");

    DLOG_C("LoadConfig gRomPath %s\n",gRomPath.c_str());
    DLOG_C("LoadConfig gSDRoot %s\n",gSDRoot.c_str());

    util::FixDirSlashes(gSDRoot);
    if (!util::IsDirectory(gSDRoot)) {
        MessageBox (gVccWindow,
                "Invalid SD Card root\n"
                "Set SDCard Path using SDC Config"
                ,"Error",0);
    }

    for (int i=0;i<8;i++) {
        std::string tmp = "FlashFile_" + std::to_string(i);
        std::string fullname = Setting().read("SDC", tmp, "");
        std::string name = VCC::Util::StripModPath(fullname);
        VCC::Util::copy_to_char(name,FlashFile[i],MAX_PATH);
    }
    
    ClockEnable = Setting().read("SDC","ClockEnable",1);
    gStartupBank = Setting().read("SDC","StartupBank",0);
}

//------------------------------------------------------------
// Save config to ini file
//------------------------------------------------------------
bool SaveConfig(HWND hDlg)
{

    if (!util::IsDirectory(gSDRoot)) {
        MessageBox(gVccWindow,"Invalid SDCard Path\n","Error",0);
        return false;
    }

    //Save SD Card Path
    char tmp[MAX_PATH];
    hSDCardBox = GetDlgItem(hConfigureDlg,ID_SD_BOX);
    GetDlgItemText(hConfigureDlg, ID_SD_BOX, tmp, sizeof(tmp));
    if (util::IsDirectory(tmp)) {
        gSDRoot = tmp;
    } else {
        MessageBox (gVccWindow,"Bad SD Card Path. Not changed","Warning",0);
    }

    Setting().write("SDC","SDCardPath",gSDRoot);
    Setting().write("DefaultPaths","RomPath",gRomPath);

    for (int i=0;i<8;i++) {
        std::string tmp = "FlashFile_" + std::to_string(i);
        Setting().write("SDC", tmp, FlashFile[i]);
    }

    if (SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0)) {
        Setting().write("SDC","ClockEnable","1");
    } else {
        Setting().write("SDC","ClockEnable","0");
    }

    gStartupBank &= 7;
    Setting().write("SDC","StartupBank",std::to_string(gStartupBank));
    return true;
}

//------------------------------------------------------------
// Make filename string fit edit box
//------------------------------------------------------------
void FitEditTextPath(HWND hDlg, int ID, const std::string& path)
{
    HDC c;
    HWND h;
    RECT r;

    if ((c = GetDC(hDlg)) == NULL) return;
    if ((h = GetDlgItem(hDlg, ID)) == NULL) return;

    GetClientRect(h, &r);
    std::string p = path;
    p.resize(MAX_PATH);
    PathCompactPathA(c, p.data(), r.right);
    util::FixDirSlashes(p);
    SetWindowTextA(h, p.c_str());
    ReleaseDC(hDlg, c);
}

//------------------------------------------------------------
// Init flash box
//------------------------------------------------------------
void InitFlashBoxes()
{
    for (int idx=0; idx<8; idx++) {
        FitEditTextPath(hConfigureDlg,EDBOXES[idx],FlashFile[idx]);
        HWND h = GetDlgItem(hConfigureDlg,UPDBTNS[idx]);
        if (*FlashFile[idx] == '\0') {
            SetWindowText(h,">");
        } else {
            SetWindowText(h,"X");
        }
    }
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------

void InitCardBox()
{
    hSDCardBox = GetDlgItem(hConfigureDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM) gSDRoot.c_str());
}

//------------------------------------------------------------
// Modify and or Update flash box item
//------------------------------------------------------------

void UpdateFlashItem(int index)
{
    if ((index < 0) | (index > 7)) return;

    if (*FlashFile[index] != '\0') {
        *FlashFile[index] = '\0';
    } else {
        char title[64];
        snprintf(title,64,"Load Flash Bank %d",index);
        FileDialog dlg;
        dlg.setDefExt("rom");
        dlg.setFilter("Rom File\0*.rom\0All Files\0*.*\0\0");
        dlg.setTitle(title);
        dlg.setInitialDir(gRomPath.c_str());
        if (dlg.show(0,hConfigureDlg)) {
            std::string fullname = dlg.getpath();
            std::string name = VCC::Util::StripModPath(fullname);
            VCC::Util::copy_to_char(name,FlashFile[index],MAX_PATH);
            gRomPath = util::GetDirectoryPart(fullname);
        }
    }
    InitFlashBoxes();

    // Save path to ini file
    char txt[32];
    sprintf(txt,"FlashFile_%d",index);
    Setting().write("SDC",txt,FlashFile[index]);
}

//------------------------------------------------------------
// Dialog to select SD card path in user home directory
//------------------------------------------------------------

// TODO: Replace Win32 browse dialog with the modern IFileDialog API (Vista+)
void SelectCardBox()
{
    namespace fs = std::filesystem;

    // Prepare browse dialog
    BROWSEINFO bi = {};
    bi.hwndOwner = GetActiveWindow();
    bi.lpszTitle = "Set the SD card path";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;

    // Set initial folder to user profile
    LPITEMIDLIST pidlRoot = nullptr;
    if (SUCCEEDED(SHGetSpecialFolderLocation(nullptr, CSIDL_PROFILE, &pidlRoot)))
        bi.pidlRoot = pidlRoot;

    // Show dialog
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    // Free root PIDL if allocated
    if (pidlRoot)
        CoTaskMemFree(pidlRoot);

    if (pidl)
    {
        char tmp[MAX_PATH] = {};
        if (SHGetPathFromIDList(pidl, tmp))
        {
            gSDRoot = tmp;
            util::FixDirSlashes(gSDRoot);
            SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)gSDRoot.c_str());
        }

        CoTaskMemFree(pidl);
    }
}

//=====================================================================
// SDC startup
//======================================================================

//----------------------------------------------------------------------
// Init the controller. This gets called by PakReset
//----------------------------------------------------------------------
void InitSDC()
{

#ifdef USE_LOGGING
    DLOG_C("\nInitSDC\n");
    MoveWindow(GetConsoleWindow(),0,0,400,800,TRUE);
#endif

    // Make sure drives are unloaded
    SDCMountDisk (0,"",0);
    SDCMountDisk (1,"",0);

    // Load SDC settings
    LoadConfig();
    LoadRom(gStartupBank);

    SDCSetCurDir(""); // May be changed by ParseStartup()

    gCocoDisk[0] = {};
    gCocoDisk[1] = {};

    gFileList = {};

    // Process the startup config file
    ParseStartup();

    // init the interface
    IFace = {};

    return;
}

//----------------------------------------------------------------------
// Parse the startup.cfg file
//----------------------------------------------------------------------
void ParseStartup()
{
    namespace fs = std::filesystem;
    fs::path sd = fs::path(gSDRoot);

    if (!fs::is_directory(sd)) {
        DLOG_C("ParseStartup SDCard path invalid\n");
        return;
    }

    fs::path cfg = sd / "startup.cfg";
    FILE* su = std::fopen(cfg.string().c_str(), "r");
    if (su == nullptr) {
        DLOG_C("ParseStartup file not found, %s\n", cfg.string().c_str());
        return;
    }

    // Strict single char followed by '=' then path
    char buf[MAX_PATH];
    while (fgets(buf,sizeof(buf),su) != nullptr) {
        //Chomp line ending
        buf[strcspn(buf,"\r\n")] = 0;
        // Skip line if less than 3 chars;
        if (strlen(buf) < 3) continue;
        // Skip line if second char is not '='
        if (buf[1] != '=') continue;
        // Grab drive num char
        char drv = buf[0];
        // Attempt to mount drive
        switch (drv) {
        case '0':
            SDCMountDisk(0,&buf[2],0);
            break;
        case '1':
            SDCMountDisk(1,&buf[2],0);
            break;
        case 'D':
            SDCSetCurDir(&buf[2]);
            break;
        }
    }
    fclose(su);
}

//-------------------------------------------------------------
// Load rom from flash bank
//-------------------------------------------------------------
void LoadRom(unsigned char bank)
{
    unsigned char ch;
    int ctr = 0;
    char *p_rom;
    char *RomFile;

    DLOG_C("LoadRom load flash bank %d\n",bank);

//    // Skip if bank is already loaded
//    if (bank == gLoadedBank) return;

    // Sanity check before trying to save to empty file
    if (*FlashFile[gLoadedBank] == '\0') gRomDirty = 0;

    // If bank contents have been changed write the flash file
    if (gRomDirty) {
        RomFile = FlashFile[gLoadedBank];
        int rc = MessageBox(gVccWindow,
                "Save Rom Contents",
                "Write Rom File?",
                MB_YESNOCANCEL);
        switch (rc) {
        case IDYES:
        {
            FILE *hf = fopen(RomFile,"wb");
            if (!hf) {
                MessageBox (gVccWindow,
                    "Can not write Rom file",
                    "Write Rom Failed",0);
            } else {
                ctr = 0;
                p_rom = PakRom;
                while (ctr++ < 0x4000) fputc(*p_rom++, hf);
                fclose(hf);
                gRomDirty = 0;
            }
            break;
        }
        case IDNO:
            gRomDirty = 0;
            break;
        }
    }

    // Abort if user canceled or save failed
    if (gRomDirty) return;

    RomFile = FlashFile[bank];  // RomFile is a pointer

    // If bank is empty and is the StartupBank load SDC-DOS
    if (*FlashFile[bank] == '\0') {
        DLOG_C("LoadRom bank %d is empty\n",bank);
        if (bank == gStartupBank) {
            DLOG_C("LoadRom loading default SDC-DOS\n");
            strncpy(RomFile,"SDC-DOS.ROM",MAX_PATH);
        } else {
            return;
        }
    }

    // Load the rom
    std::string file = VCC::Util::QualifyModPath(RomFile);
    FILE *hf = fopen(file.c_str(),"rb");
    if (hf == nullptr) {
        std::string msg="Check Rom Path and SDC Config\n"+std::string(RomFile);
        MessageBox (gVccWindow,msg.c_str(),"SDC Rom Load Failed",0);
        return;
    }

    // Load rom from flash
    memset(PakRom, 0xFF, 0x4000);
    ctr = 0;
    p_rom = PakRom;
    while (ctr++ < 0x4000) {
        if ((ch = fgetc(hf)) < 0) break;
        *p_rom++ = (char) ch;
    }

    gLoadedBank = bank;
    fclose(hf);
    return;
}

//=====================================================================
// SDC Interface
//=====================================================================

//----------------------------------------------------------------------
// Write port.  If a command needs a data block to complete it
// will put a count (256 or 512) in IFace.bufcnt.
//----------------------------------------------------------------------
void SDCWrite(unsigned char data,unsigned char port)
{
    if (port < 0x40 || port > 0x4F) return;

    if (IFace.sdclatch) {
        switch (port) {
        // Toggle Control Latch
        case 0x40:
            if (IFace.sdclatch) IFace = {};
            break;
        // Command registor
        case 0x48:
            IFace.cmdcode = data;
            SDCCommand();
            break;
        // Command param #1
        case 0x49:
            IFace.param1 = data;
            break;
        // Command param #2 or block data receive
        case 0x4A:
            if (IFace.bufcnt > 0)
                SDCBlockReceive(data);
            else
                IFace.param2 = data;
            break;
        // Command param #3 or block data receive
        case 0x4B:
            if (IFace.bufcnt > 0)
                SDCBlockReceive(data);
            else
                IFace.param3 = data;
            break;
        // Unhandled
        default:
            DLOG_C("SDCWrite L %02x %02x\n",port,data);
            break;
        }
    } else {
        switch (port) {
        case 0x40:
            // Command latch and floppy drive select
            // Mask out halt, density, precomp, and motor
            switch (data & 0x43) { // 0b01000111
            case 0:
                IFace.sdclatch = false;
                break;
            case 0x43:
                IFace.sdclatch = true;
                break;
            case 0b00000001:
                FlopDrive = 0;
                break;
            case 0b00000010:
                FlopDrive = 1;
                break;
            case 0b00000100:
                DLOG_C("SDC floppy 2 not supported\n");
                break;
            case 0b01000000:
                DLOG_C("SDC floppy 3 not supported\n");
                break;
            default:
                DLOG_C("SDC invalid select %d\n",data);
                break;
            }
            break;
        // Flash Data
        case 0x42:
            BankData = data;
            break;
        // Flash Control
        case 0x43:
            SDCFlashControl(data);
            break;
        // floppy command
        case 0x48:
            SDCFloppyCommand(data);
            break;
        // floppy set track
        case 0x49:
            SDCFloppyTrack(data);
            break;
        // floppy set sector
        case 0x4A:
            SDCFloppySector(data);
            break;
        // floppy write data
        case 0x4B:
            SDCFloppyWriteData(data);
            break;
        // Unhandled
        default:
            DLOG_C("SDCWrite U %02x %02x\n",port,data);
            break;
        }
    }
    return;
}

//----------------------------------------------------------------------
// Read port. If there are bytes in the reply buffer return them.
//----------------------------------------------------------------------
unsigned char SDCRead(unsigned char port)
{
    unsigned char rpy = 0;

    if (IFace.sdclatch) {
        switch (port) {
        case 0x48:
            if (IFace.bufcnt > 0) {
                rpy = (IFace.reply_status != 0) ? IFace.reply_status:STA_BUSY|STA_READY;
            } else {
                rpy = IFace.status;
            }
            break;
        // Reply data 1
        case 0x49:
            rpy = IFace.reply1;
            break;
        // Reply data 2 or block reply
        case 0x4A:
            if (IFace.bufcnt > 0) {
                rpy = SDCPickReplyByte(port);
            } else {
                rpy = IFace.reply2;
            }
            break;
        // Reply data 3 or block reply
        case 0x4B:
            if (IFace.bufcnt > 0) {
                rpy = SDCPickReplyByte(port);
            } else {
                rpy = IFace.reply3;
            }
            break;
        default:
            DLOG_C("SDCRead L %02x\n",port);
            rpy = 0;
            break;
        }
    // If not SDC latched do floppy controller simulation
    } else {
        switch (port) {
        case 0x40:
            //Nitros9 driver reads this
            DLOG_C("SDCRead floppy port 40?\n");
            rpy = 0;
            break;
        // Flash control read is used by SDCDOS to detect the SDC
        case 0x43:
            rpy = gLoadedBank | (BankData & 0xF8);
            break;
        // Floppy read status
        case 0x48:
            rpy = SDCFloppyStatus();
            break;
        // Current Track
        case 0x49:
            //Nitros9 driver reads this every sector
            //DLOG_C("SDCRead floppy track?\n");
            rpy = 0;
            break;
        // Current Sector
        case 0x4A:
            //Nitros9 driver reads this every sector
            //DLOG_C("SDCRead floppy sector?\n");
            rpy = 0;
            break;
        // Floppy read data
        case 0x4B:
            rpy = SDCFloppyReadData();
            break;
        default:
            DLOG_C("SDCRead U %02x\n",port);
            rpy = 0;
            break;
        }
    }
    return rpy;
}

//----------------------------------------------------------------------
//  Dispatch SDC commands
//----------------------------------------------------------------------
void SDCCommand()
{
    switch (IFace.cmdcode & 0xF0) {
    // Read sector
    case 0x80:
        SDCReadSector();
        break;
    // Stream 512 byte sectors
    case 0x90:
        SDCStreamImage();
        break;
    // Get drive info
    case 0xC0:
        SDCGetDriveInfo();
        break;
     // Control SDC
    case 0xD0:
        SDCControl();
        break;
    // Next two are block receive commands
    case 0xA0:
    case 0xE0:
        IFace.status = STA_READY | STA_BUSY;
        IFace.bufptr = IFace.blkbuf;
        IFace.bufcnt = 256;
        IFace.half_sent = 0;
        break;
    }
    return;
}

//----------------------------------------------------------------------
// Floppy I/O
//----------------------------------------------------------------------

void SDCFloppyCommand(unsigned char data)
{
    unsigned char cmd = data >> 4;
    switch (cmd) {
    case 0:    //RESTORE
        SDCFloppyRestore();
        break;
    case 1:    //SEEK
        SDCFloppySeek();
        break;
    //case 2:  //STEP
    //case 3:  //STEPUPD
    //case 4:  //STEPIN
    //case 5:  //STEPINUPD
    //case 6:  //STEFOUT
    //case 7:  //STEPOUTUPD
    case 8:    //READSECTOR
    case 9:    //READSECTORM
        SDCFloppyReadDisk();
        break;
    case 10:   //WRITESECTOR
    case 11:   //WRITESECTORM
        SDCFloppyWriteDisk();
        break;
    case 12: //READADDRESS
        //Nitros9 driver does this
        DLOG_C("SDCFloppyCommand read address?\n");
        break;
    case 13:   //FORCEINTERUPT
        //Nitros9 driver does this
        //SDC-ROM does this after bank switch
        DLOG_C("SDCFloppyCommand force interrupt?\n");
        break;
    //case 14: //READTRACK
    //case 15: //WRITETRACK
    default:
        DLOG_C("SDCFloppyCommand %d not implemented\n",cmd);
        break;
    }
}

//----------------------------------------------------------------------
// Get drive information
//----------------------------------------------------------------------
void SDCGetDriveInfo()
{
    int drive = IFace.cmdcode & 1;
    switch (IFace.param1) {
    case 0x49:
        // 'I' - return drive information in block
        SDCGetMountedImageRec();
        break;
    case 0x43:
        // 'C' Return current directory leaf in block
        SDCGetDirectoryLeaf();
        break;
    case 0x51:
        // 'Q' Return the size of disk image in p1,p2,p3
        SDCGetSectorCount();
        break;
    case 0x3E:
        // '>' Get directory page
        SDCLoadNextDirPage();
        IFace.reply_mode=0;
        SDCLoadReply(gDirPage,256);
        break;
    case 0x2B:
        // '+' Mount next next disk in set.
        SDCMountNext(drive);
        break;
    case 0x56:
        // 'V' Get BCD firmware version number in p2, p3.
        IFace.reply2 = 0x00;
        IFace.reply3 = 0x01;
        break;
    }
}

//----------------------------------------------------------------------
//  Update SD Commands.
//----------------------------------------------------------------------
void SDCUpdateSD()
{
    switch (IFace.blkbuf[0]) {
    case 0x4D: //M
        SDCMountDisk(IFace.cmdcode&1,&IFace.blkbuf[2],0);
        break;
    case 0x6D: //m
        SDCMountDisk(IFace.cmdcode&1,&IFace.blkbuf[2],1);
        break;
    case 0x4E: //N
        SDCMountNewDisk(IFace.cmdcode&1,&IFace.blkbuf[2],0);
        break;
    case 0x6E: //n
        SDCMountNewDisk(IFace.cmdcode&1,&IFace.blkbuf[2],1);
        break;
    case 0x44: //D
        SDCSetCurDir(&IFace.blkbuf[2]);
        break;
    case 0x4C: //L
        SDCInitiateDir(&IFace.blkbuf[2]);
        break;
    case 0x4B: //K
        SDCMakeDirectory(&IFace.blkbuf[2]);
        break;
    case 0x52: //R
        SDCRenameFile(&IFace.blkbuf[2]);
        break;
    case 0x58: //X
        SDCKillFile(&IFace.blkbuf[2]);
        break;
    default:
        DLOG_C("SDCUpdateSD %02x not Supported\n",IFace.blkbuf[0]);
        IFace.status = STA_FAIL;
        break;
    }
}

//-------------------------------------------------------------------
// Load a DirPage from FileList. Called until list is exhausted
//-------------------------------------------------------------------
bool SDCLoadNextDirPage()
{
    DLOG_C("SDCLoadNextDirPage cur:%d siz:%d\n",
            gFileList.cursor,gFileList.files.size());

    std::memset(gDirPage, 0, sizeof gDirPage);

    if (gFileList.cursor >= gFileList.files.size()) {
        DLOG_C("SDCLoadNextDirPage no files left\n");
        return false;
    }

    size_t count = 0;
    while (count < 16 && gFileList.cursor < gFileList.files.size()) {
        gDirPage[count++] = FileRecord(gFileList.files[gFileList.cursor]);
        ++gFileList.cursor;
    }

    // gFileList.cursor not valid for next file loading
    gFileList.nextload_flag = false;

    return true;
}

//----------------------------------------------------------------------
// floppy restore
//----------------------------------------------------------------------
void SDCFloppyRestore()
{
    DLOG_C("FloppyRestore\n");
    FlopTrack = 0;
    FlopSector = 0;
    FlopStatus = FLP_NORMAL;
    FlopWrCnt = 0;
    FlopRdCnt = 0;
    AssertIntCallback(gSlotId, INT_NMI, IS_NMI);
}

//----------------------------------------------------------------------
// floppy seek.  No attempt is made to simulate seek times here.
//----------------------------------------------------------------------
void SDCFloppySeek()
{
    DLOG_C("FloppySeek %d %d\n",FlopTrack,FlopData);
    FlopTrack = FlopData;
}

//----------------------------------------------------------------------
// convert side (drive), track, sector, to LSN.  The floppy controller
// uses CHS addressing while hard drives, and the SDC, use LBA addressing.
// FIXME:
//  Nine sector tracks, (512 byte sectors)
//  disk type: gCocoDisk[drive].type
//----------------------------------------------------------------------
unsigned int SDCFloppyLSN(
        unsigned int drive,   // 0 or 1
        unsigned int track,   // 0 to num tracks
        unsigned int sector   // 1 to 18
    )
{
    return track * 18 + sector - 1;
}

//----------------------------------------------------------------------
// floppy read sector
//----------------------------------------------------------------------
void SDCFloppyReadDisk()
{
    auto lsn = SDCFloppyLSN(FlopDrive,FlopTrack,FlopSector);
    //DLOG_C("FloppyReadDisk %d %d\n",FlopDrive,lsn);

    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",gLoadedBank,FlopDrive,lsn);
    if (SDCSeekSector(FlopDrive,lsn)) {
        if (ReadFile(gCocoDisk[FlopDrive].hFile,FlopRdBuf,256,&FlopRdCnt,nullptr)) {
            DLOG_C("FloppyReadDisk %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_DATAREQ;
        } else {
            DLOG_C("FloppyReadDisk read error %d %d\n",FlopDrive,lsn);
            FlopStatus = FLP_READERR;
        }
    } else {
        DLOG_C("FloppyReadDisk seek error %d %d\n",FlopDrive,lsn);
        FlopStatus = FLP_SEEKERR;
    }
}

//----------------------------------------------------------------------
// floppy write sector
//----------------------------------------------------------------------
void SDCFloppyWriteDisk()
{
    // write not implemented
    int lsn = FlopTrack * 18 + FlopSector - 1;
    DLOG_C("FloppyWriteDisk %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

//----------------------------------------------------------------------
// floppy set track
//----------------------------------------------------------------------
void SDCFloppyTrack(unsigned char data)
{
    FlopTrack = data;
}

//----------------------------------------------------------------------
// floppy set sector
//----------------------------------------------------------------------
void SDCFloppySector(unsigned char data)
{
    FlopSector = data;  // Sector num in track (1-18)
    //int lsn = FlopTrack * 18 + FlopSector - 1;
    //DLOG_C("FloppySector %d lsn %d\n",FlopSector,lsn);
    FlopStatus = FLP_NORMAL;
}

//----------------------------------------------------------------------
// floppy write data
//----------------------------------------------------------------------
void SDCFloppyWriteData(unsigned char data)
{
    //DLOG_C("FloppyWriteData %d\n",data);
    if (FlopWrCnt<256)  {
        FlopWrCnt++;
        FlopWrBuf[FlopWrCnt] = data;
    } else {
        FlopStatus = FLP_NORMAL;
        if ((FlopStatus &= FLP_DATAREQ) != 0)
            AssertIntCallback(gSlotId, INT_NMI, IS_NMI);
    }
    FlopData = data;
}

//----------------------------------------------------------------------
// floppy get status
//----------------------------------------------------------------------
unsigned char SDCFloppyStatus()
{
    //DLOG_C("FloppyStatus %02x\n",FlopStatus);
    return FlopStatus;
}

//----------------------------------------------------------------------
// floppy read data
//----------------------------------------------------------------------
unsigned char SDCFloppyReadData()
{
    unsigned char rpy;
    if (FlopRdCnt>0)  {
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
    } else {
        if ((FlopStatus &= FLP_DATAREQ) != 0)
            AssertIntCallback(gSlotId, INT_NMI, IS_NMI);
        FlopStatus = FLP_NORMAL;
        rpy = 0;
    }
    return rpy;
}

//----------------------------------------------------------------------
// Can't reply with words, only bytes.  But the SDC interface design
// has most replies in words and the order the word bytes are read can
// vary so we play games to send the right ones
//----------------------------------------------------------------------
unsigned char SDCPickReplyByte(unsigned char port)
{
    unsigned char rpy = 0;

    // Byte mode bytes come on port 0x4B
    if (IFace.reply_mode == 1) {
        if (IFace.bufcnt > 0) {
            rpy = *IFace.bufptr++;
            IFace.bufcnt--;
        }
    // Word mode bytes come on port 0x4A and 0x4B
    } else {
        if (port == 0x4A) {
            rpy = IFace.bufptr[0];
        } else {
            rpy = IFace.bufptr[1];
        }
        if (IFace.half_sent) {
            IFace.bufcnt -= 2;
            IFace.bufptr += 2;
            IFace.half_sent = 0;
        } else {
            IFace.half_sent = 1;
        }
    }

    // Keep stream going until stopped
    if ((IFace.bufcnt < 1) && streaming) SDCStreamImage();

    return rpy;
}

//----------------------------------------------------------------------
// Receive block data
//----------------------------------------------------------------------
void SDCBlockReceive(unsigned char byte)
{
    if (IFace.bufcnt > 0) {
        IFace.bufcnt--;
        *IFace.bufptr++ = byte;
    }

    // Done receiving block
    if (IFace.bufcnt < 1) {
        switch (IFace.cmdcode & 0xF0) {
        case 0xA0:
            SDCWriteSector();
            break;
        case 0xE0:
            SDCUpdateSD();
            break;
        default:
            DLOG_C("SDCBlockReceive invalid cmd %d\n",IFace.cmdcode);
            IFace.status = STA_FAIL;
            break;
        }
    }
}

//----------------------------------------------------------------------
// Reply with directory leaf. Reply is 32 byte directory record. The first
// 8 bytes are the name of the leaf, the next 3 are blanks, and the
// remainder is undefined ("Private" is SDC docs)
// SDCEXP uses this with the set directory '..' command to learn the full
// path when restore last session is active. The full path is saved in
// SDCX.CFG for the next session.
//----------------------------------------------------------------------
void SDCGetDirectoryLeaf()
{
    namespace fs = std::filesystem;

    char leaf[32] = {};

    if (!gCurDir.empty()) {
        std::string leafName = fs::path(gCurDir).filename().string();
        memset(leaf, ' ', 11);
        size_t copyLen = std::min<size_t>(8,leafName.size());
        memcpy(leaf, leafName.data(), copyLen);
    } else {
        IFace.reply_status = STA_FAIL | STA_NOTFOUND;
    }
    DLOG_C("SDCGetDirectoryLeaf CurDir '%s' leaf '%s' \n", gCurDir.c_str(),leaf);

    IFace.reply_mode = 0;
    SDCLoadReply(leaf, sizeof(leaf));
}

//----------------------------------------------------------------------
// Flash control
//----------------------------------------------------------------------
void SDCFlashControl(unsigned char data)
{
    unsigned char bank = data & 7;
    EnableBankWrite = data & 0x80;
    if (EnableBankWrite) {
        BankWriteNum = bank;
    } else if (gLoadedBank != bank) {
        LoadRom(bank);
    }
}

//----------------------------------------------------------------------
// Write or kill flash bank
//
// To write a byte a sequence of four writes is used. The first three
// writes prepare the SST39S flash. The last writes the byte.
//
//    state 0 write bank 1 adr $1555 val $AA
//    state 1 write bank 0 adr $2AAA val $55
//    state 2 write bank 1 adr $1555 val $A0
//    state 3 write bank # adr [adr] val write val to adr in bank #
//
// Kill bank sector is a sequence of six writes. All six are required
// to kill the bank sector.
//
//    state 0 write bank 1 adr 1555 val AA
//    state 1 write bank 0 adr 2AAA val 55
//    state 2 write bank 1 adr 1555 val 80
//    state 4 write bank 1 adr 1555 val AA
//    state 5 write bank 0 adr 2AAA val 55
//    state 6 write bank # adr sect val 30 kill bank # sect (adr & 0x3000)
//
//----------------------------------------------------------------------
unsigned char SDCWriteFlashBank(unsigned short adr)
{
    DLOG_C("SDCWriteFlashBank %d %d %04X %02X\n",
            BankWriteState,BankWriteNum,adr,BankData);

    // BankWriteState controls the write or kill
    switch (BankWriteState) {
    case 0:
        if ((BankWriteNum == 1) && (adr == 0x1555))
            if (BankData == 0xAA) BankWriteState = 1;
        break;
    case 1:
        if ((BankWriteNum == 0) && (adr == 0x2AAA))
            if (BankData == 0x55) BankWriteState = 2;
        break;
    case 2:
        if ((BankWriteNum == 1) && (adr == 0x1555)) {
            if (BankData == 0xA0) BankWriteState = 3;
            else if (BankData == 0x80) BankWriteState = 4;
        }
        break;
    // State three writes data
    case 3:
        if (BankWriteNum != gLoadedBank) LoadRom(BankWriteNum);
        PakRom[adr] = BankData;
        gRomDirty = 1;
        break;
    // State four continues kill sequence
    case 4:
        if ((BankWriteNum == 1) && (adr == 0x1555))
            if (BankData == 0xAA) BankWriteState = 5;
        break;
    case 5:
        if ((BankWriteNum == 0) && (adr == 0x2AAA))
            if (BankData == 0x55) BankWriteState = 6;
        break;
    // State six kills (fills with 0xFF) bank sector
    case 6:
        if (BankData == 0x30) {
            if (BankWriteNum != gLoadedBank) LoadRom(BankWriteNum);
            memset(PakRom + (adr & 0x3000), 0xFF, 0x1000);
            gRomDirty = 1;
        }
        break;
    }
    EnableBankWrite = 0;
    return BankData;
}

//----------------------------------------------------------------------
// Read logical sector
// cmdcode:
//    b0 drive number
//    b1 single sided flag
//    b2 eight bit transfer flag
//----------------------------------------------------------------------
void SDCReadSector()
{
    unsigned int lsn = (IFace.param1 << 16) + (IFace.param2 << 8) + IFace.param3;

    //DLOG_C("R%d\n",lsn);

    IFace.reply_mode = ((IFace.cmdcode & 4) == 0) ? 0 : 1; // words : bytes
    if (!SDCReadDrive(IFace.cmdcode,lsn))
        IFace.status = STA_FAIL | STA_READERROR;
    else
        IFace.status = STA_NORMAL;
}

//----------------------------------------------------------------------
// Stream image data
//----------------------------------------------------------------------
void SDCStreamImage()
{
    // If already streaming continue
    if (streaming) {
        stream_lsn++;
    // Else start streaming
    } else {
        stream_cmdcode = IFace.cmdcode;
        IFace.reply_mode = ((IFace.cmdcode & 4) == 0) ? 0 : 1;
        stream_lsn = (IFace.param1 << 16) + (IFace.param2 << 8) + IFace.param3;
        DLOG_C("SDCStreamImage lsn %d\n",stream_lsn);
    }

    // For now can only stream 512 byte sectors
    int drive = stream_cmdcode & 1;
    gCocoDisk[drive].sectorsize = 512;
    gCocoDisk[drive].tracksectors = 9;

    if (stream_lsn > (gCocoDisk[drive].size/gCocoDisk[drive].sectorsize)) {
        DLOG_C("SDCStreamImage done\n");
        streaming = 0;
        return;
    }

    if (!SDCReadDrive(stream_cmdcode,stream_lsn)) {
        DLOG_C("SDCStreamImage read error %s\n",util::LastErrorTxt());
        IFace.status = STA_FAIL;
        streaming = 0;
        return;
    }
    streaming = 1;
}

//----------------------------------------------------------------------
// Write logical sector
//----------------------------------------------------------------------
void SDCWriteSector()
{
    DWORD cnt = 0;
    int drive = IFace.cmdcode & 1;
    unsigned int lsn = (IFace.param1 << 16) + (IFace.param2 << 8) + IFace.param3;
    snprintf(SDC_Status,16,"SDC:%d Wr %d,%d",gLoadedBank,drive,lsn);

    if (gCocoDisk[drive].hFile == nullptr) {
        IFace.status = STA_FAIL;
        return;
    }

    if (!SDCSeekSector(drive,lsn)) {
        IFace.status = STA_FAIL;
        return;
    }
    if (!WriteFile(gCocoDisk[drive].hFile,IFace.blkbuf,
                   gCocoDisk[drive].sectorsize,&cnt,nullptr)) {
        DLOG_C("SDCWriteSector %d %s\n",drive,util::LastErrorTxt());
        IFace.status = STA_FAIL;
        return;
    }
    if (cnt != gCocoDisk[drive].sectorsize) {
        DLOG_C("SDCWriteSector %d short write\n",drive);
        IFace.status = STA_FAIL;
        return;
    }
    IFace.status = 0;
    return;
}

//----------------------------------------------------------------------
// Return sector count for mounted disk image
//----------------------------------------------------------------------
void  SDCGetSectorCount() {

    int drive = IFace.cmdcode & 1;
    unsigned int numsec = gCocoDisk[drive].size/gCocoDisk[drive].sectorsize;
    IFace.reply3 = numsec & 0xFF;
    numsec = numsec >> 8;
    IFace.reply2 = numsec & 0xFF;
    numsec = numsec >> 8;
    IFace.reply1 = numsec & 0xFF;
    IFace.status = STA_READY;
}

//----------------------------------------------------------------------
// Return file record for mounted disk image
//----------------------------------------------------------------------
void SDCGetMountedImageRec()
{
    int drive = IFace.cmdcode & 1;
    //DLOG_C("SDCGetMountedImageRec %d %s\n",drive,gCocoDisk[drive].fullpath);
    if (strlen(gCocoDisk[drive].fullpath) == 0) {
        IFace.status = STA_FAIL;
    } else {
        IFace.reply_mode = 0;
        SDCLoadReply(&gCocoDisk[drive].filerec,sizeof(FileRecord));
    }
}

//----------------------------------------------------------------------
// $DO Abort stream and mount disk in a set of disks.
// IFace.param1  0: Next disk 1-9: specific disk.
// IFace.param2 b0: Blink Enable
//----------------------------------------------------------------------
void SDCControl()
{
    // If streaming is in progress abort it.
    if (streaming) {
        DLOG_C ("Streaming abort");
        streaming = 0;
        IFace.status = STA_READY;
        IFace.bufcnt = 0;
    } else {
        // TODO: Mount in set
        DLOG_C("SDCControl Mount in set unsupported %d %d %d \n",
                  IFace.cmdcode,IFace.param1,IFace.param2);
        IFace.status = STA_FAIL | STA_NOTFOUND;
    }
}

//----------------------------------------------------------------------
// Seek sector in drive image
//  cmdcode:
//    b0 drive number
//    b1 single sided LSN flag
//    b2 eight bit transfer flag
//
//----------------------------------------------------------------------
bool SDCSeekSector(unsigned char cmdcode, unsigned int lsn)
{
    int drive = cmdcode & 1; // Drive number 0 or 1
    int sside = cmdcode & 2; // Single sided LSN flag

    int trk = lsn / gCocoDisk[drive].tracksectors;
    int sec = lsn % gCocoDisk[drive].tracksectors;

    // The single sided LSN flag tells the controller that the lsn
    // assumes the disk image is a single-sided floppy disk. If the
    // disk is actually double-sided the LSN must be adjusted.
    if (sside && gCocoDisk[drive].doublesided) {
        DLOG_C("SDCSeekSector sside %d %d\n",drive,lsn);
        lsn = 2 * gCocoDisk[drive].tracksectors * trk + sec;
    }

    // Allow seek to expand a writable file to a resonable limit
    if (lsn > MAX_DSK_SECTORS) {
        DLOG_C("SDCSeekSector exceed max image %d %d\n",drive,lsn);
        return false;
    }

    // Seek to logical sector on drive.
    LARGE_INTEGER pos{0};
    pos.QuadPart = lsn * gCocoDisk[drive].sectorsize + gCocoDisk[drive].headersize;

    if (!SetFilePointerEx(gCocoDisk[drive].hFile,pos,nullptr,FILE_BEGIN)) {
        DLOG_C("SDCSeekSector error %s\n",util::LastErrorTxt());
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// Read a sector from drive image and load reply
//----------------------------------------------------------------------
bool SDCReadDrive(unsigned char cmdcode, unsigned int lsn)
{
    char buf[520];
    DWORD cnt = 0;
    int drive = cmdcode & 1;
    if (gCocoDisk[drive].hFile == nullptr) {
        DLOG_C("SDCReadDrive %d not open\n");
        return false;
    }

    if (!SDCSeekSector(cmdcode,lsn)) {
        return false;
    }

    if (!ReadFile(gCocoDisk[drive].hFile,buf,gCocoDisk[drive].sectorsize,&cnt,nullptr)) {
        DLOG_C("SDCReadDrive %d %s\n",drive,util::LastErrorTxt());
        return false;
    }

    if (cnt != gCocoDisk[drive].sectorsize) {
        DLOG_C("SDCReadDrive %d short read\n",drive);
        return false;
    }

    snprintf(SDC_Status,16,"SDC:%d Rd %d,%d",gLoadedBank,drive,lsn);
    SDCLoadReply(buf,cnt);
    return true;
}

//----------------------------------------------------------------------
// Load reply. Count is bytes, 512 max.
//----------------------------------------------------------------------
void SDCLoadReply(const void *data, int count)
{
    if ((count < 2) | (count > 512)) {
        DLOG_C("SDCLoadReply bad count %d\n",count);
        return;
    }

    memcpy(IFace.blkbuf,data,count);

    IFace.bufptr = IFace.blkbuf;
    IFace.bufcnt = count;
    IFace.half_sent = 0;

    // If port reads exceed the count zeros will be returned
    IFace.reply2 = 0;
    IFace.reply3 = 0;

    return;
}

//----------------------------------------------------------------------
// Create and mount a new disk image
//  IFace.param1 == 0 create 161280 byte JVC file
//  IFace.param1 SDF image number of cylinders
//  IFace.param2 SDC image number of sides
//----------------------------------------------------------------------
void SDCMountNewDisk (int drive, const char * path, int raw)
{
    DLOG_C("SDCMountNewDisk %d %s %d\n",drive,path,raw);
    // limit drive to 0 or 1
    drive &= 1;

    // Close and clear previous entry
    UnloadDisk(drive);

    // Convert from SDC format
    char file[MAX_PATH] {};
    strncpy(file,FixFATPath(path).c_str(),MAX_PATH-1);

    // Look for pre-existing file
    if (SearchFile(file)) {
        SDCOpenFound(drive,raw);
        return;
    }

    SDCOpenNew(drive,path,raw);
    return;
}

//----------------------------------------------------------------------
// Unload disk in drive
//----------------------------------------------------------------------
void UnloadDisk(int drive) {
    int d = drive & 1;
    if ( gCocoDisk[d].hFile &&
         gCocoDisk[d].hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(gCocoDisk[d].hFile);
        DLOG_C("UnloadDisk %d %s\n",drive,gCocoDisk[d].name);
    }
    gCocoDisk[d] = {};
}

//----------------------------------------------------------------------
// Mount Disk. If image path starts with '/' load drive relative
// to SDRoot, else load drive relative to the current directory.
// If there is no '.' in the path first appending '.DSK' will be
// tried then wildcard. If wildcarded the set will be available for
// the 'Next Disk' function.
// TODO: Sets of type SOMEAPPn.DSK
//----------------------------------------------------------------------
void SDCMountDisk (int drive, const char * path, int raw)
{
    DLOG_C("SDCMountDisk %d %s %d\n",drive,path,raw);

    drive &= 1;

    // Unload previous dsk
    UnloadDisk(drive);

    // Check for UNLOAD.  Path will be an empty string.
    if (*path == '\0') {
        DLOG_C("SDCMountDisk unload %d %s\n",drive,path);
        IFace.status = STA_NORMAL;
        if (drive == 0)
            SendMessage(gVccWindow,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
        return;
    }

    // Convert from SDC format
    char file[MAX_PATH] {};
    strncpy(file,FixFATPath(path).c_str(),MAX_PATH-1);

    // Look for the file
    bool found = SearchFile(file);

    char tmp[MAX_PATH];

    // if no '.' in the name try appending .DSK  or wildcard
    if (!found && (strchr(file,'.') == nullptr)) {
        strncpy(tmp,file,MAX_PATH);
        strncat(tmp,".DSK",MAX_PATH);
        found = SearchFile(tmp);
        if(!found) {
            strncpy(tmp,file,MAX_PATH);
            strncat(tmp,"*.*",MAX_PATH);
            found = SearchFile(tmp);
        }
    }

    // Give up
    if (!found) {
        DLOG_C("SDCMountDisk not found '%s'\n",file);
        IFace.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    // Mount first image found
    SDCOpenFound(drive,raw);
    return;
}

//----------------------------------------------------------------------
// Mount Next Disk from found set
//----------------------------------------------------------------------
bool SDCMountNext (int drive)
{

    // Can't mount next unless disk set has been loaded
    if (!gFileList.nextload_flag) {
        DLOG_C("SDCMountNext disabled\n");
        return false;
    }

    if (gFileList.files.size() <= 1) {
        DLOG_C("SDCMountNext List empty or only one file\n");
        return false;
    }

    gFileList.cursor++;

    if (gFileList.cursor >= gFileList.files.size()) {
        gFileList.cursor = 0;
        DLOG_C("SDCMountNext reset filelist cursor\n");
    }

    DLOG_C("SDCMountNext %d %d\n",
                gFileList.cursor, gFileList.files.size());

    // Open next image found on the drive
    SDCOpenFound(drive,0);

    return true;
}

//----------------------------------------------------------------------
// Open new disk image
//
//  IFace.Param1: $FF49 B   number of cylinders for SDF
//  IFace.Param2: $FF4A X.H number of sides for SDF image
//
//  SDCOpenNew 0 'A.DSK' 0 40 0 NEW
//  SDCOpenNew 0 'B.DSK' 0 40 1 NEW++ one side
//  SDCOpenNew 0 'C.DSK' 0 40 2 NEW++ two sides
//
//  Currently new JVC files are 35 cylinders, one sided
//  Possibly future num cylinders could specify 40 or more
//  cylinders with num cyl controlling num sides
//
//----------------------------------------------------------------------
void SDCOpenNew( int drive, const char * path, int raw)
{
    DLOG_C("SDCOpenNew %d '%s' %d %d %d\n",
          drive,path,raw,IFace.param1,IFace.param2);

    // Number of sides controls file type
    switch (IFace.param2) {
    case 0:    //NEW
        // create JVC DSK file
        break;
    case 1:    //NEW+
    case 2:    //NEW++
        DLOG_C("SDCOpenNew SDF file not supported\n");
        IFace.status = STA_FAIL | STA_INVALID;
        return;
    }

    namespace fs = std::filesystem;
    fs::path fqn = fs::path(gSDRoot) / gCurDir / path;

    if (fs::is_directory(fqn)) {
        DLOG_C("SDCOpenNew %s is a directory\n",path);
        IFace.status = STA_FAIL | STA_INVALID;
        return;
    }
    UnloadDisk(drive);

    strncpy(gCocoDisk[drive].fullpath,fqn.string().c_str(),MAX_PATH);

    // Open file for write
    gCocoDisk[drive].hFile = CreateFile(
        gCocoDisk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
        nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);

    if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
        DLOG_C("SDCOpenNew fail %d file %s\n",drive,gCocoDisk[drive].fullpath);
        DLOG_C("... %s\n",util::LastErrorTxt());
        IFace.status = STA_FAIL | STA_WIN_ERROR;
        return;
    }

    // Sectorsize and sectors per track
    gCocoDisk[drive].sectorsize = 256;
    gCocoDisk[drive].tracksectors = 18;

    IFace.status = STA_FAIL;
    if (raw) {
        // New raw file is empty - can be any format
        IFace.status = STA_NORMAL;
        gCocoDisk[drive].doublesided = 0;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].size = 0;
    } else {
        // TODO: handle SDF
        // Create headerless 35 track JVC file
        gCocoDisk[drive].doublesided = 0;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].size = 35
                         * gCocoDisk[drive].sectorsize
                         * gCocoDisk[drive].tracksectors
                         + gCocoDisk[drive].headersize;
        // Extend file to size
        LARGE_INTEGER l_siz;
        l_siz.QuadPart = gCocoDisk[drive].size;
        if (SetFilePointerEx(gCocoDisk[drive].hFile,l_siz,nullptr,FILE_BEGIN)) {
            if (SetEndOfFile(gCocoDisk[drive].hFile)) {
                IFace.status = STA_NORMAL;
            } else {
                IFace.status = STA_FAIL | STA_WIN_ERROR;
            }
        }
    }
    return;
 }

//----------------------------------------------------------------------
// Open disk image found.  Raw flag skips file type checks
//----------------------------------------------------------------------
void SDCOpenFound (int drive,int raw)
{
    drive &= 1;
    int writeprotect = 0;

    if (gFileList.cursor >= gFileList.files.size()) {
        DLOG_C("SDCOpenFound no file %d %d %d\n",
            drive, gFileList.cursor, gFileList.files.size());
        return;
    }

    UnloadDisk(drive);

    std::string name = gFileList.files[gFileList.cursor].name.c_str();
    DLOG_C("SDCOpenFound drive %d %d %s\n",
            drive, gFileList.cursor, name.c_str());

    if (gFileList.files[gFileList.cursor].isDir)
    {
        std::string pattern = name + "/*.DSK";
        DLOG_C("SDCOpenFound %s directory initiate\n",pattern.c_str());
        if (SDCInitiateDir(pattern.c_str())) {
            SDCOpenFound(drive, 0);
            if (drive == 0)
                SendMessage(gVccWindow,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
        }
        return;
    }

    std::string file = gFileList.directory + "/" + name;
    DLOG_C("SDCOpenFound %s\n", file.c_str());

    gCocoDisk[drive].hFile = CreateFileA(
        file.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
        DLOG_C("SDCOpenFound fail %d file %s\n",drive,file.c_str());
        DLOG_C("... %s\n",util::LastErrorTxt());
        int ecode = GetLastError();
        if (ecode == ERROR_SHARING_VIOLATION) {
            IFace.status = STA_FAIL | STA_INUSE;
        } else {
            IFace.status = STA_FAIL | STA_WIN_ERROR;
        }
        return;
    }

    // If more than one file in list enable nextload function
    if (gFileList.files.size() > 1) gFileList.nextload_flag = true;

    strncpy(gCocoDisk[drive].fullpath,file.c_str(),MAX_PATH);
    strncpy(gCocoDisk[drive].name,name.c_str(),MAX_PATH);

    // Default sectorsize and sectors per track
    gCocoDisk[drive].sectorsize = 256;
    gCocoDisk[drive].tracksectors = 18;

    // Grab filesize
    gCocoDisk[drive].size = gFileList.files[gFileList.cursor].size;

    // Determine file type (RAW,DSK,JVC,VDK,SDF)
    if (raw) {
        writeprotect = 0;
        gCocoDisk[drive].type = DTYPE_RAW;
        gCocoDisk[drive].headersize = 0;
        gCocoDisk[drive].doublesided = 0;
    } else {

        // Read a few bytes of the file to determine it's type
        unsigned char header[16];
        if (ReadFile(gCocoDisk[drive].hFile,header,12,nullptr,nullptr) == 0) {
            DLOG_C("SDCOpenFound header read error\n");
            IFace.status = STA_FAIL | STA_INVALID;
            return;
        }

        // Check for SDF file. SDF file has a 512 byte header and each
        // track record is 6250 bytes. Assume at least 35 tracks so
        // minimum size of a SDF file is 219262 bytes. The first four
        // bytes of the header contains "SDF1"
        if ((gCocoDisk[drive].size >= 219262) &&  // is this reasonable?
            (strncmp("SDF1",(const char *) header,4) == 0)) {
            gCocoDisk[drive].type = DTYPE_SDF;
            DLOG_C("SDCOpenFound SDF file unsupported\n");
            IFace.status = STA_FAIL | STA_INVALID;
            return;
        }

        unsigned int numsec;
        gCocoDisk[drive].headersize = gCocoDisk[drive].size & 255;
        DLOG_C("SDCOpenFound headersize %d\n",gCocoDisk[drive].headersize);
        switch (gCocoDisk[drive].headersize) {
        // JVC optional header bytes
        case 4: // First Sector     = header[3]   1 assumed
        case 3: // Sector Size code = header[2] 256 assumed {128,256,512,1024}
        case 2: // Number of sides  = header[1]   1 or 2
        case 1: // Sectors per trk  = header[0]  18 assumed
            gCocoDisk[drive].doublesided = (header[1] == 2);
            gCocoDisk[drive].type = DTYPE_JVC;
            break;
        // No apparant header
        // JVC or OS9 disk if no header, side count per file size
        case 0:
            numsec = gCocoDisk[drive].size >> 8;
            DLOG_C("SDCOpenFound JVC/OS9 sectors %d\n",numsec);
            gCocoDisk[drive].doublesided = ((numsec > 720) && (numsec <= 2880));
            gCocoDisk[drive].type = DTYPE_JVC;
            break;
        // VDK
        case 12:
            gCocoDisk[drive].doublesided = (header[8] == 2);
            writeprotect = header[9] & 1;
            gCocoDisk[drive].type = DTYPE_VDK;
            break;
        // Unknown or unsupported
        default: // More than 4 byte header is not supported
            DLOG_C("SDCOpenFound unsuported image type %d %d\n",
                drive, gCocoDisk[drive].headersize);
            IFace.status = STA_FAIL | STA_INVALID;
            return;
            break;
        }
    }

    gCocoDisk[drive].hf = gFileList.files[gFileList.cursor];
    gCocoDisk[drive].filerec = FileRecord(gCocoDisk[drive].hf);

    // Set readonly attrib per find status or file header
    if ((gCocoDisk[drive].filerec.FR_attrib & ATTR_RDONLY) != 0) {
        writeprotect = 1;
    } else if (writeprotect) {
        gCocoDisk[drive].filerec.FR_attrib |= ATTR_RDONLY;
    }

    // If file is writeable reopen read/write
    if (!writeprotect) {
        CloseHandle(gCocoDisk[drive].hFile);
        gCocoDisk[drive].hFile = CreateFile(
            gCocoDisk[drive].fullpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (gCocoDisk[drive].hFile == INVALID_HANDLE_VALUE) {
            DLOG_C("SDCOpenFound reopen fail %d\n",drive);
            DLOG_C("... %s\n",util::LastErrorTxt());
            IFace.status = STA_FAIL | STA_WIN_ERROR;
            return;
        }
    }

    if (drive == 0)
            SendMessage(gVccWindow,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);

    IFace.status = STA_NORMAL;
    return;
}

//----------------------------------------------------------------------
// Convert file name from FAT format and prepend current dir.
//----------------------------------------------------------------------
std::string SDCGetFullPath(const std::string& file)
{
    std::string out = gSDRoot;
    if (gCurDir.size() > 0) out += '/' + gCurDir;
    out += '/' + FixFATPath(file);

    DLOG_C("SDCGetFullPath in %s out %s\n",file.c_str(),out.c_str());
    return (out);
}

//----------------------------------------------------------------------
// Rename disk or directory
//   names contains two consecutive null terminated name strings
//   first is file or directory to rename, second is target name
//----------------------------------------------------------------------
void SDCRenameFile(const char *names)
{
    const char* p = names;
    std::string sfrom = p;
    p += sfrom.size() + 1;
    std::string starget = p;

    DLOG_C("SDCRenameFile %s to %s\n",sfrom.c_str(),starget.c_str());
    std::string from = SDCGetFullPath(sfrom);
    std::string target = SDCGetFullPath(starget);

    if (std::rename(from.c_str(),target.c_str())) {
        DLOG_C("SDCRenameFile %s\n", strerror(errno));
        IFace.status = STA_FAIL | STA_WIN_ERROR;
    } else {
        IFace.status = STA_NORMAL;
    }
    return;
}

//----------------------------------------------------------------------
// Delete disk or directory
//----------------------------------------------------------------------
void SDCKillFile(const char* file)
{
    namespace fs = std::filesystem;

    fs::path p = SDCGetFullPath(std::string(file));
    std::error_code ec;

    DLOG_C("SDCKillFile %s\n",file);

    // Does it exist?
    if (!fs::exists(p, ec)) {
        DLOG_C("SDCKillFile does not exist\n");
        IFace.status = STA_FAIL | STA_NOTFOUND;
        return;
    }
    // Regular file
    if (fs::is_regular_file(p, ec)) {
        if (fs::remove(p, ec)) {
            IFace.status = STA_NORMAL;
        } else {
            DLOG_C("SDCKillFile error: %s\n", ec.message().c_str());
            IFace.status = STA_FAIL | STA_WPROTECT;
        }
        return;
    }
    // Directory
    if (fs::is_directory(p, ec)) {
        if (fs::is_empty(p, ec)) {
            if (fs::remove(p, ec)) {
                IFace.status = STA_NORMAL;
            } else {
                IFace.status = STA_FAIL | STA_WPROTECT;
                DLOG_C("SDCKillFile dir error: %s\n", ec.message().c_str());
            }
        } else {
            DLOG_C("SDCKillFile directory not empty\n");
            IFace.status = STA_FAIL | STA_NOTEMPTY;
        }
        return;
    }
    // Not a file or directory (symlink, device, etc.)
    IFace.status = STA_FAIL;
}

//----------------------------------------------------------------------
// Create directory
//----------------------------------------------------------------------

void SDCMakeDirectory(const char* name)
{
    namespace fs = std::filesystem;

    std::string s = SDCGetFullPath(std::string(name));
    fs::path p(s);

    DLOG_C("SDCMakeDirectory %s\n", s.c_str());

    std::error_code ec;

    if (fs::exists(p, ec)) {
        DLOG_C("SDCMakeDirectory already exists\n");
        IFace.status = STA_FAIL | STA_INVALID;
        return;
    }

    if (fs::create_directory(p, ec)) {
        IFace.status = STA_NORMAL;
    } else {
        DLOG_C("SDCMakeDirectory error: %s\n", ec.message().c_str());
        IFace.status = STA_FAIL | STA_WIN_ERROR;
    }
}

//----------------------------------------------------------------------
// Set the Current Directory relative to previous current or to SDRoot
// This is complicated by the many ways a user can change the directory
//----------------------------------------------------------------------

void SDCSetCurDir(const char* branch)
{
    namespace fs = std::filesystem;

    DLOG_C("SetCurdir '%s'\n", branch);

    fs::path cur  = gCurDir;    // current relative directory
    fs::path root = gSDRoot;    // SD card root

    // "." or "" no change
    if (*branch == '\0' || strcmp(branch, ".") == 0) {
        DLOG_C("SetCurdir no change\n");
        IFace.status = STA_NORMAL;
        return;
    }

    // "/" go to root
    if (strcmp(branch, "/") == 0) {
        gCurDir = "";
        DLOG_C("SetCurdir to root\n");
        IFace.status = STA_NORMAL;
        return;
    }

    // ".." parent directory
    if (strcmp(branch, "..") == 0) {
        cur = cur.parent_path();
        gCurDir = cur.string();
        util::FixDirSlashes(gCurDir);
        DLOG_C("SetCurdir back %s\n", gCurDir.c_str());
        IFace.status = STA_NORMAL;
        return;
    }

    //  Reject "//"
    if (strncmp(branch, "//", 2) == 0) {
        DLOG_C("SetCurdir // invalid\n");
        IFace.status = STA_FAIL | STA_INVALID;
        return;
    }

    //  Build the candidate directory
    bool relative = (branch[0] != '/');

    fs::path candidate;
    if (relative) {
        candidate = root / cur / branch;
    } else {
        candidate = root / fs::path(branch).relative_path();
    }

    //  Validate directory
    if (!fs::is_directory(candidate)) {
        DLOG_C("SetCurdir not a directory %s\n", candidate.string().c_str());
        IFace.status = STA_FAIL | STA_NOTFOUND;
        return;
    }

    //  Update CurDir
    fs::path newCur;
    if (relative) {
        newCur = cur / branch;
    } else {
        newCur = fs::path(branch).relative_path();
    }

    // String based host files
    gCurDir = newCur.string();
    util::FixDirSlashes(gCurDir);

    DLOG_C("SetCurdir set to '%s'\n", gCurDir.c_str());
    IFace.status = STA_NORMAL;
}

//----------------------------------------------------------------------
// SDCInitiateDir command.
//----------------------------------------------------------------------
bool SDCInitiateDir(const char * path)
{
    bool rc;
    DLOG_C("SDCInitiateDir '%s'\n",path);

    // Append "*.*" if last char in path is '/';
    size_t l = strlen(path);
    if (path[l-1] == '/') {
        rc = SearchFile(std::string(path) + "*.*");
    } else {
        rc = SearchFile(path);
    }

    if (rc) {
        IFace.status = STA_NORMAL;
        return true;
    } else {
        IFace.status = STA_FAIL | STA_INVALID;
        return false;
    }
}

//=========================================================================
// Host file handling
//=========================================================================

// Construct FileRecord from a HostFile object.
FileRecord::FileRecord(const HostFile& hf) noexcept
{
    memset(this, 0, sizeof(*this));
    // name and type
    sfn_from_lfn(FR_name, FR_type, hf.name);
    // Attributes
    FR_attrib = 0;
    if (hf.isDir)     FR_attrib |= 0x10;
    if (hf.isRdOnly)  FR_attrib |= 0x01;
    // Size -> 4 reversed endian bytes
    DWORD s = hf.size;
    FR_hihi_size = (s >> 24) & 0xFF;
    FR_lohi_size = (s >> 16) & 0xFF;
    FR_hilo_size = (s >> 8)  & 0xFF;
    FR_lolo_size = (s >> 0)  & 0xFF;
}

//-------------------------------------------------------------------
// Get list of files matching pattern starting from CurDir or SDRoot
//-------------------------------------------------------------------
void GetFileList(const std::string& pattern)
{
    DLOG_C("GetFileList %s\n",pattern.c_str());

    // Init search results
    gFileList = {};

    // None if pattern is empty
    if (pattern.empty()) return;

    // Set up search path.
    bool not_at_root = !gCurDir.empty();
    bool rel_pattern = pattern.front() != '/';

    std::string search_path = gSDRoot;
    if (rel_pattern) {
        search_path += "/";
        if (not_at_root)
            search_path += gCurDir + "/";
    }
    search_path += pattern;

    // A directory lookup if pattern name is "*" or "*.*";
    std::string fnpat = util::GetFileNamePart(pattern);
    bool dir_lookup = (fnpat == "*" || fnpat == "*.*");

    std::string search_dir = util::GetDirectoryPart(search_path);

    // Include ".." if dir_lookup and search_dir is not SDRoot
    if (dir_lookup && util::GetDirectoryPart(search_path) != gSDRoot) {
        gFileList.append({"..", 0, true, false});
    }

    // Add matching files to the list
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        gFileList = {};
        return;
    }
    do {
        if (strcmp(fd.cFileName,".") == 0) continue;    // exclude single dot file
        if (PathIsLFNFileSpecA(fd.cFileName)) continue; // exclude ugly or long filenames
        if (fd.nFileSizeHigh != 0) continue;            // exclude files > 4 GB
        gFileList.append({fd});
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    // Save directory
    gFileList.directory = search_dir;

    DLOG_C("GetFileList found %d\n",gFileList.files.size());
}

//-------------------------------------------------------------------
// SortFileList() may be needed when mounting a directory
//-------------------------------------------------------------------
void SortFileList()
{
    std::sort(
        gFileList.files.begin(),
        gFileList.files.end(),
        [] (const HostFile& a, const HostFile& b) {
            return a.name < b.name;
        }
    );
    gFileList.cursor = 0;
}

//----------------------------------------------------------------------
//  Host File search.
//----------------------------------------------------------------------
bool SearchFile(const std::string& pat)
{
    // Fill gFileList with files found.
    GetFileList(pat);
    // Update menu to show next disk status
    SendMessage(gVccWindow,WM_COMMAND,(WPARAM) IDC_MSG_UPD_MENU,(LPARAM) 0);
    // Return true if something found
    return (gFileList.files.size() > 0);
}

//----------------------------------------------------------------------
// A file path may use 11 char FAT format which does not use a separater
// between name and extension. User is free to use standard dot format.
//    "FOODIR/FOO.DSK"     = FOODIR/FOO.DSK
//    "FOODIR/FOO     DSK" = FOODIR/FOO.DSK
//    "FOODIR/ALONGFOODSK" = FOODIR/ALONGFOO.DSK
//----------------------------------------------------------------------
std::string FixFATPath(const std::string& sdcpath)
{
    std::filesystem::path p(sdcpath);

    auto chop = [](std::string s) {
        size_t space = s.find(' ');
        if (space != std::string::npos) s.erase(space);
        return s;
    };

    std::string fname = p.filename().string();
    if (fname.length() == 11 && fname.find('.') == std::string::npos) {
        auto nam = chop(fname.substr(0,8));
        auto ext = chop(fname.substr(8,3));
        fname = ext.empty() ? nam : nam + "." + ext;
    }

    std::filesystem::path out = p.parent_path() / fname;

    DLOG_C("FixFATPath in %s out %s\n",sdcpath.c_str(),out.generic_string().c_str());
    return out.generic_string();
}

//-------------------------------------------------------------------
// Convert string containing possible FAT name and extension to an
// LFN string. Returns empty string if invalid LFN
//-------------------------------------------------------------------
std::string NormalizeInputToLFN(const std::string& s)
{
    if (s.empty()) return {};
    if (s.size() > 11) return {};
    if (s == "..") return "..";

    // LFN candidate
    if (s.find('.') != std::string::npos) {
        if (!PathIsLFNFileSpecA(s.c_str()))
            return {}; // invalid
        return s;
    }

    // SFN candidate
    char name[8];
    char ext[3];
    sfn_from_lfn(name,ext,s);
    return lfn_from_sfn(name,ext);
}

//-------------------------------------------------------------------
// Convert LFN to FAT filename parts, 8 char name, 3 char ext
// A LNF file is less than 4GB and has a short (8.3) name.
//-------------------------------------------------------------------
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn)
{
    // Special case: parent directory
    if (lfn == "..") {
        copy_to_fixed_char(name, "..");
        std::fill(ext, ext + 3, ' ');
        return;
    }

    size_t dot = lfn.find('.');
    std::string base, extension;

    if (dot == std::string::npos) {
        base = lfn;
    } else {
        base = lfn.substr(0, dot);
        extension = lfn.substr(dot + 1);
    }

    copy_to_fixed_char(name, base);
    copy_to_fixed_char(ext, extension);
}

//-------------------------------------------------------------------
// Convert FAT filename parts to LFN. Returns empty string if invalid
//-------------------------------------------------------------------
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3])
{
    std::string base(name, 8);
    std::string extension(ext, 3);

    base = util::trim_right_spaces(base);
    extension = util::trim_right_spaces(extension);

    if (base == ".." && extension.empty())
    return "..";

    std::string lfn = base;

    if (!extension.empty())
    lfn += "." + extension;

    if (lfn.empty())
        return {};

    if (!PathIsLFNFileSpecA(lfn.c_str()))
        return {};

    return lfn;
}

//-------------------------------------------------------------------
// Copy string to fixed size char array (non terminated)
//-------------------------------------------------------------------
template <size_t N>
void copy_to_fixed_char(char (&dest)[N], const std::string& src)
{
    size_t i = 0;
    for (; i < src.size() && i < N; ++i)
        dest[i] = src[i];
    for (; i < N; ++i)
        dest[i] = ' ';
}

