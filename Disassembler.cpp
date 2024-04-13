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

#define HEXSTR(i,w) Debugger::ToHexString(i,w,true)

namespace VCC {

// Dialog and control handles
HWND hDismWin = NULL;  // Set when window created
HWND hDismDlg = NULL;  // Set when dialog activated
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtLcnt = NULL;  // Number of lines
HWND hEdtBloc = NULL;  // Bock number editbox
HWND hEdtAPPY = NULL;  // Apply button
HWND hDisText = NULL;  // Richedit20 box for output
HWND hErrText = NULL;  // Error text box

// local references
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
int RealAddr(int);
int RealBlock(int);
int CpuAddr(int);
void DecodeRange();
void Disassemble(unsigned short, unsigned short, unsigned short);
void MapAdrToReal();
void MapRealToAdr();
UINT16 HexToUint(char *);

std::string PadRight(std::string const&,size_t);
std::string OpFDB(int,std::string,std::string,std::string);
std::string OpFCB(int,std::string,std::string);
std::string FmtLine(int,std::string,std::string,std::string,std::string);

int DecodeModHdr(unsigned short, unsigned short, std::string *mhdr);

int GetLineNdx(HWND);
void SetHaltpoint(HWND,bool);
void ColorizeHaltpoint(int,bool);
void ColorizeAllHaltpoints();

bool UsePhyAdr = FALSE;
bool Os9Decode = FALSE;
bool DisTxtRealAdr = FALSE;

// MMUregs at start of last decode
MMUState MMUregs;

char initTxt[] = "Address and Block are hex.  Lines decimal";

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
    int realaddr;
    unsigned char instr;
    bool placed;
};
std::map<int,Haltpoint> mHaltpoints{};

// Functions used to subclass controls
LONG_PTR SetControlHook(HWND,LONG_PTR);
LRESULT CALLBACK SubAddrDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK SubBlocDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK SubLcntDlgProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK SubTextDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
WNDPROC BlocDlgProc;
WNDPROC LcntDlgProc;
WNDPROC TextDlgProc;
// Handler for address and count edit boxes
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);
// Handler for disassembly text richedit control
BOOL ProcTextDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);

/**************************************************/
/*       Create Disassembler Dialog Window        */
/**************************************************/
void
OpenDisassemblerWindow(HINSTANCE instance, HWND parent)
{
    if (hDismWin == NULL) {
        hDismWin = CreateDialog
            ( instance, MAKEINTRESOURCE(IDD_DISASSEMBLER),
              parent, DisassemblerDlgProc );
        ShowWindow(hDismWin, SW_SHOWNORMAL);
    }
    SetFocus(hDismWin);
}

