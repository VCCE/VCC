//============================================================================
// Copyright 2015 by Joseph Forgione
// This file is part of VCC (Virtual Color Computer).
//
//    VCC is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    VCC is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with VCC (Virtual Color Computer).  If not, see
//    <http://www.gnu.org/licenses/>.
//
//    Disassembly Display - Part of the Debugger package for VCC
//    Author: Ed Jaquay
//============================================================================

#include <Windows.h>
#include <Windowsx.h>
#include <commdlg.h>
#include <Richedit.h>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "defines.h"
#include "vcc.h"
#include "resource.h"
#include "logger.h"
#include "tcc1014mmu.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "OpDecoder.h"
#include "Disassembler.h"
#include "Audio.h"

#define HEXSTR(i,w) Debugger::ToHexString(i,w,true)

#define MAXLINES 400

namespace VCC {

/**************************************************/
/*        Local Function Templates                */
/**************************************************/

// Dialogs
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK BreakpointsDlgProc(HWND,UINT,WPARAM,LPARAM);

// Functions used to subclass controls
LRESULT CALLBACK SubEditDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK SubTextDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
WNDPROC BlocDlgProc;
WNDPROC TextDlgProc;

// Handler for address and count edit boxes
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);

// Handler for disassembly text richedit control
BOOL ProcTextDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);

// Addressing mode and converters
void ToggleAddrMode();
void EnableBlockEdit(bool);
int CpuBlock(int);
int CpuToReal(int);
int RealToCpu(int);
int HexToUint(char *);

// Disassembler and helpers
void DecodeAddr();
void Disassemble(unsigned short, unsigned short);
int DecodeModHdr(unsigned short, unsigned short, std::string *mhdr);

// Establish text line 
int FindAdrLine(int);
void SetCurrentLine();

// Breakpoint control
void RemoveHaltpoint(int);
void SetHaltpoint(bool);
void RefreshHPlist();
void ListHaltpoints();
void FindHaltpoints();
bool BreakpointPC();

// Highlighting
void HighlightPC();
void UnHighlightPC();
void HighlightLine(int,COLORREF,int);

// Information / Error line
void SetErrorText(const char *);
void ClrErrorText();

// String functions used for decode
std::string PadRight(std::string const&,size_t);
std::string OpFDB(int,std::string,std::string,std::string);
std::string OpFCB(int,std::string,std::string);
std::string FmtLine(int,std::string,std::string,std::string,std::string);

/**************************************************/
/*            Static Variables                    */
/**************************************************/

CriticalSection Section_;

HINSTANCE hVccInst;

// Dialog and control handles
HWND hDismDlg = NULL;  // Disassembler Dialog
HWND hBrkpDlg = NULL;  // Breakpoints Dialog
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtBloc = NULL;  // Bock number editbox
HWND hEdtAPPY = NULL;  // Apply button
HWND hDisText = NULL;  // Richedit20 box for output
HWND hErrText = NULL;  // Error text box

// RealAdrMode indicates what addressing was used for decode
bool RealAdrMode = FALSE;

// Os9Decode indicates block and offset used for start address
bool Os9Decode = FALSE;

// Line highlight status
int HighlightedPC = -1;
int PClinePos = 0;

// MMUregs at time of the last decode
MMUState MMUregs;

// Default Info/Text line content, current brush and color
char initTxt[] = "Address and Block are hexadecimal";
HBRUSH hbrErrorTxt = NULL;
COLORREF ErrorTxtColor = RGB(0,0,0);

// Decode data
std::string sDecoded = {}; // Disassembly string
int NumDisLines = 0;       // Number of lines in disassembly;
int DisLinePos[MAXLINES];  // Start positions of disassembly lines
int DisLineAdr[MAXLINES];  // Instruction addresses of disassembly lines
int CurrentLineNum=-1;     // CurrentLineNum selected. (-1 == none)

// mHaltpoints container.
// A new style breakpoint is named 'Haltpoint' to avoid conflict
// with the breakpoints structure used by the source code debugger.
// The real address and instruction at the breakpoint are saved.
// Haltpoints are stored in Haltpoints std::map container. Haltpoints
// are active when the cpu is in run state and are inactive
// when the cpu is halted.
struct Haltpoint
{
    int addr;  // Real address
    int lpos;  // Char index into text
    unsigned char instr;
    bool placed;
    bool exists;
};
std::map<int,Haltpoint> mHaltpoints{};
void ApplyHaltPoint(Haltpoint &,bool);

