//------------------------------------------------------------------
// Display VCC memory for debugging
//
// This file is part of VCC (Virtual Color Computer).
// Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software: you can redistribute
// it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
// Memory Map Display - Part of the Debugger package for VCC
// Authors: Mike Rojas, Chet Simpson
// Enhancements: EJJaquay
//------------------------------------------------------------------

#include "MemoryMap.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "tcc1014mmu.h"
#include "resource.h"
#include "pakinterface.h"
#include <vcc/common/logger.h>
#include <vcc/common/DialogOps.h>
#include <fstream>

namespace VCC::Debugger::UI { namespace {

// Local functions
void SetEditing(bool);
void FlashDialogWindow();
void WriteMemory(int,unsigned char);
void SetBackBuffer(const RECT&);
void CreateScrollBar(const RECT&);
void DrawForm(HDC,LPCRECT);
void DrawMemory(HDC,LPCRECT);
void SetEditPosition(int,int);
void LocateMemory();
void ExportMemory();
void CommitValue();
void SetMemType();
void InitializeDialog(HWND);
void DoScroll(WPARAM);
int CStrToHex(const char *);

LRESULT CALLBACK subEditValProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK subEditAdrBegProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK subEditAdrEndProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK MemoryMapDlgProc(HWND,UINT,WPARAM,LPARAM);

// Global handles
HWND hDlgMem = nullptr;
HWND hScrollBar = nullptr;
HWND hEditAdrBeg = nullptr;
HWND hEditAdrEnd = nullptr;
HWND hEditVal = nullptr;

// Original controls
WNDPROC EditValProc;
WNDPROC EditAdrBegProc;
WNDPROC EditAdrEndProc;

// Enum for memory type being examined
enum AddrMode
{
	Cpu,
	Real,
	ROM,
	PAK,
	NotSet
};
AddrMode AddrMode_ = AddrMode::NotSet;

int MemSize = 0;
int memoryOffset = 0;
int selectionRangeBeg = -1;
int selectionRangeEnd = -1;
unsigned char *Rom = nullptr;
bool Editing = false;
int editAddress = 0;

// Backing buffer used for painting memory data
BackBufferInfo BackBuf;

// Data cell X offsets relative to the backing buffer
// Cells are 18 pixels square with a 15 pixel gap between 7 & 8
const int Xoffset[16] =
	{70,88,106,124,142,160,178,196,229,247,265,283,301,319,337,355};

// Help text
char DbgHelp[] =
	"The default memory type displayed is 'CPU'.\n"
	"Dropdown will select 'REAL', 'ROM', or 'PAK'.\n\n"
	"In addition to the scroll bar the mouse wheel,\n"
	"Home, End, PgUp, PgDn, Up, and Down keys\n"
	"will scroll the display.\n\n"
	"Select memory to be edited by clicking on a\n"
	"cell. The cell will turn red and it's address\n"
	"will be displayed next to the box. Enter byte\n"
	"values in hexadecimal.\n";

//------------------------------------------------------------------
//  Display Memory Dialog
//------------------------------------------------------------------
INT_PTR CALLBACK MemoryMapDlgProc(
		HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) {

	case WM_INITDIALOG:
		InitializeDialog(hDlg);
		break;

	case WM_LBUTTONDOWN:
		if (AddrMode_ == AddrMode::PAK)
			FlashDialogWindow();
		else
			SetEditPosition(LOWORD(lParam),HIWORD(lParam));
		break;

	case WM_VSCROLL:
		DoScroll(wParam);
		break;

	case WM_MOUSEWHEEL:
		if ( GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
			DoScroll( (WPARAM) SB_LINEUP);
		} else {
			DoScroll( (WPARAM) SB_LINEDOWN);
		}
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);

		DrawMemory(BackBuf.DeviceContext, &BackBuf.Rect);
		BitBlt(hdc, 0, 0, BackBuf.Width, BackBuf.Height,
				BackBuf.DeviceContext, 0, 0, SRCCOPY);

		EndPaint(hDlg, &ps);
		break;
	}

