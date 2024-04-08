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

#define HEXSTR(i,w) ToHexString(i,w,true)

// Namespace starts here
namespace VCC { namespace Debugger { namespace UI {

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

int  CurLinePos = -1;
void GetLinePos(HWND);
void ColorCurAddr(HWND,bool);

bool UsePhyAdr = FALSE;
bool Os9Decode = FALSE;

char initTxt[] = "Address and Block are hex.  Lines decimal";

// Disassembled code goes into string here
std::string sDecoded = {};

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

BOOL ProcTextDlg (WNDPROC pCtl,HWND hCtl,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    // Set or Clear Breakpoint at current line
    case WM_CHAR:
        switch (wPrm) {
        case 'B':
        case 'b':
            ColorCurAddr(hCtl,true);
            return TRUE;
        case 'C':
        case 'c':
            ColorCurAddr(hCtl,false);
            return TRUE;
        }
    // Get current line position
    case WM_PAINT:
    case WM_LBUTTONUP:
        GetLinePos(hCtl);
        break;
    case WM_KEYUP:
        switch (wPrm) {
        case VK_PRIOR:
        case VK_NEXT:
        case VK_DOWN:
        case VK_UP:
            GetLinePos(hCtl);
            break;
        }
    }

    // Forward messages to original control
    return CallWindowProc(pCtl,hCtl,msg,wPrm,lPrm);
}

/***************************************************/
/*     Colorize address at Current Line            */
/***************************************************/
void ColorCurAddr(HWND hCtl, bool flag)
{
    if (CurLinePos<0) return;
    CHARFORMATA fmt;
    fmt.cbSize = sizeof(fmt);
    SendMessage(hCtl,EM_SETSEL,CurLinePos,CurLinePos+6);
    if (flag) {
        fmt.crTextColor = RGB(255,0,0); // Red
        fmt.dwEffects = CFE_BOLD;
    } else {
        fmt.crTextColor = RGB(0,0,0);
        fmt.dwEffects = 0;
    }
    fmt.dwMask = CFM_COLOR | CFM_BOLD;
    SendMessage(hCtl,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &fmt);
    SendMessage(hCtl,EM_SETSEL,CurLinePos,CurLinePos);
}

/***************************************************/
/*       Find current text line position           */
/***************************************************/
int pnum=0;
void GetLinePos(HWND hCtl)
{
    CHARRANGE range;
    SendMessage(hCtl,EM_EXGETSEL,0,(LPARAM) &range);
    if (range.cpMin==range.cpMax) {
        int i = SendMessage(hCtl,EM_LINEFROMCHAR,range.cpMin,0);
        CurLinePos = SendMessage(hCtl,EM_LINEINDEX,i,0);
        SendMessage(hCtl,EM_SETSEL,CurLinePos,CurLinePos);
    } else {
        CurLinePos = -1;
    }
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
/*         Change Address map real to CPU          */
/***************************************************/
void MapRealToAdr()
{
    char buf[16];
    std::string s;
    unsigned long address;
    unsigned short block;
    unsigned short offset;
    MMUState MMUState_;
    MMUState_ = GetMMUState();
    MMUState regs = MMUState_;
    GetWindowText(hEdtAddr, buf, 8);
    offset = HexToUint(buf);
    GetWindowText(hEdtBloc, buf, 8);
    block = HexToUint(buf);
    int i;
    for (i=0; i<8; i++) {
        if (regs.ActiveTask == 0) {
            if (regs.Task0[i] == block) break;
        } else {
            if (regs.Task1[i] == block) break;
        }
    }
    if (i < 8) {
        address = i * 0x2000 + offset;
        SetWindowText(hEdtAddr, HEXSTR(address,0).c_str());
    }
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
    MMUState MMUState_;
    MMUState_ = GetMMUState();
    MMUState regs = MMUState_;

    GetWindowText(hEdtAddr, buf, 8);
    address = HexToUint(buf);

    offset = address & 0x1FFF;
    SetWindowText(hEdtAddr, HEXSTR(offset,0).c_str());

    if (regs.ActiveTask == 0) {
        block = regs.Task0[(address >> 13) & 7];
    } else {
        block = regs.Task1[(address >> 13) & 7];
    }
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

    // Disassemble
    Disassemble(FromAdr,MaxLines,Block);
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
    std::unique_ptr<OpDecoder> Decoder;
    Decoder = std::make_unique<OpDecoder>();
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
            unsigned char op = DbgRead8(UsePhyAdr, Block, opadr);
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
    SetWindowTextA(hDisText,ToLPCSTR(sDecoded));
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

// Namespace ends here
}}}
