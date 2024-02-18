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
#include <Windowsx.h>
#include "Disassembler.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "resource.h"
#include <string>
#include <commdlg.h>
#include <Richedit.h>
#include "vcc.h"
#include "logger.h"
#include "OpDecoder.h"
#include "tcc1014mmu.h"

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
void DisAsmFromTo();
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
char initTxt[] = "Address and block are hex.  Lines is decimal";

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
    int tstops[3]={37,106,138};  // address, opcode, parm
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
        // Set tab stops in ooutput edit box
        Edit_SetTabStops(hDisText,3,tstops);
        // Inital values in edit boxes
        SetWindowText(hEdtBloc, "0");
        SetWindowText(hEdtAddr, "0");
        SetWindowText(hEdtLcnt, "1000");
        SetWindowTextA(hDisText,"");
        SetWindowTextA(hErrText,initTxt);
        // Set focus to the first edit box
        SetDialogFocus(hEdtAddr);
        break;
    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            DestroyWindow(hDlg);
            hDismWin = NULL;
            break;
        case IDAPPLY:
            DisAsmFromTo();
            SetDialogFocus(hEdtAddr);
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
        // Tab key sets focus to the next edit box
        case VK_TAB:
            SetWindowTextA(hErrText,initTxt);
            switch (next) {
            case 1:
                SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM) hEdtLcnt,(LPARAM) TRUE);
                break;
            case 2:
                SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM) hEdtBloc,(LPARAM) TRUE);
                break;
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

/***************************************************/
/*        SetFocus to a dialog control             */
/***************************************************/
void SetDialogFocus(HWND hCtrl) {
    SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM)hCtrl,TRUE);
}

/**************************************************/
/*          Get range and disassemble             */
/**************************************************/
void DisAsmFromTo()
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

    // Ramsize bytes  0x20000 0x80000 0x200000 0x800000
    //         kbytes     128     512     2048     8192
    //         Blks        10      40      100      400
    // Reads beyond physical mem will return 0xFF

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
    if (MaxLines > 1000) {
        SetWindowText(hErrText,"Invalid line count");
        return;
    }

    // Disassemble
    Disassemble(FromAdr,MaxLines,UsePhyAdr,Block);
    return;
}

// VCC startup executables mapping
// Block  Phy address  CPU address Contents
//  $3C  $78000-$79FFF $8000-$9FFF Extended Basic ROM
//  $3D  $7A000-$7BFFF $A000-$BFFF Color Basic ROM
//  $3E  $7C000-$7DFFF $C000-$DFFF Cartridge or Disk Basic ROM
//  $3F  $7E000-$7FFFF $D000-$FFFF Super Basic, GIME regs, I/O, Interrupts

/**************************************************/
/*  Disassemble Instructions in specified range   */
/**************************************************/
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
    int numlines = 0;

    state.phyAddr=UsePhyAddr; // Boolean decode using physical addressing
    state.block=Block;        // Physical address = PC + block * 0x2000

    unsigned short MaxAdr = UsePhyAddr? 0xFFFF: 0xFF00;
    while ((numlines < MaxLines) && (PC < MaxAdr)) {
        state.PC = PC;
        trace = {};
        Decoder->DecodeInstruction(state,trace);

        std::string HexAddr;
        std::string HexInst;

        // Create string containing instruction address
        if (UsePhyAddr) {
            HexAddr = ToHexString(PC+Block*0x2000,6,TRUE);
        } else {
            HexAddr = ToHexString(PC,4,TRUE);
        }

        // Create string of instruction bytes
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