/**************************************************/
/*        Disassembler Dialog Handler             */
/**************************************************/
INT_PTR CALLBACK DisassemblerDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_INITDIALOG:

        // Grab dialog and control handles
        hDismDlg = hDlg;
        hEdtAddr = GetDlgItem(hDismDlg, IDC_EDIT_PC_ADDR);
        hEdtBloc = GetDlgItem(hDismDlg, IDC_EDIT_BLOCK);
        hEdtLcnt = GetDlgItem(hDismDlg, IDC_EDIT_PC_LCNT);
        hEdtAPPY = GetDlgItem(hDismDlg, IDAPPLY);
        hDisText = GetDlgItem(hDismDlg, IDC_DISASSEMBLY_TEXT);
        hErrText = GetDlgItem(hDismDlg, IDC_ERROR_TEXT);

        // Hook the input dialogs to capture keystrokes
        AddrDlgProc = (WNDPROC) SetControlHook(hEdtAddr,(LONG_PTR) SubAddrDlgProc);
        BlocDlgProc = (WNDPROC) SetControlHook(hEdtBloc,(LONG_PTR) SubBlocDlgProc);
        LcntDlgProc = (WNDPROC) SetControlHook(hEdtLcnt,(LONG_PTR) SubLcntDlgProc);
        // Hook the Disasembly text
        TextDlgProc = (WNDPROC) SetControlHook(hDisText,(LONG_PTR) SubTextDlgProc);

        // Set Consolas font (fixed) in disassembly edit box
        CHARFORMAT disfmt;
        disfmt.cbSize = sizeof(disfmt);
        SendMessage(hDisText,EM_GETCHARFORMAT,(WPARAM) SCF_DEFAULT,(LPARAM) &disfmt);
        strcpy(disfmt.szFaceName,"Consolas");
        disfmt.yHeight=180;
        SendMessage(hDisText,EM_SETCHARFORMAT,(WPARAM) SCF_DEFAULT,(LPARAM) &disfmt);

        // Inital values in edit boxes
        SetWindowText(hEdtLcnt, "500");
        SetWindowTextA(hDisText,"");
        SetWindowTextA(hErrText,initTxt);

        // Disable Block text box until phys mem is checked
        EnableWindow(hEdtBloc,FALSE);
        break;

    case WM_PAINT:
        // Give edit control focus to capture tab/enter/up/down etc
        SetFocus(hEdtAddr);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            // Make sure no haltpoints are still placed.
            ApplyHaltpoints(false);
            DestroyWindow(hDlg);
            hDismWin = NULL;
            return FALSE;
        case IDAPPLY:
            DecodeRange();
            SetFocus(hEdtAddr);
            return TRUE;
        case IDC_BTN_OS9:
            if (IsDlgButtonChecked(hDismDlg,IDC_BTN_OS9))
                Os9Decode = TRUE;
            else
                Os9Decode = FALSE;
            SetFocus(hEdtAddr);
            return TRUE;
        case IDC_PHYS_MEM:
            if (IsDlgButtonChecked(hDlg,IDC_PHYS_MEM)) {
                MapAdrToReal();
            } else {
                MapRealToAdr();
            }
            SetFocus(hEdtAddr);
            return TRUE;
        }
    }
    return FALSE;  // unhandled message
}

/***************************************************/
/*              Set up control hooks               */
/***************************************************/
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR HookProc)
{
    LONG_PTR OrigProc=GetWindowLongPtr(hCtl,GWLP_WNDPROC);
    SetWindowLongPtr(hCtl,GWLP_WNDPROC,HookProc);
    return OrigProc;
}

/***************************************************/
/*     Process Disassembly text messages here      */
/***************************************************/
LRESULT CALLBACK SubTextDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcTextDlg(TextDlgProc,hCtl,msg,wPrm,lPrm); }

inline std::string HPSTR(bool b) {
  return b ? "Placed" : "Not Placed";
}

BOOL ProcTextDlg (WNDPROC pCtl,HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    std::string s;
    std::map<int, Haltpoint>::iterator it;
    switch (msg) {
    // Set or Clear Breakpoint at current line
    case WM_CHAR:
        switch (wPrm) {
        // Haltpoint
        case 'H':
        case 'h':
            SetHaltpoint(hCtl,true);
            return TRUE;
            break;
        // Remove Haltpoint
        case 'R':
        case 'r':
            SetHaltpoint(hCtl,false);
            return TRUE;
            break;
        //Status
        case 'S':
        case 's':
            if (mHaltpoints.size() > 0) {
                s = "";
                it = mHaltpoints.begin();
                while (it != mHaltpoints.end())
                {
                    int realaddr = it->first;
                    Haltpoint hp = it->second;
                    s += HEXSTR(realaddr,4)+" "+HPSTR(hp.placed)+"\n";
                    it++;
                }
                MessageBox(hDismWin,s.c_str(),"Haltpoints",0);
            } else {
                MessageBox(hDismWin,"No haltpoints","Haltpoints",0);
            }
            return TRUE;

// Debug
case 'P':
case 'p':
  ApplyHaltpoints(true);
  return TRUE;
  break;
case 'U':
case 'u':
  ApplyHaltpoints(false);
  return TRUE;
  break;
// Debug ends

        }
        break;
    // Use GetLineNdx to position caret to start of current line
    case WM_PAINT:
    case WM_LBUTTONUP:
        GetLineNdx(hCtl);
        break;
    case WM_KEYUP:
        switch (wPrm) {
        case VK_PRIOR:
        case VK_NEXT:
        case VK_DOWN:
        case VK_UP:
            GetLineNdx(hCtl);
            break;
        }
    }

    // Forward messages to original control
    return CallWindowProc(pCtl,hCtl,msg,wPrm,lPrm);
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
    int LineNdx = GetLineNdx(hCtl);
    if (LineNdx<0) return;

    // Get the real address of the instruction
    s = sDecoded.substr(LineNdx,6);
    int addr = stoi(s,0,16);
    realaddr = RealAddr(addr);

    // Create a Haltpoint
    if (flag) {
        // Convert instruction byte to unsigned char
        s = sDecoded.substr(LineNdx+7,2);
        //hp.lndx = LineNdx;
        hp.realaddr = realaddr;
        hp.instr = stoi(s,0,16);
        hp.placed = false;
        mHaltpoints[realaddr] = hp;
        ColorizeHaltpoint(LineNdx,true);
        return;
    // Remove Haltpoint
    } else {
        hp = mHaltpoints[realaddr];
        // If it is placed restore original opcode first
        if (hp.placed) SetMem(realaddr, hp.instr);
        // Remove haltpoint
        mHaltpoints.erase(realaddr);
        ColorizeHaltpoint(LineNdx,false);
    }
    return;
}