	case WM_TIMER:
		switch (wParam) {
		case IDT_MEM_TIMER:
			if ( AddrMode_ == AddrMode::Cpu ||
				 AddrMode_ == AddrMode::Real ) {
				InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MEM_TYPE:
			SetMemType();
			InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
			break;
		case IDC_BTN_EXPORT_MEM:
			LocateMemory();
			ExportMemory();
			SetFocus(hEditAdrBeg);
			break;
		case IDC_BTN_HELP:
			MessageBox(hDlg,DbgHelp,"Usage",0);
			SetFocus(hEditAdrBeg);
			break;
		case IDCLOSE:
		case WM_DESTROY:
			KillTimer(hDlg, IDT_MEM_TIMER);
			DeleteDC(BackBuf.DeviceContext);
			DestroyWindow(hDlg);
			AddrMode_ = AddrMode::NotSet;
			hDlgMem = nullptr;
			break;
		}
		break;
	}
	return FALSE;
}

//------------------------------------------------------------------
//  Subclassed Edit Value Proc
//------------------------------------------------------------------
LRESULT CALLBACK subEditValProc(
		HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			CommitValue();
			return 0;
		case VK_TAB:
			SetFocus(hEditAdrBeg);
			return 0;
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			FlashDialogWindow();
			return 0;
		}
	}
	return CallWindowProc(EditValProc, wnd, msg, wParam, lParam);
}

//------------------------------------------------------------------
//  Subclassed Edit Address Proc
//------------------------------------------------------------------
LRESULT CALLBACK subEditAdrBegProc(
		HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			LocateMemory();
			return 0;
		case VK_TAB:
			LocateMemory();
			SendMessage(hEditAdrEnd, EM_SETSEL, 0, -1);
			SetFocus(hEditAdrEnd);
			return 0;
		case VK_UP:
			DoScroll((WPARAM)SB_LINEUP);
			return 0;
		case VK_DOWN:
			DoScroll((WPARAM)SB_LINEDOWN);
			return 0;
		case VK_PRIOR:
			DoScroll((WPARAM)SB_PAGEUP);
			return 0;
		case VK_NEXT:
			DoScroll((WPARAM)SB_PAGEDOWN);
			return 0;
		case VK_HOME:
			DoScroll((WPARAM)SB_TOP);
			return 0;
		case VK_END:
			DoScroll((WPARAM)SB_BOTTOM);
			return 0;
		}
	}
	return CallWindowProc(EditValProc, wnd, msg, wParam, lParam);
}

LRESULT CALLBACK subEditAdrEndProc(
	HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			LocateMemory();
			return 0;
		case VK_TAB:
			LocateMemory();
			SetFocus(hEditAdrBeg);
			return 0;
		case VK_UP:
			DoScroll((WPARAM)SB_LINEUP);
			return 0;
		case VK_DOWN:
			DoScroll((WPARAM)SB_LINEDOWN);
			return 0;
		case VK_PRIOR:
			DoScroll((WPARAM)SB_PAGEUP);
			return 0;
		case VK_NEXT:
			DoScroll((WPARAM)SB_PAGEDOWN);
			return 0;
		case VK_HOME:
			DoScroll((WPARAM)SB_TOP);
			return 0;
		case VK_END:
			DoScroll((WPARAM)SB_BOTTOM);
			return 0;
		}
	}
	return CallWindowProc(EditValProc, wnd, msg, wParam, lParam);
}

//------------------------------------------------------------------
// Read a byte from Coco Memory
//------------------------------------------------------------------
unsigned char ReadMemory(int addr)
{
	switch (AddrMode_) {

	case AddrMode::Cpu:
		return SafeMemRead8(addr & 0xFFFF);

	case AddrMode::Real:
		return (unsigned char) GetMem(addr);

	case AddrMode::ROM:
		if (Rom == nullptr) Rom = Getint_rom_pointer();
		return Rom[addr & 0x7FFF];

	case AddrMode::PAK:
		return PackMem8Read(addr & 0x7FFF);
	}
	return 0;
}

//------------------------------------------------------------------
// Write a byte to Coco Memory
//------------------------------------------------------------------
void WriteMemory(int addr, unsigned char value)
{
	switch (AddrMode_) {

	case AddrMode::Cpu:
		EmuState.Debugger.QueueWrite(addr & 0xFFFF, value);
		break;

	case AddrMode::Real:
		SetMem(addr,value);
		break;

	case AddrMode::ROM:
		if (Rom == nullptr) Rom = Getint_rom_pointer();
		Rom[addr & 0x7FFF] = value;
		break;

	case AddrMode::PAK:
		FlashDialogWindow();
	}
}

