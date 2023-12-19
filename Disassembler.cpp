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
//#include "OpCodeTables.h"
#include "OpDecoder.h"
#include "tcc1014mmu.h"

// Namespace starts here
namespace VCC { namespace Debugger { namespace UI {

// Windows handles
HWND hDismWin = NULL;  // Set when window created
HWND hDismDlg = NULL;  // Set when dialog activated
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtLast = NULL;  // To editbox
HWND hEdtAPPY = NULL;  // Apply button
HWND hDisText = NULL;  // Richedit20 box for output

// local references
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
void DisAsmFromTo();
void Disassemble(UINT16, UINT16);
UINT16 ConvertHexAddress(char *);
LRESULT CALLBACK SubAddrDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
LRESULT CALLBACK SubLastDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC LastDlgProc;
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR Proc);
void SetDialogFocus(HWND);
WPARAM TabDirection=0;

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
        hEdtLast = GetDlgItem(hDismDlg, IDC_EDIT_PC_LAST);
        hEdtAPPY = GetDlgItem(hDismDlg, IDAPPLY);
        hDisText = GetDlgItem(hDismDlg, IDC_DISASSEMBLY_TEXT);
        // Modeless dialogs do not support tab or enter keys
        // Hook the address dialogs to capture those keystrokes
        AddrDlgProc = (WNDPROC) SetControlHook(hEdtAddr,(LONG_PTR) SubAddrDlgProc);
        LastDlgProc = (WNDPROC) SetControlHook(hEdtLast,(LONG_PTR) SubLastDlgProc);
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
// Hook for start address box
LRESULT CALLBACK SubAddrDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=0;
    return ProcEditDlg(AddrDlgProc,hDlg,msg,wPrm,lPrm);
}
// Hook for end address box
LRESULT CALLBACK SubLastDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=1;
    return ProcEditDlg(LastDlgProc,hDlg,msg,wPrm,lPrm);
}
// Messages for either address box pass through here
BOOL ProcEditDlg
    (WNDPROC DlgProc, HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wPrm) {
        // Enter key does the disassembly
        case VK_RETURN:
            SendMessage(hEdtAPPY,BM_CLICK,0,0);
            return TRUE;
        // Tab key toggles to the other address box
        case VK_TAB:
            SendMessage(hDismDlg,WM_NEXTDLGCTL,TabDirection,0);
            return TRUE;
        }
    }
    // Everything else sent to the original dialog proc.
    return CallWindowProc(DlgProc,hDlg,msg,wPrm,lPrm);
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
    UINT16 FromAdr, ToAdr;

    // Start address
    GetWindowText(hEdtAddr, buf, 8);
    FromAdr = ConvertHexAddress(buf);
    if ((FromAdr < 0) || (FromAdr > 0xFF00)) {
        MessageBox(hDismDlg, "Invalid start address", "Error", 0);
        return;
    }

    // End Address
    GetWindowText(hEdtLast, buf, 8);
    ToAdr = ConvertHexAddress(buf);
    if ((ToAdr < 0) || (ToAdr > 0xFF00)) {
        MessageBox(hDismDlg, "Invalid end address", "Error", 0);
        return;
    }

    // Validate range
    int range = ToAdr - FromAdr;
    if ((range < 1) || (range > 0x1000)) {
        MessageBox(hDismDlg, "Invalid range", "Error", 0);
        return;
    }

    // Good range, disassemble
    Disassemble(FromAdr,ToAdr);
    return;
}

/**************************************************/
/*  Disassemble Instructions in specified range   */
/**************************************************/
void Disassemble(UINT16 FromAdr, UINT16 ToAdr)
{
    std::unique_ptr<OpDecoder> Decoder;
    Decoder = std::make_unique<OpDecoder>();
    VCC::CPUTrace trace = {};
    VCC::CPUState state = {};
    std::string lines = {};
    UINT16 PC = FromAdr;

    while (PC < ToAdr) {
        state.PC = PC;
        trace = {};
        Decoder->DecodeInstruction(state,trace);

        // Create string containing PC and instruction bytes
        std::string HexDmp = ToHexString(PC,4,TRUE)+"\t";
        for (unsigned int i=0; i < trace.bytes.size(); i++) {
            HexDmp += ToHexString(trace.bytes[i],2,TRUE)+" ";
        }

        // Pad hex dump and instruction name
        if (HexDmp.length() < 24)
            HexDmp.append(24 - HexDmp.length(),' ');
        if (trace.instruction.length() < 8)
            trace.instruction.append(8 - trace.instruction.length(),' ');

        // Append line for output
        lines += HexDmp+"\t"+trace.instruction+"\t"+trace.operand+"\n";

        // Next instruction, prevent infinate loop
        if (trace.bytes.size() > 0)
            PC += (UINT16) trace.bytes.size();
        else
            PC += 1;
    }
    // Put the complete disassembly
    SetWindowTextA(hDisText,ToLPCSTR(lines));
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