/*******************************************************/
/*                Colorize Haltpoint                   */
/*******************************************************/
void ColorizeHaltpoint(int LineNdx,bool flag)
{
    CHARFORMATA fmt;
    fmt.cbSize = sizeof(fmt);
    if (flag) {
        // Address text bold red
        fmt.crTextColor = RGB(255,0,0); // Red
        fmt.dwEffects = CFE_BOLD;
    } else {
        // Address text black
        fmt.crTextColor = RGB(0,0,0);
        fmt.dwEffects = 0;
    }
    // Set color of address field
    fmt.dwMask = CFM_COLOR | CFM_BOLD;
    SendMessage(hDisText,EM_SETSEL,LineNdx,LineNdx+6);
    SendMessage(hDisText,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &fmt);
    SendMessage(hDisText,EM_SETSEL,LineNdx,LineNdx);
}

/*******************************************************/
/*            Colorize All Haltpoints                  */
/*******************************************************/
// Called after redrawing the disassembly text
void ColorizeAllHaltpoints() {
    std::string s;
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();
    while (it != mHaltpoints.end()) {
        int adr = it->first;
        if (!DisTxtRealAdr) adr = CpuAddr(adr);
        if (adr != 0) {
            Haltpoint hp = it->second;
            s = HEXSTR(adr,6) + " " + HEXSTR(hp.instr,2);
            int pos = sDecoded.find(s);
            if (pos != std::string::npos) {
                ColorizeHaltpoint(pos,true);
            }
        }
        it++;
    }
}

/*******************************************************/
/*               Apply haltpoints                      */
/*******************************************************/
void ApplyHaltpoints(bool flag)
{
    unsigned char instr;

    if (flag) EmuState.Debugger.Enable_Halt(true);

    // Either install halt instruction or restore original opcode
    std::map<int, Haltpoint>::iterator it = mHaltpoints.begin();

    while (it != mHaltpoints.end()) {

        int realaddr = it->first;
        Haltpoint hp = it->second;
//PrintLogC("apply %x %d\n",realaddr,flag);

        // Get instruction currently at haltpoint
        instr = (unsigned char) GetMem(hp.realaddr);

        if (flag) {
            // Check that it is not already placed
            if (!hp.placed) {
                if (instr != 0x15) {
                    hp.instr = instr;
                    hp.placed = true;
                    mHaltpoints[realaddr] = hp;
                    SetMem(realaddr, 0x15);
                } else {
                    MessageBox(hDismWin,"Breakpoints placement Failed!","Error",0);
                    PrintLogC("place failed\n");
                }
            } // else already placed
        } else {
            if (hp.placed) {
                // Check that it was actually placed
                if (instr == 0x15) {
                    SetMem(realaddr, hp.instr);
                    hp.placed = false;
                    mHaltpoints[realaddr] = hp;
                } else {
                    MessageBox(hDismWin,"Breakpoints removal Failed!","Error",0);
                }
            } // else was not placeed
        }
        it++;
    }
    return;
}

