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
UINT16 HexToUint(char *);
LRESULT CALLBACK SubAddrDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC AddrDlgProc;
LRESULT CALLBACK SubBlocDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC BlocDlgProc;
LRESULT CALLBACK SubLcntDlgProc(HWND,UINT,WPARAM,LPARAM);
WNDPROC LcntDlgProc;
BOOL ProcEditDlg(WNDPROC,HWND,UINT,WPARAM,LPARAM,int);
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR Proc);
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

        // Hook the input dialogs to capture keystrokes
        AddrDlgProc = (WNDPROC) SetControlHook(hEdtAddr,(LONG_PTR) SubAddrDlgProc);
        BlocDlgProc = (WNDPROC) SetControlHook(hEdtBloc,(LONG_PTR) SubBlocDlgProc);
        LcntDlgProc = (WNDPROC) SetControlHook(hEdtLcnt,(LONG_PTR) SubLcntDlgProc);

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

    // An edit box must have focus to capture tab/enter/up/down etc
    case WM_PAINT:
        SetFocus(hEdtAddr);
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
            SetFocus(hEdtAddr);
            return TRUE;
        case IDC_PHYS_MEM:
            if (IsDlgButtonChecked(hDlg,IDC_PHYS_MEM)) {
                EnableWindow(hEdtBloc,TRUE);
                SetWindowText(GetDlgItem(hDismDlg,IDC_ADRTXT),"  Offset:");
                SetFocus(hEdtBloc);
            } else {
                EnableWindow(hEdtBloc,FALSE);
                SetWindowText(GetDlgItem(hDismDlg,IDC_ADRTXT),"Address:");
                SetFocus(hEdtAddr);
            }
            return TRUE;
        }
    }   return FALSE;
}

/***************************************************/
/*         Set up edit control hooks               */
/***************************************************/
LONG_PTR SetControlHook(HWND hCtl,LONG_PTR HookProc)
{
    LONG_PTR OrigProc=GetWindowLongPtr(hCtl,GWLP_WNDPROC);
    SetWindowLongPtr(hCtl,GWLP_WNDPROC,HookProc);
    return OrigProc;
}
// Address edit control
LRESULT CALLBACK SubAddrDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(AddrDlgProc,hDlg,msg,wPrm,lPrm,1); }
// Line count edit control
LRESULT CALLBACK SubLcntDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(LcntDlgProc,hDlg,msg,wPrm,lPrm,2); }
// Block number edit control
LRESULT CALLBACK SubBlocDlgProc(HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm)
{ return ProcEditDlg(BlocDlgProc,hDlg,msg,wPrm,lPrm,0); }

/***************************************************/
/*      Process edit control messages here         */
/***************************************************/
BOOL ProcEditDlg (WNDPROC pDlg,HWND hDlg,UINT msg,WPARAM wPrm,LPARAM lPrm,int next)
{
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
            switch (next) {
            case 1:
                SetFocus(hEdtLcnt);
                break;
            case 2:
                if (IsDlgButtonChecked(hDismDlg,IDC_PHYS_MEM)) {
                    SetFocus(hEdtBloc);
                    break;
                }
            default:
                SetFocus(hEdtAddr);
                break;
            }
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
    }
    // Everything else sent to original dialog processing
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
    Disassemble(FromAdr,MaxLines,UsePhyAdr,Block);
    return;
}

/***************************************************/
/*   Right pad string to fixed size                */
/***************************************************/
std::string PadRight(std::string const& s, size_t w)
{
    if (s.size() < w)
        return s + std::string(w-s.size(),' ');
    else
        return s;
}

/***************************************************/
/*    Convert integer to zero filled hex string    */
/***************************************************/
std::string IntToHex(int i, int w)
{
    std::ostringstream fmt;
    fmt<<std::hex<<std::setw(w)<<std::uppercase<<std::setfill('0')<<i;
    return fmt.str();
}