// Help text
char DbgHelp[] =
    "'Real Mem' checkbox selects Real vs CPU Addressing.\n"
    "'Os9 mode' checkbox selects OS9 disassembly.\n"
    "'Address' is where address to disassemble is entered.\n"
    "In Real Mem Os9 mode 'Address' is offset from 'Block'.\n"
    "'Decode' (or Enter key) decodes from the set address.\n"
    "Disassembly tracks the CPU PC register when paused.\n\n"
    "The following hot keys can be used:\n\n"
    "  'P'  Pause the CPU.\n"
    "  'G'  Go - unpause the CPU.\n"
    "  'S'  Step to next instruction while paused.\n"
    "  'B'  Set breakpoint at selected line.\n"
    "  'R'  Remove breakpoint at selected line.\n"
    "  'L'  Start Breakpoints list window.\n"
    "  'M'  Toggle between real and CPU address mode.\n"
    "  'I'  Shows Processor State window.\n"
    "  'K'  Removes (kills) all breakpoints.\n";

/**************************************************/
/*      Create Disassembler Dialog Window         */
/**************************************************/
void
OpenDisassemblerWindow(HINSTANCE hInstance, HWND hParent)
{
    hVccInst = hInstance;
    if (hDismDlg == NULL) {
        hDismDlg = CreateDialog ( hInstance,
                                  MAKEINTRESOURCE(IDD_DISASSEMBLER),
                                  hParent,
                                  DisassemblerDlgProc );
        ShowWindow(hDismDlg, SW_SHOWNORMAL);
    }
    SetFocus(hDismDlg);
}

/**************************************************/
/*              Dialog Processing                 */
/**************************************************/
INT_PTR CALLBACK DisassemblerDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_INITDIALOG:
        // Grab control handles
        hEdtAddr = GetDlgItem(hDlg, IDC_EDIT_PC_ADDR);
        hEdtBloc = GetDlgItem(hDlg, IDC_EDIT_BLOCK);
        hEdtAPPY = GetDlgItem(hDlg, IDAPPLY);
        hDisText = GetDlgItem(hDlg, IDC_DISASSEMBLY_TEXT);
        hErrText = GetDlgItem(hDlg, IDC_ERROR_TEXT);

        // Hook (subclass) text controls
        AddrDlgProc = (WNDPROC) GetWindowLongPtr(hEdtAddr,GWLP_WNDPROC);
        BlocDlgProc = (WNDPROC) GetWindowLongPtr(hEdtBloc,GWLP_WNDPROC);
        TextDlgProc = (WNDPROC) GetWindowLongPtr(hDisText,GWLP_WNDPROC);
        SetWindowLongPtr(hEdtAddr,GWLP_WNDPROC,(LONG_PTR) SubEditDlgProc);
        SetWindowLongPtr(hEdtBloc,GWLP_WNDPROC,(LONG_PTR) SubEditDlgProc);
        SetWindowLongPtr(hDisText,GWLP_WNDPROC,(LONG_PTR) SubTextDlgProc);

        // Set Consolas font (fixed) in disassembly edit box
        CHARFORMAT disfmt;
        disfmt.cbSize = sizeof(disfmt);
        SendMessage(hDisText,EM_GETCHARFORMAT,(WPARAM) SCF_DEFAULT,(LPARAM) &disfmt);
        strcpy(disfmt.szFaceName,"Consolas");
        disfmt.yHeight=180;
        SendMessage(hDisText,EM_SETCHARFORMAT,(WPARAM) SCF_DEFAULT,(LPARAM) &disfmt);

        // Inital values in text boxes
        SetWindowTextA(hDisText,"");
        ClrErrorText();

        // Disable Block edit box until needed
        EnableBlockEdit(FALSE);

        // Start a timer for showing current status
        SetTimer(hDlg, IDT_BRKP_TIMER, 250, (TIMERPROC)NULL);
        break;

    case WM_PAINT:
        // Give edit control focus
        SetFocus(hEdtAddr);
        SendMessage(hEdtAddr,EM_SETSEL,0,-1); //Select contents
        return FALSE;

    case WM_CTLCOLORSTATIC:
        if ((HWND)lPrm == hErrText) {
            HDC hdc = (HDC) wPrm;
            if (hbrErrorTxt==NULL)
                hbrErrorTxt = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            SetTextColor(hdc,ErrorTxtColor);
            SetBkColor(hdc,GetSysColor(COLOR_3DFACE));
            return (INT_PTR) hbrErrorTxt;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            KillHaltpoints();
            EmuState.Debugger.Enable_Halt(false);
            DestroyWindow(hDlg);
            hDismDlg = NULL;
            return FALSE;
        case IDAPPLY:
            DecodeAddr();
            SetFocus(hEdtAddr);
            return TRUE;
        case IDC_BTN_OS9:
            if (IsDlgButtonChecked(hDismDlg,IDC_BTN_OS9)==BST_CHECKED) {
                Os9Decode = TRUE;
                EnableBlockEdit(RealAdrMode);
            } else {
                Os9Decode = FALSE;
                EnableBlockEdit(FALSE);
            }
            return TRUE;
        case IDC_PHYS_MEM:
            if (IsDlgButtonChecked(hDlg,IDC_PHYS_MEM)==BST_CHECKED) {
                RealAdrMode = TRUE;
                EnableBlockEdit(Os9Decode);
            } else {
                RealAdrMode = FALSE;
                EnableBlockEdit(FALSE);
            }
            return TRUE;
        case IDC_BTN_HELP:
            MessageBox(hDismDlg,DbgHelp,"Usage",0);
            SetFocus(hEdtAddr);
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wPrm == IDT_BRKP_TIMER) {
            HighlightPC();
            return TRUE;
        }
        break;

    }
    return FALSE;
}