/*******************************************************/
/* Find current line start index in disassambly string */
/*******************************************************/
int GetLineNdx(HWND hCtl)
{
    int lndx;
    CHARRANGE range;
    SendMessage(hCtl,EM_EXGETSEL,0,(LPARAM) &range);
    // Return index if no text is selected
    if (range.cpMin==range.cpMax) {
        // Find line number
        int lnum = SendMessage(hCtl,EM_LINEFROMCHAR,range.cpMin,0);
        // Find line start index
        lndx = SendMessage(hCtl,EM_LINEINDEX,lnum,0);
        // Set caret to start of line
        SendMessage(hCtl,EM_SETSEL,lndx,lndx);
    // Return error
    } else {
        lndx = -1;
    }
    return lndx;
}

/***************************************************/
/*      Process edit control messages here         */
/***************************************************/
LRESULT CALLBACK SubAddrDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(AddrDlgProc,hCtl,msg,wPrm,lPrm); }
LRESULT CALLBACK SubBlocDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(BlocDlgProc,hCtl,msg,wPrm,lPrm); }
LRESULT CALLBACK SubLcntDlgProc(HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(LcntDlgProc,hCtl,msg,wPrm,lPrm); }

BOOL ProcEditDlg (WNDPROC pCtl,HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    HWND nxtctl;
    switch (msg) {
    case WM_CHAR:
        switch (wPrm) {
        // Hot keys for manageing haltpoints
        case 'h':
        case 'H':
        case 'p':
        case 'P':
        case 'u':
        case 'U':
        case 'r':
        case 'R':
        case 's':
        case 'S':
            SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
        // Enter does the disassembly
        case VK_RETURN:
            DecodeRange();
            return TRUE;
        // Tab moves between active edit boxes
        case VK_TAB:
               SetWindowTextA(hErrText,initTxt);
               if (hCtl == hEdtAddr) {
                  if (IsDlgButtonChecked(hDismDlg,IDC_PHYS_MEM)) {
                     nxtctl = hEdtBloc;
                   } else {
                       nxtctl = hEdtLcnt;
                }
            } else if (hCtl == hEdtBloc) {
                nxtctl = hEdtLcnt;
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
        // Up/Down scroll disassembly display
        case VK_DOWN:
            Edit_Scroll(hDisText,1,0);
            return TRUE;
        case VK_UP:
            Edit_Scroll(hDisText,-1,0);
            return TRUE;
        // Page Up/Down send to disassembly display
        case VK_PRIOR:
        case VK_NEXT:
            SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
        }
        break;
    }
    // Everything else sent to original control processing
    return CallWindowProc(pCtl,hCtl,msg,wPrm,lPrm);
}

/***************************************************/
/*   Get real address from disassembly address     */
/***************************************************/
int RealAddr(int addr)
{
    if (DisTxtRealAdr) {
        return addr;
    } else {
        int block = RealBlock(addr);
        int offset = addr & 0x1FFF;
        return offset + block * 0x2000;
    }
}

/***************************************************/
/*          Get block from CPU address             */
/***************************************************/
int RealBlock(int address)
{
    if (MMUregs.ActiveTask == 0) {
        return MMUregs.Task0[(address >> 13) & 7];
    } else {
        return MMUregs.Task1[(address >> 13) & 7];
    }
}

/***************************************************/
/*        Get Cpu address from real address        */
/***************************************************/
int CpuAddr(int realaddr)
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
        cpuadr = 0;
    }
    return cpuadr;
}

