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
#include "tcc1014graphics.h"
#include <vcc/util/logger.h>
#include <vcc/util/DialogOps.h>
#include <fstream>

namespace VCC::Debugger::UI {
namespace {

// Local functions
void SetEditing(bool);
void FlashDialogWindow();
void WriteMemory(int,unsigned char);
void SetBackBuffer(const RECT&);
void CreateScrollBar(const RECT&);
void DrawForm(HDC,LPCRECT);
bool DrawMemory(HDC,LPCRECT);
void SetEditPosition(int,int);
void LocateMemory();
void ExportMemory();
void CommitValue();
void SetMemType();
void InitializeDialog(HWND);
void DoScroll(WPARAM);
int CStrToHex(const char *);
void ResetMemoryCache();
void ResizeWindow(int width, int height);

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
HWND hStatic = nullptr;

// Original controls
WNDPROC EditValProc;
WNDPROC EditAdrBegProc;
WNDPROC EditAdrEndProc;

GimeGpu memGpu;
unsigned char *ramCache = nullptr;
unsigned int ramPos = -1;

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

enum ViewMode
{
	VM_ASCII,
	VM_SG4,
	VM_PMODE4_NTSC,
	VM_PMODE4_RGB
};
ViewMode viewMode = VM_SG4;

const int HeaderHeight = 20;
const int TopBarStaticWidth = 510;
const int TopBarHeight = 34;
const int ScrollBarWidth = 20;

int MemSize = 0;
int memoryOffset = 0;
int selectionRangeBeg = -1;
int selectionRangeEnd = -1;
unsigned char *Rom = nullptr;
bool Editing = false;
int editAddress = 0;

struct MemoryBackBufferInfo : BackBufferInfo
{
	HPEN Pen = nullptr;
	HFONT Font = nullptr;

	HBITMAP FontBitmap = nullptr;
	HBITMAP FontRedBitmap = nullptr;
	HBITMAP FontAsciiBitmap = nullptr;
	HDC FontDC = nullptr;
	HDC FontRedDC = nullptr;
	HDC FontAsciiDC = nullptr;

	int DataWidth = 0;
	int DataHeight = 0;
	int DataStride = 0;
	char* Data = nullptr;

	void Init();
	void CleanupFonts(HWND hWnd);
	void CleanupPen();
	void CleanupFont();
	void Cleanup(HWND hWnd);
};

// Backing buffer used for painting memory data
MemoryBackBufferInfo BackBuf;

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


void ResetMemoryCache()
{
	ramPos = -1;
	delete[] ramCache;
	ramCache = nullptr;
}

void MemoryBackBufferInfo::Init()
{
	DataWidth = 640;
	DataHeight = 32;
	DataStride = DataWidth * 4;
	Data = new char[DataStride * DataHeight];
}

void MemoryBackBufferInfo::CleanupPen()
{
	DeleteObject(Pen);
	Pen = nullptr;
}

void MemoryBackBufferInfo::CleanupFont()
{
	DeleteObject(Font);
	Font = nullptr;
}

void MemoryBackBufferInfo::CleanupFonts(HWND hWnd)
{
	DeleteObject(FontBitmap);
	DeleteObject(FontRedBitmap);
	DeleteObject(FontAsciiBitmap);
	ReleaseDC(hWnd, FontDC);
	DeleteDC(FontDC);
	FontDC = nullptr;
	ReleaseDC(hWnd, FontRedDC);
	DeleteDC(FontRedDC);
	FontRedDC = nullptr;
	ReleaseDC(hWnd, FontAsciiDC);
	DeleteDC(FontAsciiDC);
	FontAsciiDC = nullptr;
}

void MemoryBackBufferInfo::Cleanup(HWND hWnd)
{
	delete[] Data;
	CleanupFonts(hWnd);
	CleanupFont();
	CleanupPen();
	CleanupBitmap();
	CleanupDC(hWnd);
}

void SetViewType()
{
	HWND hCtl = GetDlgItem(hDlgMem, IDC_VIEW_TYPE);
	ViewMode mode = (ViewMode)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
	if (viewMode == mode) return;
	viewMode = mode;
	ResetMemoryCache();
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);