/***************************************************/
/*        Process edit control messages            */
/***************************************************/
LRESULT CALLBACK SubEditDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    HWND nxtctl;
    char ch;
    switch (msg) {
    case WM_CHAR:
        ch = toupper(wPrm);
        // Hex digits made uppercase in edit text
        if (strchr("0123456789ABCDEF",ch)) {
            wPrm = ch;
            break;
        }
        // Hot keys sent to disassembly text window
        if (strchr("GPSLMIK",ch)) {
            SendMessage(hDisText,msg,ch,lPrm);
            return TRUE;
        }
        // Other characters
        switch (ch) {
        // Enter does the disassembly
        case VK_RETURN:
            DecodeAddr();
            return TRUE;
        // Tab moves between active edit boxes
        case VK_TAB:
            ClrErrorText();
            if (hCtl==hEdtAddr && RealAdrMode && Os9Decode) {
                nxtctl = hEdtBloc;
            } else {
                nxtctl = hEdtAddr;
            }
            SetFocus(nxtctl);
            SendMessage(nxtctl,EM_SETSEL,0,-1); //Select contents
            return TRUE;
        // keys used by edit control
        case VK_BACK:
        case VK_LEFT:
        case VK_RIGHT:
            break;
        // ignore unmatched keys
        default:
            return TRUE;
        }
        break;

    case WM_KEYDOWN:
        switch (wPrm) {
        case VK_DOWN:
        case VK_UP:
            SetFocus(hDisText);
            SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
        }
        break;
    case WM_KEYUP:
        switch (wPrm) {
        case VK_DOWN:
        case VK_UP:
            SetFocus(hDisText);
            SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
        }
        break;
    }
    // Everything else sent to original control processing
    if (hCtl == hEdtAddr)
        return CallWindowProc(AddrDlgProc,hCtl,msg,wPrm,lPrm);
    if (hCtl == hEdtBloc)
        return CallWindowProc(BlocDlgProc,hCtl,msg,wPrm,lPrm);
    return TRUE;
}

/***************************************************/
/*       Process Disassembly window messages       */
/***************************************************/
LRESULT CALLBACK SubTextDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_CHAR:
        switch (toupper(wPrm)) {
        case 'B':
            SetHaltpoint(true);
            return TRUE;
            break;
        // Remove Breakpoint
        case 'R':
            SetHaltpoint(false);
            return TRUE;
            break;
        // Show Procesor State Window
        case 'I':
            SendMessage(GetWindow(hDismDlg,GW_OWNER),
                        WM_COMMAND,ID_PROCESSOR_STATE,0);
            SetFocus(hEdtAddr);
            return TRUE;
            break;
        // Remove all haltpoints
        case 'K':
            KillHaltpoints();
            return TRUE;
            break;
        // List haltpoints
        case 'L':
            ListHaltpoints();
            return TRUE;
            break;
        case 'M':
            ToggleAddrMode();
            return TRUE;
            break;
        // Toggle pause go
        case 'G':
            if (EmuState.Debugger.IsHalted()) {
                PauseAudio(0);
                EmuState.Debugger.QueueRun();
            }
            return TRUE;
            break;
       case 'P':
            if (!EmuState.Debugger.IsHalted()) {
                EmuState.Debugger.QueueHalt();
                PauseAudio(1);
            }
            return TRUE;
            break;
        // Step
        case 'S':
            EmuState.Debugger.QueueStep();
            UnHighlightPC();
            return TRUE;
            break;
        case VK_TAB:
            SetFocus(hEdtAddr);
            return TRUE;
            break;
        }
        break;

    case WM_LBUTTONUP:
        SetCurrentLine();
        break;
    case WM_KEYUP:
        switch (wPrm) {
        case VK_PRIOR:
        case VK_NEXT:
        case VK_DOWN:
        case VK_UP:
            SetCurrentLine();
            break;
        }
        break;
    }

    // Forward messages to original control
    return CallWindowProc(TextDlgProc,hCtl,msg,wPrm,lPrm);
}