//------------------------------------------------------------------
// Setup back buffer for data display
//------------------------------------------------------------------
void SetBackBuffer(const RECT& rc)
{
	HDC hdc = GetDC(hDlgMem);
	FillRect(hdc, &rc, (HBRUSH) (COLOR_WINDOW+1));

	// Adjust backing buffer location on client
	BackBuf.Rect.left   = rc.left;
	BackBuf.Rect.right  = rc.right  - 22;
	BackBuf.Rect.top    = rc.top    + 34;
	BackBuf.Rect.bottom = rc.bottom + 34;

	BackBuf.Width  = BackBuf.Rect.right  - BackBuf.Rect.left;
	BackBuf.Height = BackBuf.Rect.bottom - BackBuf.Rect.top;

	BackBuf.Bitmap = CreateCompatibleBitmap
				(hdc, BackBuf.Width, BackBuf.Height);
	
	BackBuf.DeviceContext = CreateCompatibleDC(hdc);
	HBITMAP old = (HBITMAP) SelectObject
				(BackBuf.DeviceContext, BackBuf.Bitmap);
	DeleteObject(old);
	ReleaseDC(hDlgMem, hdc);
}

//------------------------------------------------------------------
// Create vertical scroll bar
//------------------------------------------------------------------
void CreateScrollBar(const RECT& Rect)
{
		hScrollBar = CreateWindowEx(
			0,
			"SCROLLBAR",
			nullptr,
			WS_VISIBLE | WS_CHILD | SBS_VERT,
			Rect.right - 21,   //top x
			38,                //top y
			20,                //width
			Rect.bottom - 42,  //height
			hDlgMem,
			(HMENU)IDC_MEM_VSCROLLBAR,
			(HINSTANCE)GetWindowLong(hDlgMem, GWL_HINSTANCE),
			nullptr);

		if (!hScrollBar) {
			MessageBox(nullptr, "Vertical Scroll Bar Failed.", "Error",
				MB_OK | MB_ICONERROR);
		}
}

//------------------------------------------------------------------
// Draw display form with header and vert guide lines
//------------------------------------------------------------------
void DrawForm(HDC hdc,LPCRECT clientRect)
{
	int top = clientRect->top;
	int lft = clientRect->left;
	int rgt = clientRect->right;
	int bot = clientRect->bottom;
	RECT rc;

	// Clear background.
	HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hdc, clientRect, brush);

	// Format for text
	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	// Draw separator lines for border, address, and ascii
	MoveToEx(hdc,lft,top,nullptr);     LineTo(hdc,rgt,top);
	MoveToEx(hdc,lft,top+20,nullptr);  LineTo(hdc,rgt,top+20);
	MoveToEx(hdc,lft+1,top,nullptr);   LineTo(hdc,lft+1,bot-1);
	MoveToEx(hdc,lft+60,top,nullptr);  LineTo(hdc,lft+60,bot-1);
	MoveToEx(hdc,rgt-135,top,nullptr); LineTo(hdc,rgt-135,bot);
	MoveToEx(hdc,rgt-1,top,nullptr);   LineTo(hdc,rgt-1,bot);

	// Horizontal separators every four rows
	int ltop = top + 20;
	for (int lnum = 3; lnum < 32; lnum+=4) {
		ltop += 18*4;
		MoveToEx(hdc,lft,ltop,nullptr); LineTo(hdc,rgt,ltop);
	}

	// Draw header
	SetTextColor(hdc, RGB(138,27,255));
	SetRect(&rc,lft,top,lft+60,top+20);
	DrawText(hdc, "Address", 7, &rc, fmt);
	for (int n = 0; n < 16; n++) {
		SetRect(&rc, lft+Xoffset[n], top, lft+Xoffset[n]+15, top+20);
		const std::string s(ToHexString(n, 2, false));
		DrawText(hdc, s.c_str(), 2, &rc, fmt);
	}
	SetRect(&rc, rgt - 135, top, rgt - 5, top + 20);
	DrawText(hdc, "ASCII", 6, &rc, fmt);
}

