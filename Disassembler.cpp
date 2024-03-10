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

// Namespace starts here
namespace VCC { namespace Debugger { namespace UI {

// Windows handles
HWND hDismWin = NULL;  // Set when window created
HWND hDismDlg = NULL;  // Set when dialog activated
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtLcnt = NULL;  // To editbox
HWND hEdtBloc = NULL;  // Bock number editbox
HWND hEdtAPPY = NULL;  // Apply button
HWND hDisText = NULL;  // Richedit20 box for output
HWND hErrText = NULL;  // Error text box

// local references
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
void DecodeRange();
void Disassemble(unsigned short, unsigned short, bool, unsigned short);
UINT16 ConvertHexAddress(char *);
LRESULT CALLBACK SubAddrDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
LRESULT CALLBACK SubBlocDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC BlocDlgProc;
LRESULT CALLBACK SubLcntDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC LcntDlgProc;
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM,int);
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR Proc);
void SetDialogFocus(HWND);
char initTxt[] = "Address and Block are hex.  Lines decimal";
std::string IntToHex(int,int);
std::string OpFDB(int,std::string,std::string,std::string);
std::string OpFCB(int,std::string,std::string);
int DecodeModHdr(unsigned long, std::string *mhdr);

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
    int tstops[4]={37,106,138,180};  // address, opcode, parm, cmt
    switch (msg) {
    case WM_INITDIALOG:
        // Grab dialog and control handles
        hDismDlg = hDlg;
        hEdtAddr = GetDlgItem(hDismDlg, IDC_EDIT_PC_ADDR);
        hEdtLcnt = GetDlgItem(hDismDlg, IDC_EDIT_PC_LCNT);
        hEdtAPPY = GetDlgItem(hDismDlg, IDAPPLY);
        hDisText = GetDlgItem(hDismDlg, IDC_DISASSEMBLY_TEXT);
        hEdtBloc = GetDlgItem(hDismDlg, IDC_EDIT_BLOCK);
        hErrText = GetDlgItem(hDismDlg, IDC_ERROR_TEXT);

        // Modeless dialogs do not support tab or enter keys
        // Hook the input dialogs to capture those keystrokes
        AddrDlgProc = (WNDPROC) SetControlHook(hEdtAddr,(LONG_PTR) SubAddrDlgProc);
        BlocDlgProc = (WNDPROC) SetControlHook(hEdtBloc,(LONG_PTR) SubBlocDlgProc);
        LcntDlgProc = (WNDPROC) SetControlHook(hEdtLcnt,(LONG_PTR) SubLcntDlgProc);
        // Set tab stops in output edit box
        Edit_SetTabStops(hDisText,4,tstops);
        // Inital values in edit boxes
        SetWindowText(hEdtBloc, "0");
        SetWindowText(hEdtAddr, "0");
        SetWindowText(hEdtLcnt, "500");
        SetWindowTextA(hDisText,"");
        SetWindowTextA(hErrText,initTxt);
        // Disable Block text box until phys mem is checked
        EnableWindow(hEdtBloc,FALSE);
        break;
    case WM_PAINT:
        SendMessage(hDlg,WM_NEXTDLGCTL,(WPARAM) hEdtAddr,(LPARAM) TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            DestroyWindow(hDlg);
            hDismWin = NULL;
            break;
        case IDAPPLY:
            DecodeRange();
            SetDialogFocus(hEdtAddr);
            return TRUE;
        case IDC_PHYS_MEM:
            if (IsDlgButtonChecked(hDlg,IDC_PHYS_MEM)) {
                EnableWindow(hEdtBloc,TRUE);
                SendMessage(hDlg,WM_NEXTDLGCTL,(WPARAM) hEdtBloc,(LPARAM) TRUE);
            } else {
                EnableWindow(hEdtBloc,FALSE);
                SendMessage(hDlg,WM_NEXTDLGCTL,(WPARAM) hEdtAddr,(LPARAM) TRUE);
            }

            EnableWindow(hEdtBloc,IsDlgButtonChecked(hDlg,IDC_PHYS_MEM));
            return TRUE;
        }
    }   return FALSE;
}

/***************************************************/
/*            set up control hook                  */
/***************************************************/
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR HookProc)
{
    LONG_PTR OrigProc=GetWindowLongPtr(hCtl,GWLP_WNDPROC);
    SetWindowLongPtr(hCtl,GWLP_WNDPROC,HookProc);
    return OrigProc;
}

/***************************************************/
/* Grab enter and tab keystrokes from edit dialogs */
/***************************************************/