/**************************************************/
/*    Breakpoints list Dialog Processing          */
/**************************************************/
INT_PTR CALLBACK BreakpointsDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    int sel;
    HWND hList;
    char buf[64];

    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            DestroyWindow(hDlg);
            hBrkpDlg = NULL;
            break;
        case IDC_BTN_DEL_BREAKPOINT:
            hList = GetDlgItem(hDlg,IDC_LIST_BREAKPOINTS);
            sel = SendMessage(hList,LB_GETCURSEL,0,0);
            SendMessage(hList,LB_GETTEXT, sel, (LPARAM) &buf);
            buf[6]='\0';
            RemoveHaltpoint(HexToUint(buf));
            RefreshHPlist();
            return true;
            break;
        }
    }
    return false;
}

/**************************************************/
/*             Set Error text                     */
/**************************************************/
void SetErrorText(const char * txt) {
    ErrorTxtColor = RGB(255,0,0);
    SendMessage(hDismDlg,WM_CTLCOLORSTATIC,
            (WPARAM) GetDC(hErrText),(LPARAM) hErrText);
    SetWindowTextA(hErrText,txt);
}

/**************************************************/
/*             Clr Error text                     */
/**************************************************/
void ClrErrorText() {
    ErrorTxtColor = RGB(0,0,0);
    SendMessage(hDismDlg,WM_CTLCOLORSTATIC,
            (WPARAM) GetDC(hErrText),(LPARAM) hErrText);
    SetWindowTextA(hErrText,initTxt);
}

/**************************************************/
/*       Enable/Disable Block edit box            */
/**************************************************/
void EnableBlockEdit(bool flag) {
    if (flag) {
        EnableWindow(hEdtBloc,TRUE);
        SetFocus(hEdtBloc);
        SendMessage(hEdtBloc,EM_SETSEL,0,-1); //Select contents
    } else {
        EnableWindow(hEdtBloc,FALSE);
        SetFocus(hEdtAddr);
        SendMessage(hEdtAddr,EM_SETSEL,0,-1); //Select contents
    }
    return;
}

/***************************************************/
/*    Toggle address mode between CPU and Real     */
/***************************************************/
void ToggleAddrMode()
{
    char buf[16];
    int addr;
    int block;
    int tmpadr;

    // Get real or CPU address of top line
    GetWindowText(hEdtAddr, buf, 8);
    addr = HexToUint(buf);

    // Map address to CPU if possible
    if (RealAdrMode) {
        // Adjust for block addressing
        if (Os9Decode) {
            GetWindowTextA(hEdtBloc,buf, 8);
            block = HexToUint(buf);
            addr += block * 0x2000;
        }
        // Convert to CPU address
        tmpadr = RealToCpu(addr);
        if (tmpadr < 0) {
            SetErrorText("Address not Mapped by MMU");
            return;
        }
        addr = tmpadr;
        RealAdrMode = false;
        Button_SetCheck(GetDlgItem(hDismDlg,IDC_PHYS_MEM),BST_UNCHECKED);
        EnableBlockEdit(FALSE);

    // Map address to real
    } else {
        // Convert to Real address
        tmpadr = CpuToReal(addr);
        // Adjust for block addressing
        if (Os9Decode) {
            block = (tmpadr >> 13);
            addr = tmpadr & 0x1FFF;
            EnableBlockEdit(TRUE);
        } else {
            addr = tmpadr;
            block = 0;
        }
        SetWindowText(hEdtBloc,HEXSTR(block,0).c_str());
        RealAdrMode = true;
        Button_SetCheck(GetDlgItem(hDismDlg,IDC_PHYS_MEM),BST_CHECKED);
    }

    SetWindowText(hEdtAddr,HEXSTR(addr,0).c_str());
    DecodeAddr();
}

/***************************************************/
/*      Convert Cpu address to real address        */
/***************************************************/
int CpuToReal(int cpuadr) {
   int block = CpuBlock(cpuadr);
   int offset = cpuadr & 0x1FFF;
   return offset + block * 0x2000;
}

/***************************************************/
/*          Get block from CPU address             */
/***************************************************/
int CpuBlock(int cpuadr)
{
    int mmublk = (cpuadr>>13) & 7;
    if (MMUregs.ActiveTask == 0) {
        return MMUregs.Task0[mmublk];
    } else {
        return MMUregs.Task1[mmublk];
    }
}