//------------------------------------------------------------------
// Fill memory data on form
//------------------------------------------------------------------
void DrawMemory(HDC hdc, LPCRECT clientRect)
{
	int top = clientRect->top;
	int lft = clientRect->left;
	int rgt = clientRect->right;
	RECT rc;

	bool hlfound = false;
	std::string s;

	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	int ltop = top + 20; // Back buff relative

	for (int lnum = 0; lnum < 32; lnum++) {
		
		// Draw address of start of line
		SetTextColor(hdc, RGB(138, 27, 255));
		s = ToHexString(memoryOffset + lnum * 16, 6, true);
		SetRect(&rc, lft, ltop, lft+64, ltop+18);
		DrawText(hdc, s.c_str(), s.size(), &rc, fmt);

		// Fill in data cells
		SetTextColor(hdc, RGB(0, 0, 0));
		int hilite = false;

		std::string ascii;
		for (int n = 0; n < 16; n++) {

            // Get data
			int adr = lnum * 16 + n + memoryOffset;
			unsigned char val = ReadMemory(adr);

			// Get hex string to display.
			s = ToHexString(val, 2, true);

			// Highlight data if cell is being edited
			if (Editing && editAddress == adr) {
				SetTextColor(hdc, RGB(255, 0, 0));  // Red
				hilite = true;
			}

			SetRect(&rc,lft+Xoffset[n],ltop,lft+Xoffset[n]+18,ltop+18);
			DrawText(hdc, s.c_str(), 2, &rc, fmt);

			// if highlight on turn it off
			if (hilite) {
				SetTextColor(hdc, RGB(0, 0, 0));
				hlfound = true;
			}

			// Append to ascii
			if (isprint(val)) ascii += val; else ascii += ".";
		}

		// Draw the ascii string
		SetRect(&rc, rgt-133, ltop, rgt-5, ltop+18);
		DrawText(hdc, ascii.c_str(), ascii.size(), &rc, fmt);

		ltop += 18;
	}

	// Not editmode if no cell highlighted.
	if (Editing && !hlfound) {
		SetEditing(false);
	}
}

//------------------------------------------------------------------
//  Determine byte to edit based on click location
//------------------------------------------------------------------
void SetEditPosition(int xPos, int yPos)
{
	int xStart = 70; // X Start of data cells
	int yStart = 55; // Y Start of data cells (dialog relative)

	// Cells have a dead zone between col 7 and 8
	int leftDeadArea  = xStart + (8 * 18);
	int rightDeadArea = leftDeadArea + 15;
	int xMax = rightDeadArea + (8 * 18);

	// Stop edit if location is not in cell area
	if ( yPos < yStart || xPos < xStart || xPos > xMax ||
		(xPos >= leftDeadArea && xPos <= rightDeadArea) ) {
		SetEditing(false);
		return;
	}

	// Determine address per cell (18x18) row and column
	int row = (yPos - yStart) / 18;

	int col;
	if (xPos < leftDeadArea)
		col = (xPos - xStart) / 18;
	else
		col = (xPos - rightDeadArea) / 18 + 8;

	int addr = memoryOffset + col + row * 16;

	editAddress = addr;
	SetEditing(true);

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}

//------------------------------------------------------------------
// Determine data to display based on address box
//------------------------------------------------------------------
void LocateMemory()
{
	char buf[32] = {0};

	SendDlgItemMessage(hDlgMem, IDC_EDIT_RANGE_BEG, WM_GETTEXT,
			sizeof(buf), (LPARAM) buf);

	selectionRangeBeg = CStrToHex(buf);

	SendDlgItemMessage(hDlgMem, IDC_EDIT_RANGE_END, WM_GETTEXT,
		sizeof(buf), (LPARAM)buf);

	selectionRangeEnd = CStrToHex(buf);

	if (selectionRangeEnd < selectionRangeBeg) {
		selectionRangeEnd = selectionRangeBeg;
	}

	std::string begstr = ToHexString(selectionRangeBeg, 6, true);
	std::string endstr = ToHexString(selectionRangeEnd, 6, true);

	SetDlgItemText(hDlgMem, IDC_EDIT_RANGE_BEG, begstr.c_str());
	SetDlgItemText(hDlgMem, IDC_EDIT_RANGE_END, endstr.c_str());

	if (selectionRangeBeg < 0) {
		selectionRangeBeg = selectionRangeEnd = -1;
		FlashDialogWindow();
		SetEditing(false);
		return;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hScrollBar, SB_CTL, &si);

	si.nPos = roundDn(selectionRangeBeg, 16);
	si.fMask = SIF_POS;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
	GetScrollInfo(hScrollBar, SB_CTL, &si);

	memoryOffset = si.nPos;

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}

