
////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
//
//----------------------------------------------------------------------
// SDC simulator E J Jaquay 2025 
//----------------------------------------------------------------------
//#define USE_LOGGING

#include <ShlObj.h>    // for SH Browse info
#include <Windows.h>

#include <vcc/devices/rtc/ds1315.h>
#include <vcc/common/logger.h>
#include <vcc/common/DialogOps.h>
#include <vcc/core/cartridge_capi.h>
#include <vcc/core/limits.h>
#include <vcc/utils/configuration_serializer.h>
#include "../CartridgeMenu.h"
#include "resource.h"
#include "sdc_cartridge.h"
#include "sdc_configuration.h"

// private functions 
bool SaveConfig(HWND);
void SelectCardBox();
void UpdateFlashItem(int);
void ModifyFlashItem(int);
void InitCardBox();
void InitEditBoxes();

// Flash bank file names
char FlashFile[8][MAX_PATH] = {};

// config control handles
HWND hControlDlg = nullptr;
HWND hConfigureDlg = nullptr;
HWND hSDCardBox = nullptr;
HWND hStartupBank = nullptr;

int EDBOXES[8] = {ID_TEXT0,ID_TEXT1,ID_TEXT2,ID_TEXT3,
                  ID_TEXT4,ID_TEXT5,ID_TEXT6,ID_TEXT7};
int UPDBTNS[8] = {ID_UPDATE0,ID_UPDATE1,ID_UPDATE2,ID_UPDATE3,
                  ID_UPDATE4,ID_UPDATE5,ID_UPDATE6,ID_UPDATE7};

int ClockEnable = 0;
char MPIPath[MAX_PATH] = {};

//-------------------------------------------------------------
// Generate menu for configuring the SDC
//-------------------------------------------------------------
void BuildCartridgeMenu()
{
    DLOG_C("BuildCartridgeMenu()\n");
    CartMenuCallback(gHostKey, "", MID_BEGIN, MIT_Head);
    CartMenuCallback(gHostKey, "", MID_ENTRY, MIT_Seperator);
    CartMenuCallback(gHostKey, "SDC Config", ControlId(10), MIT_StandAlone);
    CartMenuCallback(gHostKey, "SDC Control", ControlId(11), MIT_StandAlone);
    CartMenuCallback(gHostKey, "", MID_FINISH, MIT_Head);
    DLOG_C("ret\n");
}

//------------------------------------------------------------
// Control SDC multi floppy
//------------------------------------------------------------
LRESULT CALLBACK
SDC_Control(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hDlg);
        hControlDlg=nullptr;
        return TRUE;
        break;
    case WM_INITDIALOG:
        CenterDialog(hDlg);
        SetFocus(GetDlgItem(hDlg,ID_NEXT));
        return TRUE;
        break;
    case WM_PAINT:
        update_disk0_box();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_NEXT:
            MountNext (0);
            SetFocus(GetParent(hDlg));
            return TRUE;
            break;
        }
    }
    return FALSE;
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
        InitEditBoxes();
        InitCardBox();
        SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnable,0);
        hStartupBank = GetDlgItem(hDlg,ID_STARTUP_BANK);
        char tmp[4];
        snprintf(tmp,4,"%d",(StartupBank & 7));
        SetWindowText(hStartupBank,tmp);
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
                if (*tmp != '\0') strncpy(SDCard,tmp,MAX_PATH);
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
        case ID_TEXT0:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(0);
            return TRUE;
        case ID_TEXT1:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(1);
            return FALSE;
        case ID_TEXT2:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(2);
            return FALSE;
        case ID_TEXT3:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(3);
            return FALSE;
        case ID_TEXT4:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(4);
            return FALSE;
        case ID_TEXT5:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(5);
            return FALSE;
        case ID_TEXT6:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(6);
            return FALSE;
        case ID_TEXT7:
            if (HIWORD(wParam) == EN_CHANGE) ModifyFlashItem(7);
            return FALSE;
        case ID_STARTUP_BANK:
            if (HIWORD(wParam) == EN_CHANGE) {
                char tmp[4];
                GetWindowText(hStartupBank,tmp,4);
                StartupBank = atoi(tmp);// & 7;
                if (StartupBank > 7) {
                    StartupBank &= 7;
                    char tmp[4];
                    snprintf(tmp,4,"%d",StartupBank);
                    SetWindowText(hStartupBank,tmp);
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
    ::vcc::utils::configuration_serializer conf(IniFile);

    strncpy(MPIPath,conf.read("DefaultPaths","MPIPath").c_str(),MAX_PATH);
    strncpy(SDCard,conf.read("SDC","SDCardPath").c_str(),MAX_PATH);

    char key[32];
    for (int i=0;i<8;i++) {
        snprintf(key,32,"FlashFile_%d",i);
        strncpy(FlashFile[i],conf.read("SDC",key).c_str(),MAX_PATH);
    }

    ClockEnable = conf.read("SDC","ClockEnable",1);
    StartupBank = conf.read("SDC","StartupBank",0);
}

//------------------------------------------------------------
// Save SDC settings to ini file
//------------------------------------------------------------
bool SaveConfig(HWND hDlg)
{
    if (!IsDirectory(SDCard)) {
        MessageBox(nullptr,"Invalid SDCard Path\n","Error",0);
        return false;
    }

    if (SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0)==BST_CHECKED) {
        ClockEnable = 1;
    } else {
        ClockEnable = 0;
    }

    ::vcc::utils::configuration_serializer conf(IniFile);

    conf.write("SDC","SDCardPath",SDCard);
    char key[32];
    for (int i=0;i<8;i++) {
        snprintf(key,32,"FlashFile_%d",i);
        conf.write("SDC",key,FlashFile[i]);
    }
    conf.write("SDC","ClockEnable",ClockEnable);
    conf.write("SDC","StartupBank",StartupBank & 7);
    return true;
}