/***************************************************/
/*      Convert real address to cpu address        */
/***************************************************/
int RealToCpu(int realaddr)
{
    int offset = realaddr & 0x1FFF;
    int block = (unsigned) realaddr >> 13;
    int cpuadr;
    int i;

    for (i=0; i<8; i++) {
        if (MMUregs.ActiveTask == 0) {
            if (MMUregs.Task0[i] == block) break;
        } else {
            if (MMUregs.Task1[i] == block) break;
        }
    }
    if (i < 8) {
        cpuadr = i * 0x2000 + offset;
    } else {
        cpuadr = -1;
    }
    return cpuadr;
}

/*******************************************************/
/*          Highlight a disassembly line               */
/*******************************************************/
void HighlightLine(int lpos, COLORREF color, int flags) {

    // flags
    // bit 0 set make bold
    // bit 1 set make visible

    // Get current caret location and selection
    DWORD SelMin;
    DWORD SelMax;
    SendMessage(hDisText,EM_GETSEL,(WPARAM) &SelMin,(LPARAM) &SelMax);

    CHARFORMATA fmt;
    fmt.cbSize = sizeof(fmt);
    fmt.crTextColor = color;

    // Flags first bit make bold
    if (flags & 1) {
        fmt.dwEffects = CFE_BOLD;
    } else {
        fmt.dwEffects = 0;
    }
    fmt.dwMask = CFM_COLOR | CFM_BOLD;
    SendMessage(hDisText,EM_SETSEL,lpos,lpos+6);
    SendMessage(hDisText,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &fmt);

    // Flags second bit make visible
    if (flags & 2) {
        HWND f = GetFocus();
        SetFocus(hDisText);
        SendMessage(hDisText,EM_LINESCROLL,0,0);
        if (f) SetFocus(f);
    }

    // Restore original selection.
    SendMessage(hDisText,EM_SETSEL,(WPARAM) SelMin,(LPARAM) SelMax);
}

/*******************************************************/
/*       Find address match in disassembly             */
/*******************************************************/
int FindAdrLine(int adr)
{
    // If someone is bored this could be a binary search
    for (int line=0; line < NumDisLines; line++) {
        if (DisLineAdr[line] == adr) return line;
    }
    return MAXLINES;  // Indicates failure
}

/*******************************************************/
/*        Test for breakpoint at PC line               */
/*******************************************************/
bool BreakpointPC()
{
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        Haltpoint hp = it->second;
        if (hp.lpos == PClinePos) return true;
        it++;
    }
    return false;
}

/*******************************************************/
/*       Find and highlight all haltpoints             */
/*******************************************************/
void FindHaltpoints()
{
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        int adr = it->first;
        // If decoded with CPU addressing convert haltpoint to CPU address
        if (!RealAdrMode) adr = RealToCpu(adr);
        if (adr >= 0) {
            // Search for match in disassembly
            int line = FindAdrLine(adr);
            if (line < MAXLINES) {
                Haltpoint hp = it->second;
                hp.lpos = DisLinePos[line];
                HighlightLine(hp.lpos,RGB(255,0,0),1);
            }
        }
        it++;
    }
}

/**************************************************/
/*          Highlight the Current PC              */
/*          This gets called on timer             */
/**************************************************/
void HighlightPC()
{
    // Skip if CPU not halted
    if (! EmuState.Debugger.IsHalted()) {
        UnHighlightPC();
        return;
    }

    // Get the halted PC
    CPUState state = CPUGetState();
    int CPUadr = state.PC;

    // If PC already highlighted just return
    if (HighlightedPC == CPUadr) return;

    // Remove highlight from previous PC line
    UnHighlightPC();

    // If disassembly used real addresses convert PC to real
    int adr = RealAdrMode ? CpuToReal(CPUadr) : CPUadr;

    // Search for match in disassembly
    int line = FindAdrLine(adr);
    if (line < MAXLINES) {
        HighlightedPC = CPUadr;
        PClinePos = DisLinePos[line];
        if (BreakpointPC()) {
            HighlightLine(PClinePos,RGB(255,0,255),3); // Magneta
        } else {
            HighlightLine(PClinePos,RGB(0,0,255),3);   // Blue
        }

    // Not found disassemble using CPU mode starting from PC
    // PC should get painted on next timer event
    } else {
        SetWindowText(hEdtAddr,HEXSTR(CPUadr,0).c_str());
        RealAdrMode = false;
        EnableBlockEdit(FALSE);
        Button_SetCheck(GetDlgItem(hDismDlg,IDC_PHYS_MEM),BST_UNCHECKED);
        DecodeAddr();
        HighlightedPC = -1;
    }
}