//------------------------------------------------------------------
// Export the selected range to disk
//------------------------------------------------------------------
void ExportMemory()
{
	if (selectionRangeBeg < 0 || selectionRangeEnd < 0) {
		FlashDialogWindow();
		SetEditing(false);
		return;
	}

	FileDialog dlg;
	dlg.setFilter("BIN\0*.bin\0\0");
	dlg.setDefExt("bin");
	dlg.setTitle(TEXT("Export Memory Range"));
	dlg.setFlags(OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT);

	if (dlg.show(1)) {
		std::ofstream fout(dlg.path(), std::ios::out | std::ios::trunc
			| std::ios::binary);

		for (int adr = selectionRangeBeg; adr <= selectionRangeEnd; adr++) {
			unsigned char val = ReadMemory(adr);
			fout.write(reinterpret_cast<const char *>(&val), sizeof(val));
		}

		MessageBox(hDlgMem, "Export Complete", "Export", 0);
	}
}

//------------------------------------------------------------------
//  Commit edit value to memory
//------------------------------------------------------------------
void CommitValue()
{
	if (!Editing) {
		FlashDialogWindow();
		SetDlgItemText(hDlgMem, IDC_EDIT_VALUE, "");
		return;
	}

	// Fetch then clear the data to commit
	char buf[32] = {0};
	SendDlgItemMessage(hDlgMem, IDC_EDIT_VALUE, WM_GETTEXT,
			sizeof(buf), (LPARAM) buf);
	SetDlgItemText(hDlgMem, IDC_EDIT_VALUE, "");

	int val = CStrToHex(buf);
	if (val < 0 || val > 255) {
		FlashDialogWindow();
		return;
	}

	// Commit the value to memory
	WriteMemory(editAddress, (unsigned char) val);
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);

	// Attempt to advance to next cell
	editAddress += 1;
	if (editAddress >= MemSize) {
		FlashDialogWindow();
		editAddress = 0;
		SetEditing(false);
	} else {
		SetEditing(true);
	}
}

//------------------------------------------------------------------
// Set memory type and size as per combobox index
//------------------------------------------------------------------
void SetMemType()
{
	int PhySiz[4]={0x20000,0x80000,0x200000,0x800000};

	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_TYPE);
	AddrMode mode = (AddrMode) SendMessage(hCtl,CB_GETCURSEL,0,0);
	if (AddrMode_ == mode) return;  // Not changed

	memoryOffset = 0;

	AddrMode_ = mode;
	switch (AddrMode_) {
	case AddrMode::Cpu:
		MemSize = 0x10000;
		break;
	case AddrMode::Real:
		MemSize = PhySiz[EmuState.RamSize];
		break;
	case AddrMode::ROM:
		MemSize = 0x8000;
		break;
	case AddrMode::PAK:
		MemSize = 0x8000;
		break;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_POS;
	si.nMin = 0;
	si.nPage = 32 * 16;  // 32 lines of 16 bytes
	si.nMax = MemSize - si.nPage;
	si.nPos = 0;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);

	SetEditing(false);
}

