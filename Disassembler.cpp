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

CriticalSection Section_;

// Dialog and control handles
HWND hDismDlg = NULL;  // Disassembler Dialog
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtBloc = NULL;  // Bock number editbox
HWND hEdtAPPY = NULL;  // Apply button
HWND hDisText = NULL;  // Richedit20 box for output
HWND hErrText = NULL;  // Error text box

// Address mode of disassembly
enum AddrMode { ADR_NULL, ADR_CPU, ADR_REAL, ADR_BLOCK };
int PrevAdrMode = ADR_NULL;
bool RealAdrMode = FALSE;

// local references
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
int RealBlock(int);
int CpuBlock(int);
int CpuToReal(int); 
int RealToCpu(int); 
void DecodeAddr();
void Disassemble(unsigned short, unsigned short);
void ReCalcAddress();
int HexToUint(char *);
void EnableBlockEdit(bool);

std::string PadRight(std::string const&,size_t);
std::string OpFDB(int,std::string,std::string,std::string);
std::string OpFCB(int,std::string,std::string);
std::string FmtLine(int,std::string,std::string,std::string,std::string);

int DecodeModHdr(unsigned short, unsigned short, std::string *mhdr);

int FindLineNdx(HWND);
void SetHaltpoint(HWND,bool);
void ListHaltpoints();
void FindHaltpoints();
void HighlightPC();
void UnHighlightPC();
void HighlightLine(int,COLORREF,bool);

// Static local variables
bool UsePhyAdr = FALSE;
bool Os9Decode = FALSE;

int LastPC = 0xFFFFFF;
int PCline = std::string::npos;

// MMUregs at start of last decode
//MMUState MMUregs;

char initTxt[] = "Address and Block are hexadecimal";

// Disassembled code goes into string here
std::string sDecoded = {};

// mHaltpoints container.
// A new style breakpoint is named 'Haltpoint' to avoid conflict
// with the structure used by the source code debugger. The real
// address and instruction at the breakpoint are saved. Haltpoints
// are stored in Haltpoints std::map container. Haltpoints
// are placed before the cpu enters run state and are removed
// when the cpu is halted.
struct Haltpoint
{
    int lndx;
    int addr;
    unsigned char instr;
    bool placed;
};
std::map<int,Haltpoint> mHaltpoints{};
void ApplyHaltPoint(Haltpoint &,bool);

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

/**************************************************/
/*      Create Disassembler Dialog Window         */
/**************************************************/
void
OpenDisassemblerWindow(HINSTANCE hInstance, HWND hParent)
{
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
        SetWindowTextA(hEdtAddr,"0");
        SetWindowTextA(hEdtBloc,"0");
        SetWindowTextA(hErrText,initTxt);

        // Disable Block text box until needed
        EnableBlockEdit(FALSE);

        // Start a timer for showing current status
		SetTimer(hDlg, IDT_BRKP_TIMER, 250, (TIMERPROC)NULL);
        break;

    case WM_PAINT:
        // Give edit control focus 
        SetFocus(hEdtAddr);
        SendMessage(hEdtAddr,EM_SETSEL,0,-1); //Select contents
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            ApplyHaltpoints(false); // Remove haltpoints
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
                EnableBlockEdit(UsePhyAdr);
            } else {
                Os9Decode = FALSE;
                EnableBlockEdit(FALSE);
            }
            return TRUE;
        case IDC_PHYS_MEM:
            if (IsDlgButtonChecked(hDlg,IDC_PHYS_MEM)==BST_CHECKED) {
                UsePhyAdr = TRUE;
                EnableBlockEdit(Os9Decode);
            } else {
                UsePhyAdr = FALSE;
                EnableBlockEdit(FALSE);
            }
            return TRUE;
        case IDC_RESET:
            ReCalcAddress();
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wPrm == IDT_BRKP_TIMER) {
            //if (EmuState.Debugger.IsHalted()) HighlightPC();
            HighlightPC();
            return TRUE;
        }
        break;

    }
    return FALSE;  // unhandled message
}