// Subclass hooks for dialog edit boxes to establish tab order
LRESULT CALLBACK SubAddrDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(AddrDlgProc,hDlg,msg,wPrm,lPrm,1); }
LRESULT CALLBACK SubLcntDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(LcntDlgProc,hDlg,msg,wPrm,lPrm,2); }
LRESULT CALLBACK SubBlocDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(BlocDlgProc,hDlg,msg,wPrm,lPrm,0); }

// All messages to dialog edit boxes go through ProcEditDlg
BOOL ProcEditDlg (WNDPROC pDlg,HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm,int next)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wPrm) {
        // Enter key does the disassembly
        case VK_RETURN:
            SendMessage(hEdtAPPY,BM_CLICK,0,0);
            return TRUE;
		// Page Up/Down send to disassembly display
        case VK_PRIOR:
        case VK_NEXT:
			SendMessage(hDisText,msg,wPrm,lPrm);
            return TRUE;
		// Tab key sets focus to the next edit box
        case VK_TAB:
            SetWindowTextA(hErrText,initTxt);
            switch (next) {
            case 1:
                SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM) hEdtLcnt,(LPARAM) TRUE);
                break;
            case 2:
                if (IsDlgButtonChecked(hDismDlg,IDC_PHYS_MEM)) {
                    SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM) hEdtBloc,(LPARAM) TRUE);
                    break;
                }
                // Falls through if button not checked
            default:
                SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM) hEdtAddr,(LPARAM) TRUE);
                break;
            }
            return TRUE;
        }
    }
    // Everything else sent to the original dialog proc.
    return CallWindowProc(pDlg,hDlg,msg,wPrm,lPrm);
}

/**************************************************/
/*          Get range and disassemble             */
/**************************************************/
void DecodeRange()
{
    char buf[16];
    unsigned short FromAdr = 0;
    unsigned short MaxLines;
    bool UsePhyAdr;
    unsigned short Block = 0;

    // Physical address check box
    unsigned int ret = IsDlgButtonChecked(hDismDlg,IDC_PHYS_MEM);
    UsePhyAdr = (ret == BST_CHECKED) ? TRUE : FALSE;

    // If not using physical addressing disallow access I/O ports
    unsigned short MaxAdr = UsePhyAdr? 0xFFFF: 0xFF00;

    if (UsePhyAdr) {
        GetWindowText(hEdtBloc, buf, 8);
        int blk = ConvertHexAddress(buf);
        if ((blk < 0) || (blk > 0x3FF)) {
            SetWindowText(hErrText,"Invalid Block Number");
            return;
        }
        Block = (unsigned short) blk;
    }

    // Start CPU address or block offset
    GetWindowText(hEdtAddr, buf, 8);
    FromAdr = ConvertHexAddress(buf);
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
    Disassemble(FromAdr,MaxLines,UsePhyAdr,Block);
    return;
}

/***************************************************/
/*        SetFocus to a dialog control             */
/***************************************************/
void SetDialogFocus(HWND hCtrl) {
    SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM)hCtrl,TRUE);
}

/***************************************************/
/*    Convert integer to zero filled hex string    */
/***************************************************/
std::string IntToHex(int ival,int width) {
    std::ostringstream fmt;
    fmt<<std::hex<<std::setw(width)<<std::uppercase<<std::setfill('0')<<ival;
    return fmt.str();
}

/***************************************************/
/*        Create FDB line with comment             */
/***************************************************/
std::string OpFDB(int addr,std::string b1,std::string b2,std::string cmt) {
    return IntToHex(addr,6)+"\t"+b1+" "+b2+"\tFDB\t$"+b1+b2+"\t"+cmt+"\n";
}

/***************************************************/
/*        Create FCB line with comment             */
/***************************************************/
std::string OpFCB(int addr,std::string b,std::string cmt) {
    return IntToHex(addr,6)+"\t"+b+"\tFCB\t$"+b+"\t"+cmt+"\n";
}

