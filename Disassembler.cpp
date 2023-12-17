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
// Ed Jaquay
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
#include "OpCodeTables.h"
#include "tcc1014mmu.h"

// Namespace starts here
namespace VCC { namespace Debugger { namespace UI {

// Windows handles
HWND hDismWin = NULL;  // Set when window created
HWND hDismDlg = NULL;  // Set when dialog activated
HWND hEdtAddr = NULL;  // From editbox
HWND hEdtLast = NULL;  // To editbox
HWND hEdtAPPY = NULL;  // Apply button

// Local references
INT_PTR CALLBACK DisassemblerDlgProc(HWND,UINT,WPARAM,LPARAM);
void DisAsmFromTo();
void Disassemble(unsigned short, unsigned short);
unsigned short ConvertHexAddress(char *);
LRESULT CALLBACK SubAddrDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
LRESULT CALLBACK SubLastDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC LastDlgProc;
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM);
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
        // Modeless dialogs do not do tab or enter keys properly
        // Subclass the address dialogs to capture those keystrokes
        AddrDlgProc = (WNDPROC) GetWindowLongPtr(hEdtAddr, GWLP_WNDPROC);
        SetWindowLongPtr(hEdtAddr, GWLP_WNDPROC, (LONG_PTR) SubAddrDlgProc);
        LastDlgProc = (WNDPROC) GetWindowLongPtr(hEdtLast, GWLP_WNDPROC);
        SetWindowLongPtr(hEdtLast, GWLP_WNDPROC, (LONG_PTR) SubLastDlgProc);
        // Set focus to start address box
        SetDialogFocus(hEdtAddr);
        break;
    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            DestroyWindow(hDlg);
            hDismWin = NULL;
            break;
        // Try to keep focus on start address box
        case WM_ACTIVATE:
            SetDialogFocus(hEdtAddr);
            break;
        // Apply button does the disassembly
        case IDAPPLY:
            DisAsmFromTo();
            SetDialogFocus(hEdtAddr);
            return TRUE;
        }
    }   return FALSE;
}

/***************************************************/
/*        SetFocus to a dialog control             */
/***************************************************/
void SetDialogFocus(HWND hCtrl) {
    SendMessage(hDismDlg,WM_NEXTDLGCTL,(WPARAM)hCtrl,TRUE);
}

/***************************************************/
/* Handle enter and tab keystrokes in edit dialogs */
/***************************************************/

// Sub class handler for start address box
LRESULT CALLBACK SubAddrDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=0;
    return ProcEditDlg(AddrDlgProc,hDlg,msg,wPrm,lPrm);
}
// Sub class handler for end address box
LRESULT CALLBACK SubLastDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=1;
    return ProcEditDlg(LastDlgProc,hDlg,msg,wPrm,lPrm);
}
// Either address box messages land here
BOOL ProcEditDlg
(WNDPROC DlgProc, HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wPrm) {
        // Return key does the disassembly
        case VK_RETURN:
            SendMessage(hEdtAPPY,BM_CLICK,0,0);
            return TRUE;
        // Tab key goes to the other address box
        case VK_TAB:
            SendMessage(hDismDlg,WM_NEXTDLGCTL,TabDirection,0);
            return TRUE;
        }
    }
    // Everything else handled by stock handler
    return CallWindowProc(DlgProc,hDlg,msg,wPrm,lPrm);
}

/**************************************************/
/*          Get range and disassemble             */
/**************************************************/
void DisAsmFromTo()
{
    char buf[16];
    unsigned short from, to;

    // Start address
    GetWindowText(hEdtAddr, buf, 8);
    from = ConvertHexAddress(buf);
	if ((from < 0) || (from > 0xFF00)) {
        MessageBox(hDismDlg, "Invalid start address", "Error", 0);
        return;
    }

    // End Address
    GetWindowText(hEdtLast, buf, 8);
    to = ConvertHexAddress(buf);
	if ((to < 0) || (to > 0xFF00)) {
        MessageBox(hDismDlg, "Invalid end address", "Error", 0);
        return;
	}

    // Validate range
    int range = to - from;
    if ((range < 1) || (range > 0x1000)) {
        MessageBox(hDismDlg, "Invalid range", "Error", 0);
        return;
    }

    // Good range, disassemble
    Disassemble(from,to);
    return;
}

/**************************************************/
/*  Disassemble Instructions in specified range   */
/**************************************************/
void Disassemble(unsigned short from, unsigned short to)
{
    std::unique_ptr<OpCodeTables> OpCdTbl;
    OpCdTbl = std::make_unique<OpCodeTables>();
    OpCodeTables::OpCodeInfo OpInf;
    VCC::CPUTrace trace = {};
    VCC::CPUState state = {};

    std::string lines = {};
    unsigned char opcd;

    unsigned short PC = from;
    while (PC < to) {

        unsigned short addr = PC;
        trace = {};
        state.PC = addr;

        // Get information for opcode or extended opcode
        opcd = MemRead8(addr);
        OpInf = OpCdTbl->Page1OpCodes[opcd];
        trace.bytes.push_back(opcd);
        if (OpInf.mode == OpCodeTables::AddressingMode::OpPage2) {
            opcd = MemRead8(addr+1);
            OpInf = OpCdTbl->Page2OpCodes[opcd];
            trace.bytes.push_back(opcd);
        } else if (OpInf.mode == OpCodeTables::AddressingMode::OpPage3) {
            opcd = MemRead8(addr+1);
            OpInf = OpCdTbl->Page3OpCodes[opcd];
            trace.bytes.push_back(opcd);
        }
        trace.instruction = OpInf.name;

        // Use ProcessHeuristics to decode operands.  Kludgey
        // but gets it done without creating new a new method
        OpCdTbl->ProcessHeuristics(OpInf, state, trace);

        // Instuction length or num of opcode bytes if bad code
        unsigned short
            inslen = (OpInf.numbytes > 0) ? OpInf.numbytes : OpInf.oplen;

        // No std format so use sprintf to format hex for dump
        char cstr[8];
        std::string HexDmp = {};
        sprintf(cstr,"%04X\t",addr);
        HexDmp.append(cstr);
        for (int i = 0; i < inslen; i++) {
            sprintf(cstr,"%02X ",MemRead8(addr++));
            HexDmp.append(cstr);
        }

        // Pad hex dump and instruction name
        if (HexDmp.length() < 25)
            HexDmp.append(25 - HexDmp.length(),' ');
        if (OpInf.name.length() < 8)
            OpInf.name.append(8 - OpInf.name.length(),' ');

        // Put results
        lines += HexDmp.c_str();
        lines += "\t";
        lines += OpInf.name.c_str();
        lines += "\t";
        lines += trace.operand.c_str();
        lines += "\n";

        // Next instruction
        PC += inslen;
    }
    HWND hTxt = GetDlgItem(hDismDlg,IDC_DISASSEMBLY_TEXT);
    SetWindowTextA(hTxt,ToLPCSTR(lines));
}

/**************************************************/
/*        Convert hex digits to Address           */
/**************************************************/
unsigned short ConvertHexAddress(char * buf)
{   char *eptr;
    long val = strtol(buf,&eptr,16);
    // Address must be between 0 and 0xFF00  (65280)
    if (*buf == '\0' || *eptr != '\0' || val < 0 || val > 65279) {
        return -1;
    } else {
        return val & 0xFFFF;
    }
}

// Namespace ends here
}}}
