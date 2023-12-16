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
        // Modeless dialogs process tab or enter keys properly
        // Subclass edit dialogs to capture those keystrokes
        hDismDlg = hDlg;
        hEdtAddr = GetDlgItem(hDismDlg, IDC_EDIT_PC_ADDR);
        hEdtLast = GetDlgItem(hDismDlg, IDC_EDIT_PC_LAST);
        hEdtAPPY = GetDlgItem(hDismDlg, IDAPPLY);
        AddrDlgProc = (WNDPROC) GetWindowLongPtr(hEdtAddr, GWLP_WNDPROC);
        SetWindowLongPtr(hEdtAddr, GWLP_WNDPROC, (LONG_PTR) SubAddrDlgProc);
        LastDlgProc = (WNDPROC) GetWindowLongPtr(hEdtLast, GWLP_WNDPROC);
        SetWindowLongPtr(hEdtLast, GWLP_WNDPROC, (LONG_PTR) SubLastDlgProc);
        SetDialogFocus(hEdtAddr);
        break;
    case WM_COMMAND:
        switch (LOWORD(wPrm)) {
        case IDCLOSE:
        case WM_DESTROY:
            DestroyWindow(hDlg);
            hDismWin = NULL;
            break;
        case WM_ACTIVATE:
            SetDialogFocus(hEdtAddr);
            break;
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
LRESULT CALLBACK SubAddrDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=0;
    return ProcEditDlg(AddrDlgProc,hDlg,msg,wPrm,lPrm);
}
LRESULT CALLBACK SubLastDlgProc
    (HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    TabDirection=1;
    return ProcEditDlg(LastDlgProc,hDlg,msg,wPrm,lPrm);
}
BOOL ProcEditDlg
(WNDPROC DlgProc, HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wPrm) {
        case VK_RETURN:
            SendMessage(hEdtAPPY,BM_CLICK,0,0);
            return TRUE;
        case VK_TAB:
            SendMessage(hDismDlg,WM_NEXTDLGCTL,TabDirection,0);
            return TRUE;
        }
    }   return CallWindowProc(DlgProc,hDlg,msg,wPrm,lPrm);
}

/**************************************************/
/*             Get disassembly range              */
/**************************************************/
void DisAsmFromTo()
{
    char buf[16];
    unsigned short from, to;

    GetWindowText(hEdtAddr, buf, 8);
    from = ConvertHexAddress(buf);
    if (from < 0) {
        MessageBox(hDismDlg, "Invalid from address", "Error", 0);
    } else {
        GetWindowText(hEdtLast, buf, 8);
        to = ConvertHexAddress(buf);
        int range = to - from;
        if ((range < 1) || (range > 0x1000)) {
            MessageBox(hDismDlg,
                  "Invalid to address or range error", "Error", 0);
        } else {
            Disassemble(from,to);
        }
    }
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
    unsigned short PC = from;
    while (PC < to) {

        // Get information for opcode or extended opcode
        OpInf = OpCdTbl->Page1OpCodes[MemRead8(PC)];
        if (OpInf.mode == OpCodeTables::AddressingMode::OpPage2) {
            OpInf = OpCdTbl->Page2OpCodes[MemRead8(PC+1)];
        } else if (OpInf.mode == OpCodeTables::AddressingMode::OpPage3) {
            OpInf = OpCdTbl->Page3OpCodes[MemRead8(PC+1)];
        }

        // Use ProcessHeuristics to decode operands.  Kludgey
        // but gets it done without creating new a new method
        // in OpCodeTables
        state.PC = PC;
        trace = {};
        trace.bytes.push_back(OpInf.opcode);

        if (!OpCdTbl->ProcessHeuristics(OpInf, state, trace)) {
            OpInf.numbytes = 1;
            OpInf.name = "???";
            trace.operand = "";
        }
        // Use sprintf to format hex for dump
        std::string HexDmp;
        char cstr[8];
        sprintf(cstr,"%04X\t",PC);
        HexDmp.append(cstr);
        sprintf(cstr,"%02X ",MemRead8(PC++));
        HexDmp.append(cstr);
        for (int i = 1; i < OpInf.numbytes; i++) {
            sprintf(cstr,"%02X ",MemRead8(PC++));
            HexDmp.append(cstr);
        }

//PrintLogC("%s%s%s\n",HexDmp.c_str(),OpInf.name.c_str(),trace.operand.c_str());

        // Blank pad hex dump and instruction name
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