	// clear backbuffer
	HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(BackBuf.DeviceContext, &BackBuf.Rect, brush);

	// redraw headers
	DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);
}

//------------------------------------------------------------------
//  Display Memory Dialog
//------------------------------------------------------------------
INT_PTR CALLBACK MemoryMapDlgProc(
		HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) {
	case WM_ERASEBKGND:
		return 1;

	case WM_INITDIALOG:
		InitializeDialog(hDlg);
		break;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;

		// Set minimum tracking size (width, height)
		mmi->ptMinTrackSize.x = 540; // Minimum width in pixels
		mmi->ptMinTrackSize.y = 540; // Minimum height in pixels

		return 0;
	}

	case WM_SIZE:
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		ResizeWindow(width, height);
		return 0;
	}

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
	{
		auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
		auto amount = (delta / 30) != 0 ? (delta / 30) : delta;
		auto param = amount > 0 ? (WPARAM)SB_LINEUP : (WPARAM)SB_LINEDOWN;
		for (int i = 0; i < std::abs(amount); ++i)
			DoScroll((WPARAM)param);
		break;
	}

	case WM_PAINT: 
	{
		if (DrawMemory(BackBuf.DeviceContext, &BackBuf.Rect))
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			BitBlt(hdc, 0, 0, BackBuf.Width, BackBuf.Height, BackBuf.DeviceContext, 0, 0, SRCCOPY);
			EndPaint(hDlg, &ps);
		}
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
		switch (LOWORD(wParam)) 
		{
			case IDC_MEM_TYPE:
				SetMemType();
				ResetMemoryCache();
				InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
				break;
			case IDC_VIEW_TYPE:
				SetViewType();
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
				ResetMemoryCache();
				break;
		}
		break;
	}
	return FALSE;
}

void ResizeWindow(int width, int height)
{
	RECT Rect;
	GetClientRect(hDlgMem, &Rect);

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);

	// recreate the back buffer
	ResetMemoryCache();
	SetBackBuffer(Rect);
	DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);

	// reposition scroll bar
	MoveWindow(hScrollBar, Rect.right - ScrollBarWidth, TopBarHeight, ScrollBarWidth, Rect.bottom - TopBarHeight, TRUE);
	MoveWindow(hStatic, Rect.left, 0, Rect.right, TopBarHeight, TRUE);
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

	RECT frc;
	SetRect(&frc, 0, 0, 7 * 16, 14);

	if (!BackBuf.Font)
	{
		// Set display Font
		BackBuf.Font = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE,
			FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
			TEXT("Consolas"));
	}

	UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE;

	BackBuf.FontDC = CreateCompatibleDC(hdc);
	BackBuf.FontBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(BackBuf.FontDC, BackBuf.FontBitmap);
	SelectObject(BackBuf.FontDC, BackBuf.Font);
	DrawText(BackBuf.FontDC, "0123456789ABCDEF", 32, &frc, fmt);

	BackBuf.FontRedDC = CreateCompatibleDC(hdc);
	BackBuf.FontRedBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(BackBuf.FontRedDC, BackBuf.FontRedBitmap);
	SelectObject(BackBuf.FontRedDC, BackBuf.Font);
	SetTextColor(BackBuf.FontRedDC, RGB(255, 0, 0));  // Red
	DrawText(BackBuf.FontRedDC, "0123456789ABCDEF", 32, &frc, fmt);

	frc.right = 7 * 96;
	BackBuf.FontAsciiDC = CreateCompatibleDC(hdc);
	BackBuf.FontAsciiBitmap = CreateCompatibleBitmap(hdc, 7 * 96, 14);
	SelectObject(BackBuf.FontAsciiDC, BackBuf.FontAsciiBitmap);
	SelectObject(BackBuf.FontAsciiDC, BackBuf.Font);
	DrawText(BackBuf.FontAsciiDC, "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[.]^_`abcdefghijklmnopqrstuvwxyz{|}~.", 96, &frc, fmt);

	RECT topBar;
	topBar.left = rc.left + TopBarStaticWidth;
	topBar.right = rc.right;
	topBar.top = rc.top;
	topBar.bottom = TopBarHeight;

	// Adjust backing buffer location on client
	BackBuf.Rect.left   = rc.left;
	BackBuf.Rect.right  = rc.right  - ScrollBarWidth;
	BackBuf.Rect.top    = rc.top    + TopBarHeight;
	BackBuf.Rect.bottom = rc.bottom + TopBarHeight;

	BackBuf.Width  = BackBuf.Rect.right  - BackBuf.Rect.left;
	BackBuf.Height = BackBuf.Rect.bottom - BackBuf.Rect.top;

	BackBuf.CleanupBitmap();
	BackBuf.CleanupDC(hDlgMem);

	BackBuf.DeviceContext = CreateCompatibleDC(hdc);
	BackBuf.Bitmap = CreateCompatibleBitmap(hdc, BackBuf.Width, BackBuf.Height);
	
	HBITMAP old = (HBITMAP) SelectObject(BackBuf.DeviceContext, BackBuf.Bitmap);
	DeleteObject(old);
	ReleaseDC(hDlgMem, hdc);

	if (!BackBuf.Pen)
	{
		// Set display pen color
		BackBuf.Pen = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
	}
	SelectObject(BackBuf.DeviceContext, BackBuf.Pen);

	SelectObject(BackBuf.DeviceContext, BackBuf.Font);
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
			Rect.right - ScrollBarWidth,   //top x
			TopBarHeight,      //top y
			ScrollBarWidth,     //width
			Rect.bottom - TopBarHeight,  //height
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
	int width = rgt - lft;
	int column2Width = width - 388;
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
	MoveToEx(hdc,rgt-column2Width-2,top,nullptr); LineTo(hdc,rgt-column2Width-2,bot);
	MoveToEx(hdc,rgt-1,top,nullptr);   LineTo(hdc,rgt-1,bot);

	// Horizontal separators every four rows
	int height = clientRect->bottom - top - HeaderHeight;
	int lineHeight = (height / 32) - 1;
	int ltop = top + 28 - lineHeight/2;

	for (int lnum = 0; lnum < 32; lnum+=4) {
		ltop += lineHeight * 4;
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
	SetRect(&rc, rgt - column2Width - 2, top, rgt - 5, top + 20);
	const char* viewModes[] = { "ASCII", "Semi Graphics 4", "PMODE 4 NTSC", "PMODE 4 RGB"};
	DrawText(hdc, viewModes[viewMode], strlen(viewModes[viewMode]), &rc, fmt);
}