/**************************************************/
/*   Remove PC highlight with haltpoint check     */
/**************************************************/
void UnHighlightPC()
{
    if (HighlightedPC >= 0) {
        HighlightedPC = -1;
        if (BreakpointPC()) {
            HighlightLine(PClinePos,RGB(255,0,0),1);
        } else {
            HighlightLine(PClinePos,RGB(0,0,0),0);
        }
    }
}

/***************************************************/
/*    Apply or Unapply a single Halt Point         */
/***************************************************/
void ApplyHaltPoint(Haltpoint &hp,bool flag)
{
    SectionLocker lock(Section_);
    if (flag) {
        if (!hp.placed) {
            // Be sure really not placed
            int instr = GetMem(hp.addr);
            if (instr != 0x15) {
                hp.instr = (unsigned char) instr;
                SetMem(hp.addr,0x15);
            }
            hp.placed = true;
        }
    } else {
        if (hp.placed) {
            SetMem(hp.addr, hp.instr);
            hp.placed = false;
        }
    }
    return;
}

/***************************************************/
/*      Delete Halt Point at real address          */
/***************************************************/
void RemoveHaltpoint(int realaddr)
{
    Haltpoint hp = mHaltpoints[realaddr];
    if (hp.exists) {
        HighlightLine(hp.lpos,RGB(0,0,0),0);
        ApplyHaltPoint(hp,false);
    }
    mHaltpoints.erase(realaddr);
}

/***************************************************/
/*       Set Halt Point at Current Line            */
/***************************************************/
void SetHaltpoint(bool flag)
{
HWND hDisText = NULL;  // Richedit20 box for output
    std::string s;
    Haltpoint hp;
    int realaddr;

    if ((CurrentLineNum < 0) || (CurrentLineNum >= MAXLINES))
        return;

    int lpos = DisLinePos[CurrentLineNum];
    int addr = DisLineAdr[CurrentLineNum];

    if (RealAdrMode)
        realaddr = addr;
    else
        realaddr = CpuToReal(addr);

    // Create or fetch existing haltpoint
    hp = mHaltpoints[realaddr];

    // Update Haltpoint
    if (flag) {
        hp.lpos = lpos;
        hp.exists = true;
        hp.addr = realaddr;
        ApplyHaltPoint(hp,true);
        mHaltpoints[realaddr] = hp;
        HighlightLine(lpos,RGB(255,0,0),1);
        EmuState.Debugger.Enable_Halt(true);

    // Remove Haltpoint
    } else {
        RemoveHaltpoint(realaddr);
    }

    // Refresh setpoints listbox
    RefreshHPlist();

    // Timer will apply PC highlight as required
    if (PClinePos == hp.lpos) UnHighlightPC();

    return;
}

/*******************************************************/
/*          Refresh haltpoints in ListBox              */
/*******************************************************/
void RefreshHPlist()
{
    if (hBrkpDlg == NULL) return;
    SendDlgItemMessage(hBrkpDlg,IDC_LIST_BREAKPOINTS,LB_RESETCONTENT,0,0);
    if (mHaltpoints.size() > 0) {
        std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
        while (it != mHaltpoints.end()) {
            int realaddr = it->first;
            Haltpoint hp = it->second;
            std::string s = HEXSTR(realaddr,6)+"\t "+HEXSTR(hp.instr,2);
            SendDlgItemMessage(hBrkpDlg, IDC_LIST_BREAKPOINTS,
                            LB_ADDSTRING, 0, (LPARAM) s.c_str());
            it++;
        }
    }
}

/*******************************************************/
/*          List haltpoints in DialogBox               */
/*******************************************************/
void ListHaltpoints()
{
    // Create breakpoints list dialog if required
    if (hBrkpDlg == NULL) {
        hBrkpDlg = CreateDialog ( hVccInst,
                              MAKEINTRESOURCE(IDD_BPLISTDIALOG),
                              hDismDlg, BreakpointsDlgProc );
        ShowWindow(hBrkpDlg, SW_SHOWNORMAL);
        SetFocus(hBrkpDlg);
    }
    RefreshHPlist();
    return;
}

/*******************************************************/
/*          Remove all haltpoints (public)             */
/*******************************************************/
void KillHaltpoints()
{
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        int realaddr = it->first;
        Haltpoint hp = it->second;
        ApplyHaltPoint(hp,false);
        HighlightLine(hp.lpos,RGB(0,0,0),0);
        mHaltpoints.erase(realaddr);
        it++;
    }
    UnHighlightPC();
    RefreshHPlist();
}

/*******************************************************/
/*           Apply all haltpoints (public)             */
/*     Install HALTs or restore original opcodes       */
/*******************************************************/
void ApplyHaltpoints(bool flag)
{
    // Iterate over all defined haltpoints
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        int realaddr = it->first;
        Haltpoint hp = it->second;
        ApplyHaltPoint(hp,flag);
        mHaltpoints[realaddr] = hp;
        it++;
    }
    return;
}