/***************************************************/
/*        Process edit control messages            */
/***************************************************/
LRESULT CALLBACK SubEditDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    HWND nxtctl;
    switch (msg) {
    case WM_CHAR:
        switch (toupper(wPrm)) {
        // Hot keys for disassembly text window
        case 'G':  // Toggle run
        case 'P':  // Toggle run
        case 'N':  // Next
        case 'S':  // Set haltpoint
        case 'R':  // Remove haltpoint
        case 'L':  // List haltpoints
            SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
        // Enter does the disassembly
        case VK_RETURN:
            DecodeAddr();
            return TRUE;
        // Tab moves between active edit boxes
        case VK_TAB:
            SetWindowTextA(hErrText,initTxt);
            if (hCtl==hEdtAddr && UsePhyAdr && Os9Decode) {
                nxtctl = hEdtBloc;
            } else {
                nxtctl = hEdtAddr;
            }
            SetFocus(nxtctl);
            SendMessage(nxtctl,EM_SETSEL,0,-1); //Select contents
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
/*    Process Disassembly text window messages     */
/***************************************************/
LRESULT CALLBACK SubTextDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_CHAR:
        switch (toupper(wPrm)) {
        // Create Haltpoint
        case 'S':
            EmuState.Debugger.Enable_Halt(true);  //TODO: enable just once?
            SetHaltpoint(hCtl,true);
            return TRUE;
            break;
        // Remove Haltpoint
        case 'R':
            SetHaltpoint(hCtl,false);
            return TRUE;
            break;
        // List haltpoints
        case 'L':
            ListHaltpoints();
            return TRUE;
            break;
        // Toggle pause
        case 'G':
        case 'P':
            if (EmuState.Debugger.IsHalted()) {
			    PauseAudio(0);
			    EmuState.Debugger.QueueRun();
            } else {
			    EmuState.Debugger.QueueHalt();
			    PauseAudio(1);
            }
            return TRUE;
            break;
        // Step Next
        case 'N':
            EmuState.Debugger.QueueStep();
            return TRUE;
            break;
        case VK_TAB:
            SetFocus(hEdtAddr);
            return TRUE;
            break;
        }
        break;

    // Use FindLineNdx to position caret to start of current line
    case WM_PAINT:
    case WM_LBUTTONUP:
        FindLineNdx(hCtl);
        break;
    case WM_KEYUP:
        switch (wPrm) {
        case VK_PRIOR:
        case VK_NEXT:
        case VK_DOWN:
        case VK_UP:
            FindLineNdx(hCtl);
            break;
        }
        break;
    }

    // Forward messages to original control
    return CallWindowProc(TextDlgProc,hCtl,msg,wPrm,lPrm);
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
/*       Recalc Address from previous mode         */
/***************************************************/
void ReCalcAddress()
{
    // current address mode
    int NewAdrMode;
    if (!IsDlgButtonChecked(hDismDlg,IDC_PHYS_MEM)==BST_CHECKED)
        NewAdrMode = ADR_CPU;
    else if (IsDlgButtonChecked(hDismDlg,IDC_BTN_OS9)==BST_CHECKED)
        NewAdrMode = ADR_BLOCK;
    else
        NewAdrMode = ADR_REAL;

    // current address and block
    char buf[16];
    GetWindowText(hEdtAddr, buf, 8);
    int addr = HexToUint(buf);
    GetWindowTextA(hEdtBloc,buf, 8);
    int block = HexToUint(buf);
    int tmpadr;

    // Compare previous Address Modes
    switch (PrevAdrMode) {
    case ADR_CPU:
        switch (NewAdrMode) {
        case ADR_REAL:                  // Recalc CPU to Real
            addr = CpuToReal(addr);
            break;
        case ADR_BLOCK:                 // Recalc CPU to Block
            block = CpuBlock(addr);
            addr = addr & 0x1FFF;
            break;
        }
        break;
    case ADR_REAL:
        switch (NewAdrMode) {
        case ADR_CPU:                   // Recalc Real to CPU
            tmpadr = RealToCpu(addr);
            if (tmpadr < 0) {
                SetWindowTextA(hErrText,"Address not Mapped by MMU");
                Button_SetCheck(GetDlgItem(hDismDlg,IDC_PHYS_MEM),BST_CHECKED);
                UsePhyAdr = TRUE;
                NewAdrMode = ADR_REAL;
            } else {
                addr = tmpadr;
            }
            break;
        case ADR_BLOCK:                 // Recalc Real to Block
            block = (addr >> 13);
            addr = addr & 0x1FFF;
            break;
        }
        break;
    case ADR_BLOCK:
        switch (NewAdrMode) {
        case ADR_REAL:                  // Recalc Block to Real
            addr += block * 0x2000;
            break;
        case ADR_CPU:                   // Recalc Block to CPU
            tmpadr = RealToCpu(addr+block*0x2000);
            if (tmpadr < 0) {
                SetWindowTextA(hErrText,"Address not Mapped by MMU");
                Button_SetCheck(GetDlgItem(hDismDlg,IDC_PHYS_MEM),BST_CHECKED);
                EnableWindow(hEdtBloc,TRUE);
                UsePhyAdr = TRUE;
                NewAdrMode = ADR_BLOCK;
            } else {
                addr = tmpadr;
            }
            break;
        }
        break;
    }
    // Put updated address and block
    SetWindowText(hEdtAddr,HEXSTR(addr,0).c_str());
    SetWindowTextA(hEdtBloc,HEXSTR(block,0).c_str());
    if (PrevAdrMode != NewAdrMode) DecodeAddr();
    return;
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
    MMUState regs = GetMMUState();
    if (regs.ActiveTask == 0) {
        return regs.Task0[(cpuadr >> 13) & 7];
    } else {
        return regs.Task1[(cpuadr >> 13) & 7];
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

    MMUState regs = GetMMUState();
    for (i=0; i<8; i++) {
        if (regs.ActiveTask == 0) {
            if (regs.Task0[i] == block) break;
        } else {
            if (regs.Task1[i] == block) break;
        }
    }
    if (i < 8) {
        cpuadr = i * 0x2000 + offset;
    } else {
        cpuadr = -1;
    }
    return cpuadr;
}

/**************************************************/
/*          Highlight the Current PC              */
/**************************************************/
void HighlightPC()
{
    // Disassembly has to have a least one line
    if (sDecoded.find('\n') == std::string::npos) return;

    int adr;
    CPUState state = CPUGetState();
    if (RealAdrMode) {
        adr = CpuToReal(state.PC);
    } else {
        adr = state.PC;
    }

    // If PC already highlighted just return
    if (adr == LastPC) return;

    // Remove highlight from previous PC line
    UnHighlightPC();

    // Search for PC line in disassembly.
    std::string sAdr = HEXSTR(adr,6);
    PCline = sDecoded.find('\n'+ sAdr);
    if (PCline != std::string::npos) {
        PCline += 1;
    } else if (sAdr.compare(sDecoded.substr(0,6)) == 0) {
        PCline = 0;
    } else {
        PCline = std::string::npos;
    }

    if (PCline != std::string::npos) {
        HighlightLine(PCline,RGB(0,0,255),true); // Blue

        // Make visible in display
        HWND f = GetFocus();
        SetFocus(hDisText);
        SendMessage(hDisText,EM_LINESCROLL,0,0);
        if (f) SetFocus(f);

        LastPC = adr;
    } else {
        LastPC = 0xFFFFFF;
    }
}

/**************************************************/
/*   Remove PC highlight with haltpoint check     */
/**************************************************/
void UnHighlightPC()
{
    if (PCline == std::string::npos) return;
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        Haltpoint hp = it->second;
        // if was a breakpoint highlight it red
        if (hp.lndx == PCline) {
            HighlightLine(PCline,RGB(255,0,0),true);
            return;
        }
        it++;
    }
    // Clear highlight
    HighlightLine(PCline,RGB(0,0,0),false);
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
/*       Set Halt Point at Current Line            */
/***************************************************/
void SetHaltpoint(HWND hCtl, bool flag)
{
    std::string s;
    Haltpoint hp;
    int realaddr;
                                         
    // The line index is used to get the haltpoint address
    int LineNdx = FindLineNdx(hCtl);
    if (LineNdx<0) return;

    // Get the address of the instruction
    s = sDecoded.substr(LineNdx,6);
    int addr = stoi(s,0,16);
    if (RealAdrMode) 
        realaddr = addr;
    else
        realaddr = CpuToReal(addr);

    // Create or fetch existing haltpoint
    hp = mHaltpoints[realaddr];

    // Update Haltpoint
    if (flag) {
        hp.lndx = LineNdx;
        hp.addr = realaddr;
        ApplyHaltPoint(hp,true);
        mHaltpoints[realaddr] = hp;
        HighlightLine(LineNdx,RGB(255,0,0),true);

    // Remove Haltpoint
    } else {
        ApplyHaltPoint(hp,false);
        mHaltpoints.erase(realaddr);
        HighlightLine(LineNdx,RGB(0,0,0),false);
    }

    //PrintLogC("%X %X %s\n",hp.addr,hp.instr,hp.placed);
    return;
}

/*******************************************************/
/*          List haltpoints in MessageBox              */
/*******************************************************/
void ListHaltpoints()
{
    if (mHaltpoints.size() > 0) {
        std::string s = "";
        std::map<int, Haltpoint>::iterator it;
        it = mHaltpoints.begin();
        while (it != mHaltpoints.end()) {
            int realaddr = it->first;
            Haltpoint hp = it->second;
            s += HEXSTR(realaddr,4)+" "+HEXSTR(hp.instr,2);
            if (hp.placed) 
                s += " p\n";
            else
                s += "\n";
            it++;
        }
        MessageBox(hDismDlg,s.c_str(),"Haltpoints",0);
    } else {
        MessageBox(hDismDlg,"No haltpoints","Haltpoints",0);
    }
    return;
}

/*******************************************************/
/*           Apply haltpoints (exported)               */
/*     Install HALTs or restore original opcodes       */
/*******************************************************/
void ApplyHaltpoints(bool flag)
{

//    if (flag) EmuState.Debugger.Enable_Halt(true);  //TODO: enable once?

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

/*******************************************************/
/*   Highlight address field in disassembly line       */
/*******************************************************/
void HighlightLine(int LineNdx,COLORREF color,bool bold) {
    CHARFORMATA fmt;
    fmt.cbSize = sizeof(fmt);
    fmt.crTextColor = color;
    if (bold) {
        fmt.dwEffects = CFE_BOLD;
    } else {
        fmt.dwEffects = 0;
    }
    fmt.dwMask = CFM_COLOR | CFM_BOLD;
    SendMessage(hDisText,EM_SETSEL,LineNdx,LineNdx+6);
    SendMessage(hDisText,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &fmt);
    SendMessage(hDisText,EM_SETSEL,LineNdx,LineNdx);
}

/*******************************************************/
/*       Find and highlight all haltpoints             */
/*******************************************************/
// Called after redrawing the disassembly text
void FindHaltpoints() {
    std::string s;
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        int adr = it->first;
        if (!RealAdrMode) adr = RealToCpu(adr);
        if (!(adr < 0)) {
            Haltpoint hp = it->second;
            s = HEXSTR(adr,6);
            // Search newline followed by address
            int pos = sDecoded.find("\n"+s);
            if (pos != std::string::npos) {
                hp.lndx = pos+1;
                mHaltpoints[adr] = hp;
                HighlightLine(pos+1,RGB(255,0,0),true);
            // Check first line
            } else if (s == sDecoded.substr(0,6)) {
                hp.lndx = 0;
                mHaltpoints[adr] = hp;
                HighlightLine(0,RGB(255,0,0),true);
            }
        }
        it++;
    }
}

/*********************************************************************/
/* Find position of current line in disassambly string and set caret */
/*********************************************************************/
int FindLineNdx(HWND hCtl)
{

    // Ignore find if text is selected
    CHARRANGE range;
    SendMessage(hCtl,EM_EXGETSEL,0,(LPARAM) &range);
    if (range.cpMin!=range.cpMax) return -1;
    
    // Find line number and start position
    int lnum = SendMessage(hCtl,EM_LINEFROMCHAR,range.cpMin,0);
    int lndx = SendMessage(hCtl,EM_LINEINDEX,lnum,0);

    // Set caret to start of line
    SendMessage(hCtl,EM_SETSEL,lndx,lndx);

    return lndx;
}

/**************************************************/
/*  Get user specified address and disassemble    */
/**************************************************/
void DecodeAddr()
{
    char buf[16];
    unsigned short FromAdr = 0;
    unsigned short Block = 0;


    // Get user specified address and maybe block offset
    GetWindowText(hEdtAddr, buf, 8);
    int addr = HexToUint(buf);

    // OpDecoder uses a 16 bit PC so we have to use a Block number to
    // offset to the real address. If OS9 mode the block number is
    // input from the user, otherwise it is calculated.
    if (UsePhyAdr) {
        if (Os9Decode) {
            GetWindowText(hEdtBloc, buf, 8);
            int blk = HexToUint(buf);
            if ((blk < 0) || (blk > 0x3FF)) {
                SetWindowText(hErrText,"Invalid Block Number");
                return;
            }
            Block = (unsigned short) blk;
            FromAdr = addr;
            PrevAdrMode = ADR_BLOCK;
        } else {
            Block = (unsigned) addr >> 13;
            FromAdr = addr & 0x1FFF;
            PrevAdrMode = ADR_REAL;
        }
    } else {
        FromAdr = addr;
        // If not using physical addressing disallow access I/O ports
        if (FromAdr >= 0xFF00) { 
            SetWindowText(hErrText,"Invalid start address");
            return;
        }
        PrevAdrMode = ADR_CPU;
    }

    //RealAdrMode controls Disassemble addressing
    RealAdrMode = UsePhyAdr;

    Disassemble(FromAdr,Block);
    FindHaltpoints();
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
    int numlines = 0;
    bool modhdr = false;

    state.block = Block;
    state.phyAddr = RealAdrMode;
    while (numlines < MAXLINES) {

        if (Os9Decode) {
            // Check for module header
            std::string hdr;
            int hdrlen = DecodeModHdr(Block, PC, &hdr);
            if (hdrlen) {
                modhdr = true;
                PC += hdrlen;
                if (hdrlen > 9)
                    numlines += 8;
                else
                    numlines += 6;
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
        numlines++;

        // Next instruction
        if (trace.bytes.size() > 0)
            PC += (UINT16) trace.bytes.size();
        else
            PC += 1;
    }
    // Put the completed disassembly
    SetWindowTextA(hDisText,Debugger::ToLPCSTR(sDecoded));
    SetWindowTextA(hErrText,initTxt);
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