//------------------------------------------------------------------
// Fill memory data on form
//------------------------------------------------------------------
bool DrawMemory(HDC hdc, LPCRECT clientRect)
{
	memGpu.GimeReset();
	memGpu.SetCompatMode(1);

	bool showAscii = viewMode == VM_ASCII;

	if (viewMode == VM_SG4)
	{
		memGpu.SetVidMask(524287);
	}
	else if (viewMode == VM_PMODE4_NTSC)
	{
		memGpu.SetMonitorType(0);
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(31);
	}
	else if (viewMode == VM_PMODE4_RGB)
	{
		memGpu.SetMonitorType(1);
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(31);
	}

	memGpu.SetupDisplay();
	memGpu.VertCenter = 0;
	memGpu.HorzCenter = 0;

	int top = clientRect->top;
	int lft = clientRect->left;
	int rgt = clientRect->right;
	int height = clientRect->bottom - top - HeaderHeight;
	int width = rgt - lft;
	int lineHeight = (height / 32) - 1;
	int column2Width = width - 388;

	bool hlfound = false;
	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	int ltop = top + HeaderHeight; // Back buff relative
	bool dirty = false;
	bool forcedUpdate = false;
	int dataWidth = 16;
	int pixelHeight = memGpu.LinesperRow;

	if (!ramCache)
	{
		forcedUpdate = true;
		ramCache = new unsigned char[dataWidth*32];
	}

	// if not the same address then it will need repainting
	if (memoryOffset != ramPos)
	{
		dirty = true;
		ramPos = memoryOffset;
	}

	for (int lnum = 0; lnum < 32; lnum++, ltop += lineHeight) 
	{
		unsigned int offset = lnum * dataWidth;
		unsigned int address = memoryOffset + offset;

		int x = lft + 10;
		int y = ltop + 2;

		// Draw address of start of line
		BitBlt(hdc, x, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 20) & 15), 0, SRCCOPY);
		BitBlt(hdc, x + 7, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 16) & 15), 0, SRCCOPY);
		BitBlt(hdc, x + 14, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 12) & 15), 0, SRCCOPY);
		BitBlt(hdc, x + 21, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 8) & 15), 0, SRCCOPY);
		BitBlt(hdc, x + 28, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 4) & 15), 0, SRCCOPY);
		BitBlt(hdc, x + 35, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 0) & 15), 0, SRCCOPY);

		bool update = Editing || forcedUpdate;
		for (int a = 0; a < dataWidth; ++a)
		{
			auto b = ReadMemory(address + a);
			if (b != ramCache[offset + a])
				ramCache[offset + a] = b, update = true;
		}

		// skip nothing to update
		if (!update) continue;

		dirty = true;

		for (int n = 0; n < dataWidth; n++)
		{
			// Get data
			unsigned char val = ramCache[offset + n];

			// Highlight data if cell is being edited
			bool isRed = Editing && editAddress == address + n;
			if (isRed) hlfound = true;

			x = lft + Xoffset[n] + 3;
			y = ltop + 2;
			BitBlt(hdc, x, y, 7, 12, isRed ? BackBuf.FontRedDC : BackBuf.FontDC, 7 * (val >> 4), 0, SRCCOPY);
			BitBlt(hdc, x + 7, y, 7, 12, isRed ? BackBuf.FontRedDC : BackBuf.FontDC, 7 * (val & 15), 0, SRCCOPY);

			// Append to ascii
			if (showAscii)
			{
				auto ch = val >= 32 && val < 127 ? val : '.';
				x = rgt - column2Width + n * 7 + 1;
				y = ltop + 2;
				BitBlt(hdc, x, y, 7, 12, BackBuf.FontAsciiDC, 7 * (ch - 34), 0, SRCCOPY);
			}
		}

		if (!showAscii)
		{
			// reset address start to zero
			memGpu.TagY = 0;
			memGpu.Start = 0;
			memGpu.StartofVidram = 0;
			memGpu.NewStartofVidram = 0;

			// bytes to copy
			memGpu.BytesperRow = dataWidth;

			// render 12 lines
			for (int j = 0; j < pixelHeight; ++j)
				memGpu.UpdateScreen32To(ramCache+lnum*dataWidth, (unsigned int*)BackBuf.Data, j, BackBuf.DataWidth, false);

			// blit to back buffer
			HBITMAP bm = CreateBitmap(BackBuf.DataWidth, BackBuf.DataHeight, 1, 32, BackBuf.Data);
			HDC src = CreateCompatibleDC(hdc);
			auto obj = SelectObject(src, bm);
			int column = (column2Width - 2) & (~0xF);
			StretchBlt(hdc, rgt - column2Width, ltop, column, lineHeight, src, 0, 0, dataWidth * 16, pixelHeight*2, SRCCOPY);
			SelectObject(src, obj);
			DeleteObject(bm);
			DeleteDC(src);
		}
	}

	// Not editmode if no cell highlighted.
	if (Editing && !hlfound) {
		SetEditing(false);
	}

	return dirty;
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

		BackBuf.Init();
		SetBackBuffer(Rect);
		CreateScrollBar(Rect);

		hStatic = GetDlgItem(hDlg,-1);

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

		SetTimer(hDlg, IDT_MEM_TIMER, 1000/60, nullptr);

		// Dropdown to select memory type displayed
		HWND hCtl = GetDlgItem(hDlg, IDC_MEM_TYPE);
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "CPU");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "REAL");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "ROM");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "PAK");
		SendMessage(hCtl,CB_SETCURSEL,(WPARAM) 0, (LPARAM) 0);
		SetMemType();

		hCtl = GetDlgItem(hDlg, IDC_VIEW_TYPE);
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"ASCII");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"SG4");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE4 NTSC");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE4 RGB");
		SendMessage(hCtl, CB_SETCURSEL, (WPARAM)VM_SG4, (LPARAM)0);
		SetViewType();

		SetBackBuffer(Rect);

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
	memoryOffset = roundDn(si.nPos,16);

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