//------------------------------------------------------------
// Init flash box
//------------------------------------------------------------
void InitEditBoxes()
{
    for (int index=0; index<8; index++) {
        HWND h;
        h = GetDlgItem(hConfigureDlg,EDBOXES[index]);
        SetWindowText(h,FlashFile[index]);
        h = GetDlgItem(hConfigureDlg,UPDBTNS[index]);
        if (*FlashFile[index] == '\0') {
            SetWindowText(h,">");
        } else {
            SetWindowText(h,"X");
        }
    }
}

//----------------------------------------------------------------------
// Put disk 0 name to control dialog
//----------------------------------------------------------------------
void update_disk0_box()
{ 
    if (hControlDlg != nullptr) {
        HWND h = GetDlgItem(hControlDlg,ID_DISK0);
        SendMessage(h, WM_SETTEXT, 0, (LPARAM) DiskName(0).c_str());
    }
}

//------------------------------------------------------------
// Init SD card box
//------------------------------------------------------------

void InitCardBox()
{
    hSDCardBox = GetDlgItem(hConfigureDlg,ID_SD_BOX);
    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)SDCard);
}

//------------------------------------------------------------
// Modify and or Update flash box item
//------------------------------------------------------------
void ModifyFlashItem(int index)
{
    if ((index < 0) | (index > 7)) return;
    HWND h = GetDlgItem(hConfigureDlg,EDBOXES[index]);
    GetWindowText(h, FlashFile[index], MAX_PATH);
}

void UpdateFlashItem(int index)
{
    char filename[MAX_PATH]={};

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
        dlg.setInitialDir(MPIPath);   // FIXME someday
        if (dlg.show(0,hConfigureDlg)) {
            dlg.getupath(filename,MAX_PATH); // cvt to unix style
            strncpy(FlashFile[index],filename,MAX_PATH);
        }
    }
    InitEditBoxes();
}

//------------------------------------------------------------
// Dialog to select SD card path in user home directory
//------------------------------------------------------------
void SelectCardBox()
{
    // Prompt user for path
    BROWSEINFO bi = { nullptr };
    bi.hwndOwner = GetActiveWindow();
    bi.lpszTitle = "Set the SD card path";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON;

    // Start from user home diretory
    SHGetSpecialFolderLocation
        (nullptr,CSIDL_PROFILE, const_cast<LPITEMIDLIST*>(& bi.pidlRoot));

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        SHGetPathFromIDList(pidl,SDCard);
        CoTaskMemFree(pidl);
    }

    // Sanitize slashes
    for(unsigned int i=0; i<strlen(SDCard); i++) {
        if (SDCard[i] == '\\') SDCard[i] = '/';
    }

    SendMessage(hSDCardBox, WM_SETTEXT, 0, (LPARAM)SDCard);
}