/***************************************************/
/*           Create Disassembly line               */
/***************************************************/
std::string DisLine(int adr,std::string ins,std::string opc,std::string opr)
{
    return IntToHex(adr,6) + " " + PadRight(ins,11) +
           PadRight(opc,5) + PadRight(opr,6);
}

/***************************************************/
/*        Create FDB line with comment             */
/***************************************************/
std::string OpFDB(int adr,std::string b1,std::string b2,std::string cmt)
{
    return DisLine(adr,b1+b2,"FDB","$"+b1+b2) +cmt+"\n";
}

/***************************************************/
/*        Create FCB line with comment             */
/***************************************************/
std::string OpFCB(int adr,std::string b,std::string cmt)
{
    return DisLine(adr,b,"FCB","$"+b) +cmt+"\n";
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
    *hdr =  OpFDB(addr+cnt++,hexb[0],hexb[1],"Module"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[2],hexb[3],"Mod Size"); cnt++;
    *hdr += OpFDB(addr+cnt++,hexb[4],hexb[5],"Off Nam"); cnt++;
    *hdr += OpFCB(addr+cnt++,hexb[6],"Ty/Lg");
    *hdr += OpFCB(addr+cnt++,hexb[7],"At/Rv");
    *hdr += OpFCB(addr+cnt++,hexb[8],"Parity");
    // Additional lines if executable
    char type = hexb[6][0];
    if ((type=='1')||(type=='E')) {
        for (int n=0; n<4; n++) {
            int ch = GetMem(cnt+n+addr);
            hexb[n+9] = IntToHex(ch,2);
        }
        *hdr += OpFDB(addr+cnt++,hexb[9],hexb[10],"Off Exe"); cnt++;
        *hdr += OpFDB(addr+cnt++,hexb[11],hexb[12],"Dat Size"); cnt++;
    }
    return cnt;
}

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

    SetWindowTextA(hDisText,"");

    unsigned short PC = FromAdr;
    int numlines = 0;
    int modhdr = 0;

    state.block = Block;
    state.phyAddr = UsePhyAddr;
    unsigned short MaxAdr = UsePhyAddr? 0xFFFF: 0xFF00;
    while ((numlines < MaxLines) && (PC < MaxAdr)) {

        if (UsePhyAddr) {
            // Check for module header
            std::string hdr;
            modhdr = DecodeModHdr(PC+Block*0x2000, &hdr);
            if (modhdr) {
                PC += modhdr;
                if (modhdr > 9)
                    numlines += 8;
                else
                    numlines += 6;
            }
            lines += hdr;
        }

        state.PC = PC;
        trace = {};
        Decoder->DecodeInstruction(state,trace);

        // If a module convert SWI2 to OS9 with immediate operand
        if (modhdr && (trace.instruction == "SWI2")) {
            unsigned short opadr = PC + (unsigned short) trace.bytes.size();
            unsigned char op = DbgRead8(UsePhyAddr, Block, opadr);
            trace.instruction = "OS9";
            trace.operand = "#$"+IntToHex(op,2);
            trace.bytes.push_back(op);
        }

        // Instruction address
        int iaddr;
        if (UsePhyAddr)
            iaddr = PC + Block * 0x2000;
        else
            iaddr = PC;

        // Create string of instruction bytes
        std::string HexInst;
        for (unsigned int i=0; i < trace.bytes.size(); i++) {
            HexInst += IntToHex(trace.bytes[i],2);
        }

        // append disassembled line for output
        lines += DisLine(iaddr,HexInst,trace.instruction,trace.operand)+"\n";
        numlines++;

        // Next instruction
        if (trace.bytes.size() > 0)
            PC += (UINT16) trace.bytes.size();
        else
            PC += 1;
    }
    // Put the completed disassembly
    SetWindowTextA(hDisText,ToLPCSTR(lines));
    if (PC >= MaxAdr)
        SetWindowText(hErrText,"Stopped at invalid address");
    else
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