/*********************************************************************/
/* Find position of current line in disassembly string and set caret */
/*********************************************************************/
void SetCurrentLine()
{
    //TODO: This screws with scrolling

    // Ignore find if text is selected
    CHARRANGE range;
    SendMessage(hDisText,EM_EXGETSEL,0,(LPARAM) &range);
    if (range.cpMin!=range.cpMax) return;

    // Find selected line number
    int lnum = SendMessage(hDisText,EM_LINEFROMCHAR,range.cpMin,0);
    int lpos = DisLinePos[lnum];

    // If line number has changed set caret to start of line
    if (lnum != CurrentLineNum) {
        CurrentLineNum = lnum;
        SendMessage(hDisText,EM_SETSEL,lpos,lpos);
    }

//DEBUG
//int topline = SendMessage(hCtl,EM_GETFIRSTVISIBLELINE,0,0);
//PrintLogC("%d %d %X %d\n",lnum,DisLineAdr[lnum],lpos,topline);

}

/**************************************************/
/*  Get user specified address and disassemble    */
/**************************************************/
void DecodeAddr()
{
    char buf[16];
    unsigned short FromAdr = 0;
    unsigned short Block = 0;

    // Grab current MMUState
    MMUregs = GetMMUState();

    // Get user specified address and maybe block offset
    GetWindowText(hEdtAddr, buf, 8);
    int addr = HexToUint(buf);

    if (RealAdrMode) {
        // OpDecoder uses 16 bit PC so we have to use a Block number to
        // offset to the real address. If in OS9 mode the block number
        // input from the user, otherwise it is calculated.
        if (Os9Decode) {
            GetWindowText(hEdtBloc, buf, 8);
            int blk = HexToUint(buf);
            if ((blk < 0) || (blk > 0x3FF)) {
                SetErrorText("Invalid Block Number");
                return;
            }
            if (addr > 0xFFFF) {
                SetErrorText("Invalid Address");
                return;
            }
            Block = (unsigned short) blk;
            FromAdr = addr;
        } else {
            Block = (unsigned) addr >> 13;
            if (Block > 0x3FF) {
                SetErrorText("Invalid Address");
                return;
            }
            FromAdr = addr & 0x1FFF;
        }
    } else {
        // CPU addressing just check validity of address
        if (addr > 0xFFFF) {
            SetErrorText("Invalid Address");
            return;
        }
        FromAdr = addr;
    }

    Disassemble(FromAdr,Block);

    return;
}

/***************************************************/
/*   Right pad string to fixed size                */
/***************************************************/
std::string PadRight(std::string const& s, size_t w)
{
    int pad = w - s.size();
    if (pad > 0)
        return s + std::string(pad,' ');
    else
        return s;
}

/***************************************************/
/*           Format Disassembly line               */
/***************************************************/
std::string FmtLine(int adr,
                    std::string ins,
                    std::string opc,
                    std::string opr,
                    std::string cmt)
{
    DisLineAdr[NumDisLines++] = adr;
    std::string s;
    s = HEXSTR(adr,6) + " " + PadRight(ins,11) + PadRight(opc,5);
    if (cmt.size() > 0)
        s += PadRight(opr,10) + " " + cmt;
    else
        s += opr;
    return s + "\n";
}

/***************************************************/
/*        Create FDB line with comment             */
/***************************************************/
std::string OpFDB(int adr,std::string b1,std::string b2,std::string cmt)
{
    return FmtLine(adr,b1+b2,"FDB","$"+b1+b2,cmt);
}

/***************************************************/
/*        Create FCB line with comment             */
/***************************************************/
std::string OpFCB(int adr,std::string b,std::string cmt)
{
    return FmtLine(adr,b,"FCB","$"+b,cmt);
}

/***************************************************/
/*  Get byte from either real memory or cpu memory */
/***************************************************/
unsigned char GetCondMem(unsigned long addr) {
    if (RealAdrMode) {
        return (unsigned char) GetMem(addr);
    } else {
        return SafeMemRead8((unsigned short) addr);
    }
}