/***************************************************/
/* Test for and conditionally decode module header */
/***************************************************/
int DecodeModHdr(unsigned long addr, std::string *hdr)
{
    // Convert header bytes to hex with check
    std::string hexb[13];
    int cksum = 0xFF;
    for (int n=0; n<9; n++) {
        int ch = GetMem(n+addr);
        if ((n == 0) && (ch != 0x87)) break;
        if ((n == 1) && (ch != 0xCD)) break;
        hexb[n] = IntToHex(ch,2);
        cksum = cksum ^ ch;
        //if (n < 9) cksum = cksum ^ ch;
    }
    if (cksum) return 0;

    // Generate header lines
    int cnt = 0;
    *hdr =  OpFDB(addr+cnt++,hexb[0],hexb[1],"module"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[2],hexb[3],"modsiz"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[4],hexb[5],"offnam"); cnt++;
    *hdr += OpFCB(addr+cnt++,hexb[6],"ty/lg");
    *hdr += OpFCB(addr+cnt++,hexb[7],"at/rv");
    *hdr += OpFCB(addr+cnt++,hexb[8],"parity");
    // Additional lines if executable
    char type = hexb[6][0];
    if ((type=='1')||(type=='E')) {
        for (int n=0; n<4; n++) {
            int ch = GetMem(cnt+n+addr);
            hexb[n+9] = IntToHex(ch,2);
        }
        *hdr += OpFDB(addr+cnt++,hexb[9],hexb[10],"offexe"); cnt++;
        *hdr += OpFDB(addr+cnt++,hexb[11],hexb[12],"datsiz"); cnt++;
    }
    return cnt;
}

/**************************************************/
/*  Disassemble Instructions in specified range   */
/**************************************************/

//     DECB startup executables mapping
// Block  Phy addr   CPU addr     Contents
//  $3C   $078000   $8000-$9FFF Extended Basic ROM
//  $3D   $07A000   $A000-$BFFF Color Basic ROM
//  $3E   $07C000   $C000-$DFFF Cartridge or Disk Basic ROM
//  $3F   $07E000   $D000-$FFFF Super Basic, GIME regs, I/O

void Disassemble( unsigned short FromAdr,
                  unsigned short MaxLines,
                  bool UsePhyAddr,
                  unsigned short Block)
{
    std::unique_ptr<OpDecoder> Decoder;
    Decoder = std::make_unique<OpDecoder>();
    VCC::CPUTrace trace = {};
    VCC::CPUState state = {};
    std::string lines = {};

    unsigned short PC = FromAdr;
    int numlines;

    int hdrcnt = 0;
    if (UsePhyAddr) {
        hdrcnt = DecodeModHdr(PC+Block*0x2000, &lines);
        if (hdrcnt) {
            PC += hdrcnt;
            if (hdrcnt > 9)
                numlines = 8;
            else
                numlines = 6;
        }
    }

    state.block = Block;
    state.phyAddr = UsePhyAddr;
    unsigned short MaxAdr = UsePhyAddr? 0xFFFF: 0xFF00;
    while ((numlines < MaxLines) && (PC < MaxAdr)) {
        state.PC = PC;
        trace = {};
        Decoder->DecodeInstruction(state,trace);

        // If a module convert SWI2 to OS9 with immediate operand
        if (hdrcnt && (trace.instruction == "SWI2")) {
            unsigned short opadr = PC + (unsigned short) trace.bytes.size();
            unsigned char op = DbgRead8(UsePhyAddr, Block, opadr);
            trace.instruction = "OS9";
            trace.operand = "#$"+ToHexString(op,2,TRUE);
            trace.bytes.push_back(op);
        }

        // Create string containing instruction address
        std::string HexAddr;
        if (UsePhyAddr) {
            HexAddr = ToHexString(PC+Block*0x2000,6,TRUE);
        } else {
            HexAddr = ToHexString(PC,4,TRUE);
        }

        // Create string of instruction bytes
        std::string HexInst;
        for (unsigned int i=0; i < trace.bytes.size(); i++) {
            HexInst += ToHexString(trace.bytes[i],2,TRUE)+" ";
        }

        // Append address, bytes, instruction, and operand to output
        lines += HexAddr + "\t" + HexInst + "\t"
               + trace.instruction +"\t"
               + trace.operand +"\n";
        numlines++;

        // Next instruction
        if (trace.bytes.size() > 0)
            PC += (UINT16) trace.bytes.size();
        else
            PC += 1;
    }
    // Put the completed disassembly
    SetWindowTextA(hDisText,"");
    SetWindowTextA(hDisText,ToLPCSTR(lines));
    if (PC >= MaxAdr)
        SetWindowText(hErrText,"Stopped at invalid address");
    else
        SetWindowTextA(hErrText,initTxt);
}

/**************************************************/
/*        Convert hex digits to Address           */
/**************************************************/
UINT16 ConvertHexAddress(char * buf)
{
    long val;
    char *eptr;
    if (*buf == '\0')  return -1;
    val = strtol(buf,&eptr,16);
    if (*eptr != '\0') return -1;
    if (val < 0)       return -1;
    if (val > 0xFFFF)  return -1;
    return val & 0xFFFF;
}

// Namespace ends here
}}}