//------------------------------------------------------------------
// Memory Dialog initialization
//------------------------------------------------------------------
void InitializeDialog(HWND hDlg)
{
		hDlgMem = hDlg;

		RECT Rect;
		GetClientRect(hDlg, &Rect);

		SetBackBuffer(Rect);
		CreateScrollBar(Rect);

		//Subclass edit boxes
		hEditAdrBeg = GetDlgItem(hDlg, IDC_EDIT_RANGE_BEG);
		EditAdrBegProc = (WNDPROC) SetWindowLongPtr
				(hEditAdrBeg, GWLP_WNDPROC, (LONG_PTR) subEditAdrBegProc);

		hEditAdrEnd = GetDlgItem(hDlg, IDC_EDIT_RANGE_END);
		EditAdrEndProc = (WNDPROC)SetWindowLongPtr
				(hEditAdrEnd, GWLP_WNDPROC, (LONG_PTR)subEditAdrEndProc);

		hEditVal = GetDlgItem(hDlg, IDC_EDIT_VALUE);
		EditValProc = (WNDPROC) SetWindowLongPtr
				(hEditVal, GWLP_WNDPROC, (LONG_PTR) subEditValProc);

		SetTimer(hDlg, IDT_MEM_TIMER, 64, nullptr);

		// Dropdown to select memory type displayed
		HWND hCtl = GetDlgItem(hDlg, IDC_MEM_TYPE);
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "CPU");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "REAL");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "ROM");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "PAK");
		SendMessage(hCtl,CB_SETCURSEL,(WPARAM) 0, (LPARAM) 0);
		SetMemType();

		// Set display pen color
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
		SelectObject(BackBuf.DeviceContext, pen);
		DeleteObject(pen);

		// Set display Font
		HFONT hFont = CreateFont (14, 0, 0, 0, FW_BOLD, FALSE, FALSE,
				FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
				CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
				TEXT("Consolas"));
		SelectObject(BackBuf.DeviceContext, hFont);
		DeleteObject(hFont);

		// Draw the form for memory data
		DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);

		// Not edit mode
		SetEditing(false);
}

//------------------------------------------------------------------
//  Scroll handler
//------------------------------------------------------------------
void DoScroll(WPARAM wParam)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hScrollBar, SB_CTL, &si);
	switch ((int)LOWORD(wParam)) {
	case SB_PAGEUP:
		si.nPos -= 16*32;
		break;
	case SB_PAGEDOWN:
		si.nPos += 16*32;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	case SB_TOP:
		si.nPos = 0;
		break;
	case SB_BOTTOM:
		si.nPos = si.nMax;
		break;
	case SB_ENDSCROLL:
		break;
	case SB_LINEUP:
		si.nPos -= 16;
		break;
	case SB_LINEDOWN:
		si.nPos += 16;
		break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
	GetScrollInfo(hScrollBar, SB_CTL, &si);
	memoryOffset = si.nPos;

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}

//------------------------------------------------------------------
// Convert hexadecimal string to a positive long. Return -1 on error
//------------------------------------------------------------------
int CStrToHex(const char * buf)
{
	char *p;
	if (*buf == 0) return -1;
	//long n = strtoul(buf, &p, 16);
	int n = strtoul(buf, &p, 16);
	if (*p != 0) return -1;
	return n;
}

//------------------------------------------------------------------
//  Set edit mode
//------------------------------------------------------------------
void SetEditing(bool tf) {
	Editing = tf;
	if (Editing) {
		std::string s = "Editing " + ToHexString(editAddress,6,true);
		SetDlgItemText(hDlgMem, IDC_ADRTXT, s.c_str());
		SetFocus(hEditVal);
	} else {
		SetDlgItemText(hDlgMem, IDC_ADRTXT, "");
		SetFocus(hEditAdrBeg);
	}
	SetDlgItemText(hDlgMem, IDC_EDIT_VALUE, "");
}

//------------------------------------------------------------------
//  Input error flash
//------------------------------------------------------------------
void FlashDialogWindow()
{
	FlashWindow(hDlgMem,true);
	Sleep(350);
	FlashWindow(hDlgMem,false);
}

} }  // end namespace

//------------------------------------------------------------------
// Launch Memory Dialog
//------------------------------------------------------------------
void VCC::Debugger::UI::OpenMemoryMapWindow(HINSTANCE hInst,HWND parent)
{
	if (hDlgMem == nullptr) {
		CreateDialog( hInst, MAKEINTRESOURCE(IDD_MEMORY_MAP),
		              parent, MemoryMapDlgProc );
	}

	if (hDlgMem == nullptr)
	{
		MessageBox(nullptr, "CreateDialog", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	ShowWindow(hDlgMem, SW_SHOWNORMAL);
	SetFocus(hEditAdrBeg);
}