/***************************************************/
/* Test for and conditionally decode module header */
/***************************************************/
int DecodeModHdr(unsigned short Block, unsigned short PC, std::string *hdr)
{
    unsigned long addr = PC;
    if (RealAdrMode) addr += Block * 0x2000;

    // Convert header bytes to hex with check
    std::string hexb[13];
    int cksum = 0xFF;
    for (int n=0; n<9; n++) {
        int ch = GetCondMem(n+addr);
        if ((n == 0) && (ch != 0x87)) break;
        if ((n == 1) && (ch != 0xCD)) break;
        hexb[n] = HEXSTR(ch,2);
        cksum = cksum ^ ch;
    }
    if (cksum) return 0;

    // Generate header lines
    int cnt = 0;
    *hdr =  OpFDB(addr+cnt++,hexb[0],hexb[1],"Module"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[2],hexb[3],"Mod Siz"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[4],hexb[5],"Off Nam"); cnt++;
    *hdr += OpFCB(addr+cnt++,hexb[6],"Ty/Lg");
    *hdr += OpFCB(addr+cnt++,hexb[7],"At/Rv");
    *hdr += OpFCB(addr+cnt++,hexb[8],"Parity");

    // Additional lines if executable
    char type = hexb[6][0];
    if ((type=='1')||(type=='E')||(type=='C')) {
        for (int n=0; n<4; n++) {
            int ch = GetCondMem(cnt+n+addr);
            hexb[n+9] = HEXSTR(ch,2);
        }
        *hdr += OpFDB(addr+cnt++,hexb[9],hexb[10],"Off Exe"); cnt++;
        *hdr += OpFDB(addr+cnt++,hexb[11],hexb[12],"Dat Siz"); cnt++;
    }
    return cnt; // Num bytes decoded
}

/**************************************************/
/*  Disassemble Instructions in specified range   */
/**************************************************/
void Disassemble( unsigned short FromAdr,
                  unsigned short Block)
{
    std::unique_ptr<Debugger::OpDecoder> Decoder;
    Decoder = std::make_unique<Debugger::OpDecoder>();
    VCC::CPUTrace trace = {};
    VCC::CPUState state = {};

    sDecoded = {};
    SetWindowTextA(hDisText,"");

    unsigned short PC = FromAdr;
    NumDisLines = 0;
    bool modhdr = false;

    state.block = Block;
    state.phyAddr = RealAdrMode;
    while (NumDisLines < MAXLINES) {

        if (Os9Decode) {
            // Check for module header
            std::string hdr;
            int hdrlen = DecodeModHdr(Block, PC, &hdr);
            if (hdrlen) {
                modhdr = true;
                PC += hdrlen;
            }
            sDecoded += hdr;
        }

        state.PC = PC;
        trace = {};
        Decoder->DecodeInstruction(state,trace);

        // If decoding for Os9 convert SWI2 to OS9 with immediate operand
        if (Os9Decode && (trace.instruction == "SWI2")) {
            unsigned short opadr = PC + (unsigned short) trace.bytes.size();
            unsigned char op = Debugger::DbgRead8(RealAdrMode, Block, opadr);
            trace.instruction = "OS9";
            trace.operand = "#$"+HEXSTR(op,2);
            trace.bytes.push_back(op);
        }

        // Instruction address
        int iaddr;
        if (RealAdrMode)
            iaddr = PC + Block * 0x2000;
        else
            iaddr = PC;

        // Create string of instruction bytes
        std::string HexInst;
        for (unsigned int i=0; i < trace.bytes.size(); i++) {
            HexInst += HEXSTR(trace.bytes[i],2);
        }

        // String of ascii instruction bytes
        std::string comment;
        for (unsigned int i=0; i < trace.bytes.size(); i++) {
            char c = trace.bytes[i] & 0x7f;
            if (c < 32 || c > 126) c = '.';
            comment += c;
        }

        // append disassembled line for output
        sDecoded += FmtLine(iaddr,HexInst,trace.instruction,trace.operand,comment);

        // Next instruction
        if (trace.bytes.size() > 0)
            PC += (UINT16) trace.bytes.size();
        else
            PC += 1;
    }
    // Put the completed disassembly
    SetWindowTextA(hDisText,Debugger::ToLPCSTR(sDecoded));

    // Find line positions
    for (int lnum=0;lnum < NumDisLines;lnum++) {
        DisLinePos[lnum] = SendMessage(hDisText,EM_LINEINDEX,lnum,0);
    }

    // Highlight any haltpoints
    FindHaltpoints();

    // No line selected
    CurrentLineNum = -1;

    // No PC highlighted
    HighlightedPC = -1;

    ClrErrorText();
}

/**************************************************/
/*        Convert hex digits to Address           */
/**************************************************/
int HexToUint(char * buf)
{
    long val;
    char *eptr;
    if (*buf == '\0')  return 0;
    val = strtol(buf,&eptr,16);
    if (*eptr != '\0') return -1;
    if (val < 0)       return -1;
    if (val > 0xFFFFFF)  return -1;
    return val & 0xFFFFFF;
}

}// Namespace ends