/***************************************************/
/*         Change Address map real to CPU          */
/***************************************************/
void MapRealToAdr()
{
    char buf[16];
    std::string s;
    unsigned long address;
    unsigned short block;
    unsigned short offset;
    GetWindowText(hEdtAddr, buf, 8);
    offset = HexToUint(buf);
    GetWindowText(hEdtBloc, buf, 8);
    block = HexToUint(buf);
    int i;
    for (i=0; i<8; i++) {
        if (MMUregs.ActiveTask == 0) {
            if (MMUregs.Task0[i] == block) break;
        } else {
            if (MMUregs.Task1[i] == block) break;
        }
    }
    if (i < 8) {
        address = i * 0x2000 + offset;
        SetWindowText(hEdtAddr, HEXSTR(address,0).c_str());
    } else {
        SetWindowText(hEdtAddr,"    ");
    }

    SetWindowText(hEdtBloc,"  ");
    SetWindowText(GetDlgItem(hDismDlg,IDC_ADRTXT),"Address:");
    EnableWindow(hEdtBloc,FALSE);
    UsePhyAdr = FALSE;
}


/***************************************************/
/*         Change Address map CPU to real          */
/***************************************************/
void MapAdrToReal()
{
    char buf[16];
    unsigned short address;
    unsigned short block;
    unsigned short offset;
    //MMUState MMUState_;
    //MMUState_ = GetMMUState();
    //MMUState regs = MMUState_;

    GetWindowText(hEdtAddr, buf, 8);
    address = HexToUint(buf);

    offset = address & 0x1FFF;
    block = RealBlock(address);
    SetWindowText(hEdtAddr, HEXSTR(offset,0).c_str());
    SetWindowText(hEdtBloc,HEXSTR(block,2).c_str());
    SetWindowText(GetDlgItem(hDismDlg,IDC_ADRTXT),"  Offset:");
    EnableWindow(hEdtBloc,TRUE);
    UsePhyAdr = TRUE;
}

/**************************************************/
/*          Get range and disassemble             */
/**************************************************/
void DecodeRange()
{
    char buf[16];
    unsigned short FromAdr = 0;
    unsigned short MaxLines;
    unsigned short Block = 0;

    // If not using physical addressing disallow access I/O ports
    unsigned short MaxAdr = UsePhyAdr? 0xFFFF: 0xFF00;

    if (UsePhyAdr) {
        GetWindowText(hEdtBloc, buf, 8);
        int blk = HexToUint(buf);
        if ((blk < 0) || (blk > 0x3FF)) {
            SetWindowText(hErrText,"Invalid Block Number");
            return;
        }
        Block = (unsigned short) blk;
    }

    // Start CPU address or block offset
    GetWindowText(hEdtAddr, buf, 8);
    FromAdr = HexToUint(buf);
    if ((FromAdr < 0) || (FromAdr >= MaxAdr)) {
        SetWindowText(hErrText,"Invalid start address");
        return;
    }

    // Number of lines
    GetWindowText(hEdtLcnt, buf, 8);
    MaxLines = (unsigned short) strtol(buf,NULL,10);
    if (MaxLines > 2000) {
        SetWindowText(hErrText,"Invalid line count");
        return;
    }

    // Grab current MMUregs and save phy address flag
    // at time of decode so haltpoints created are
    // based on state when decode was actually done.
    MMUregs = GetMMUState();
    DisTxtRealAdr = UsePhyAdr;

    // Disassemble
    Disassemble(FromAdr,MaxLines,Block);
 
    ColorizeAllHaltpoints();
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
    if (UsePhyAdr) {
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
    if (UsePhyAdr) addr += Block * 0x2000;

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
                  unsigned short MaxLines,
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
    state.phyAddr = UsePhyAdr;
    while (numlines < MaxLines) {

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
            unsigned char op = Debugger::DbgRead8(UsePhyAdr, Block, opadr);
            trace.instruction = "OS9";
            trace.operand = "#$"+HEXSTR(op,2);
            trace.bytes.push_back(op);
        }

        // Instruction address
        int iaddr;
        if (UsePhyAdr)
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
UINT16 HexToUint(char * buf)
{
    long val;
    char *eptr;
    if (*buf == '\0')  return 0;
    val = strtol(buf,&eptr,16);
    if (*eptr != '\0') return -1;
    if (val < 0)       return -1;
    if (val > 0xFFFF)  return -1;
    return val & 0xFFFF;
}

}// Namespace ends
