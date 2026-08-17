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
#include "config.h"
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
void DoHorzScroll(WPARAM);
int CStrToHex(const char *);
void ResetMemoryCache();
void ResizeWindow(int width, int height);

LRESULT CALLBACK subEditValProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK subEditAdrBegProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK subEditAdrEndProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK MemoryMapDlgProc(HWND,UINT,WPARAM,LPARAM);

const std::string cMemoryWindow("MemoryWindow");
const std::string cWindowSizeX("WindowSizeX");
const std::string cWindowSizeY("WindowSizeY");
const std::string cWindowPosX("WindowPosX");
const std::string cWindowPosY("WindowPosY");
const std::string cAddress("Address");
const std::string cAddressMode("AddressMode");
const std::string cViewMode("ViewMode");
const std::string cHex("Hex");
const std::string cWide("Wide");
const std::string cDataWidth("DataWidth");
const std::string cDataPosX("DataPosX");

// Global handles
HWND hDlgMem = nullptr;
HWND hScrollBar = nullptr;
HWND hHorzScrollBar = nullptr;
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
	VM_PMODE0_S0,
	VM_PMODE0_S1,
	VM_PMODE1_S0,
	VM_PMODE1_S1,
	VM_PMODE2_S0,
	VM_PMODE2_S1,
	VM_PMODE3_S0,
	VM_PMODE3_S1,
	VM_PMODE4_RGB_S0,
	VM_PMODE4_RGB_S1,
	VM_PMODE4_NTSC,
	VM_TEXT40,
	VM_TEXT80,
	VM_HSCREEN1,
	VM_HSCREEN2,
	VM_HSCREEN3,
	VM_HSCREEN4,
	VM_MAX
};
ViewMode viewMode = VM_SG4;

const int cTopBarStaticWidth = 510;		// the tool bar
const int cTopBarHeight = 34;
const int cHeaderHeight = 20;			// header with labels
const int cVertScrollBarWidth = 20;
const int cHorzScrollBarHeight = 20;	// bottom scroll bar
const int cAddressWidth = 60;			// address column width
const int cAsciiCharWidth = 7;

// windows will allow the track bar to be positioned
// pixel by pixel, but it will snap to the
// nearest whole value. as the horizontal scrolling 
// will use small numbers, this causes a visible jumping
// of the track bar which is distracting, so instead
// allow higher resolution for the track bar and scale
// the result.
const int cHorzScrollFactor = 10;

int MemSize = 0;
int memoryOffset = 0;
int selectionRangeBeg = -1;
int selectionRangeEnd = -1;
unsigned char *Rom = nullptr;
bool Editing = false;
bool Hex = true;
bool WideView = false;
int editAddress = 0;
int dataWidth = 16;
int dataPosX = 0;

struct Measurements
{
	bool showHex;
	bool showAscii;
	int top;
	int bottom;
	int left;
	int right;
	int height;
	int width;
	int hexDigitsWidth;		// width of hex digits
	int hexMinColums;		// hex shown columns
	int hexColumns;			// hex columns visible
	int hexWidth;			// hex window width (pixels)
	int viewWidth;			// view window width (pixels)
	int viewOffset;			// view window buffer offset
	int viewColumns;		// view window columns (bytes)
	int viewMaxColumns;		// maximum number of view columns (gime)
	int viewMaxWidth;		// view window maximum width (pixels)
	int viewWinWidth;		// view window minimum width (pixels)
	float viewCellWidth;
	int lineHeight;
	int currLineTop;
	int viewPosX;			// view window horizontal scroll
	int viewStep;			// scrolling increment

	const int fontHeight = 12;

	const int maxDataWidth[VM_MAX] =
	{
		256,//VM_ASCII,
		32,//VM_SG4,
		16,//VM_PMODE0_S0,
		16,//VM_PMODE0_S1,
		32,//VM_PMODE1_S0,
		32,//VM_PMODE1_S1,
		16,//VM_PMODE2_S0,
		16,//VM_PMODE2_S1,
		32,//VM_PMODE3_S0,
		32,//VM_PMODE3_S1,
		32,//VM_PMODE4_RGB_S0,
		32,//VM_PMODE4_RGB_S1,
		32,//VM_PMODE4_NTSC,
		40,//VM_TEXT40,
		80,//VM_TEXT80,
		80,//VM_HSCREEN1,
		160,//VM_HSCREEN2,
		80,//VM_HSCREEN3,
		80,//VM_HSCREEN4
	};

	const int pixelViewWidth[VM_MAX] =
	{
		8,//VM_ASCII,
		8,//VM_SG4,
		16,//VM_PMODE0_S0,
		16,//VM_PMODE0_S1,
		8,//VM_PMODE1_S0,
		8,//VM_PMODE1_S1,
		16,//VM_PMODE2_S0,
		16,//VM_PMODE2_S1,
		8,//VM_PMODE3_S0,
		8,//VM_PMODE3_S1,
		8,//VM_PMODE4_RGB_S0,
		8,//VM_PMODE4_RGB_S1,
		8,//VM_PMODE4_NTSC,
		4,//VM_TEXT40,
		2,//VM_TEXT80,
		4,//VM_HSCREEN1,
		2,//VM_HSCREEN2,
		4,//VM_HSCREEN3,
		2,//VM_HSCREEN4
	};

	//
	// construct display measurements from back buffer client rectangle
	//
	Measurements(LPCRECT clientRect)
	{
		showHex = Hex;
		showAscii = viewMode == VM_ASCII;

		// rect
		top = clientRect->top;
		bottom = clientRect->bottom - cHeaderHeight;
		left = clientRect->left;
		right = clientRect->right;
		height = bottom - top;
		width = right - left;
		
		// hex column info
		hexDigitsWidth = 18;
		hexColumns = dataWidth > 32 ? 32 : dataWidth;
		hexMinColums = hexColumns < 16 ? 16 : hexColumns;
		hexWidth = hexColumnPos(hexMinColums);

		if (!showHex)
		{
			hexWidth = hexColumnPos(0) + 10;
			hexColumns = 0;
		}

		// include a gutter after width of 16
		int gutter = (hexColumns > 16 && hexColumns & 7) ? 10 : 0;

		// view column info
		viewMaxColumns = (dataWidth > maxDataWidth[viewMode] ? maxDataWidth[viewMode] : dataWidth);
		viewColumns = WideView ? dataWidth : viewMaxColumns;
		viewWidth = width - hexWidth - gutter;				// view width is remaining width
		viewWinWidth = viewColumns * 2 * pixelViewWidth[viewMode];
		viewMaxWidth = (viewWidth - 2) & (~0xF);			// round view width
		viewOffset = 0;

		int charWidth = 16;
		viewStep = 1;
		if (viewMode == VM_TEXT40 || viewMode == VM_TEXT80)
		{
			dataWidth = roundDn(dataWidth, 2);
			charWidth = 4;
			viewStep = 2;
		}

		if (viewMaxWidth > viewColumns * charWidth * pixelViewWidth[viewMode])
			viewMaxWidth = viewColumns * charWidth * pixelViewWidth[viewMode];

		// line rows
		lineHeight = (height / 32) - 1;
		currLineTop = top + cHeaderHeight;

		if (viewMode == VM_SG4)
		{
			if (viewMaxWidth > viewColumns * 16)
				viewMaxWidth = viewColumns * 16;
		}

		viewCellWidth = ((float)viewMaxWidth / viewWinWidth) * pixelViewWidth[viewMode] * 2;

		if (viewMode == VM_ASCII)
		{
			viewMaxWidth = (viewWidth - 2) & (~0xF);
			viewColumns = std::min(viewMaxWidth / cAsciiCharWidth, dataWidth);
			viewCellWidth = 7;
		}

		if (viewMode == VM_PMODE4_NTSC)
			viewOffset = 4;

		// reposition view so hex is always visible
		const float rounding = 0.000001f; // adjust for rounding
		float pos = dataWidth <= hexColumns ? 0 : (float)dataPosX / (dataWidth - hexColumns);
		viewPosX = roundDn(std::max(0, (int)((dataWidth - viewColumns) * (pos + rounding))), viewStep);

	}

	//
	// left edge of hex column
	//
	int hexLeft() const { return left + cAddressWidth + 5; }

	//
	// left edge of view 
	//
	int viewLeft() const { return right - viewWidth; }

	//
	// left/right pos of hex window in view
	//
	int viewHexLeft() const { return viewLeft() + (int)(viewCellWidth * (dataPosX - viewPosX)); }
	int viewHexRight() const { return viewHexLeft() + (int)(viewCellWidth * hexColumns); }

	//
	// return pos (pixel offset) of hex value column n
	//
	int hexColumnPos(int n) const
	{
		// hex cell spacing
		const int minorSpacing = hexDigitsWidth;

		// gutter spacing after each 8 values
		const int majorSpacing = 10;

		// calculate minor & major column position
		int minorColumn = n & 7;
		int majorColumn = n / 8;
		return hexLeft() + minorColumn * minorSpacing + majorColumn * (minorSpacing * 8 + majorSpacing);
	}

	//
	// reverse of hexColumnPos(x) given relative pixel offset
	//
	int hexPosColumn(int x) const
	{
		// move origin
		x -= hexLeft();
		const int minorSpacing = hexDigitsWidth;
		const int majorSpacing = 10;
		int minorColumnWidth = minorSpacing;
		int majorColumnWidth = minorSpacing * 8 + majorSpacing;
		// calculate major column
		int majorColumn = x / majorColumnWidth;
		// get remainder
		x -= majorColumn * majorColumnWidth;
		// if inside gutter return invalid
		if (x > minorSpacing * 8) return -1;
		// calculate minor column
		int minorColumn = x / minorColumnWidth;
		// return column index
		return majorColumn * 8 + minorColumn;
	}

	//
	// left edge of address column
	//
	int rowAddressPos() const
	{
		return left + 10;
	}

	//
	// return current row, centered for text 
	//
	int rowTextY() const
	{
		return currLineTop + (lineHeight - fontHeight) / 2;
	}
};

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


// Help text
char DbgHelp[] =
	"The default memory type displayed is 'CPU'.\n"
	"Dropdown will select 'REAL', 'ROM', or 'PAK'.\n\n"
	"In addition to the scroll bar the mouse wheel,\n"
	"Home, End, PgUp, PgDn, Up, and Down keys\n"
	"will scroll the display.\n\n"
	"Select hex to be edited by clicking on a\n"
	"cell. The cell will turn red and it's address\n"
	"will be displayed next to the box. Enter byte\n"
	"values in hexadecimal. ESC to end edit.\n\n"
	"Select display mode: Ascii, SG4, PMODE, HSCREEN.\n\n"
	"Click hex title to collapse/expand hex.\n\n"
	"Other Keys:\n"
	" [  ]  Dec/inc data row width by 1.\n"
	" {  }  Dec/inc data row width by 8.\n"
	"";


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

	HDC hdc = GetDC(hDlgMem);

	RECT frc;
	SetRect(&frc, 0, 0, 7 * 16, 14);

	// Set display Font
	Font = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE,
		FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
		TEXT("Consolas"));

	// Set display pen color
	Pen = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));

	UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	// cache main hex font (black)
	BackBuf.FontDC = CreateCompatibleDC(hdc);
	BackBuf.FontBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(BackBuf.FontDC, BackBuf.FontBitmap);
	SelectObject(BackBuf.FontDC, BackBuf.Font);
	DrawText(BackBuf.FontDC, "0123456789ABCDEF", 32, &frc, fmt);

	// cache secondary hex font (red)
	BackBuf.FontRedDC = CreateCompatibleDC(hdc);
	BackBuf.FontRedBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(BackBuf.FontRedDC, BackBuf.FontRedBitmap);
	SelectObject(BackBuf.FontRedDC, BackBuf.Font);
	SetTextColor(BackBuf.FontRedDC, RGB(255, 0, 0));  // Red
	DrawText(BackBuf.FontRedDC, "0123456789ABCDEF", 32, &frc, fmt);

	// cache ascii font
	frc.right = 7 * 96;
	BackBuf.FontAsciiDC = CreateCompatibleDC(hdc);
	BackBuf.FontAsciiBitmap = CreateCompatibleBitmap(hdc, 7 * 96, 14);
	SelectObject(BackBuf.FontAsciiDC, BackBuf.FontAsciiBitmap);
	SelectObject(BackBuf.FontAsciiDC, BackBuf.Font);
	DrawText(BackBuf.FontAsciiDC, "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[.]^_`abcdefghijklmnopqrstuvwxyz{|}~.", 96, &frc, fmt);

	ReleaseDC(hDlgMem, hdc);
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
	Data = nullptr;
	CleanupFonts(hWnd);
	CleanupFont();
	CleanupPen();
	CleanupBitmap();
	CleanupDC(hWnd);
}

//
// refresh the whole window
//
void RepaintAll()
{
	ResetMemoryCache();
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);

	// clear backbuffer
	HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(BackBuf.DeviceContext, &BackBuf.Rect, brush);

	// redraw headers
	DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);
}

//
// adjust vertical scroll bar based on data width
//
void UpdateVertScrollBar()
{
	SCROLLINFO si = { 0 };
	int visible = 32 * dataWidth;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin = 0;
	si.nPage = visible;
	si.nMax = (MemSize > visible) ? MemSize - 1 : 0;
	si.nPos = memoryOffset;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
}


//
// adjust horizontal scroll bar based on data width
//
void UpdateHorzScrollBar()
{
	Measurements m(&BackBuf.Rect);
	int visible = (cHorzScrollFactor * (Hex ? m.hexColumns : m.viewColumns)) / m.viewStep;
	int total = (cHorzScrollFactor * dataWidth) / m.viewStep;
	int pos = (cHorzScrollFactor * dataPosX) / m.viewStep;

	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin = 0;
	si.nPage = visible;
	si.nMax = total > visible ? total - 1 : 0;
	si.nPos = pos;
	SetScrollInfo(hHorzScrollBar, SB_CTL, &si, TRUE);
}

//
// show current width
//
void UpdateWidthDisplay()
{
	char info[64];
	sprintf(info, "Width\n%d", dataWidth);
	SetDlgItemText(hDlgMem, IDC_ADRTXT, info);
}

//
// setup new type of view
//
void SetViewType()
{
	HWND hCtl = GetDlgItem(hDlgMem, IDC_VIEW_TYPE);
	ViewMode mode = (ViewMode)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
	if (viewMode == mode) return;
	viewMode = mode;
	UpdateHorzScrollBar();
	RepaintAll();
	UpdateWidthDisplay();
}

//
// save memory window state
//
void SaveSettings()
{
	RECT CurWindow;
	::GetWindowRect(hDlgMem, &CurWindow);
	RECT CurScreen;
	::GetClientRect(hDlgMem, &CurScreen);
	int clientWidth = (int)CurScreen.right;
	int clientHeight = (int)CurScreen.bottom;

	//if (!IsMaximized(DFState) && !DFState->FullScreen && !DFState->Exiting)
	{
		VCC::Rect rect;
		// remember positioning:
		rect.x = CurWindow.left;
		rect.y = CurWindow.top;

		// remember size:
		rect.w = clientWidth; // Used for saving new window size to the ini file.
		rect.h = clientHeight - cHeaderHeight;

		auto& s = Setting();
		s.write(cMemoryWindow, cWindowSizeX, rect.w);
		s.write(cMemoryWindow, cWindowSizeY, rect.h);
		s.write(cMemoryWindow, cWindowPosX, rect.x);
		s.write(cMemoryWindow, cWindowPosY, rect.y);

		// other state
		s.write(cMemoryWindow, cAddress, memoryOffset);
		s.write(cMemoryWindow, cAddressMode, (int)AddrMode_);
		s.write(cMemoryWindow, cViewMode, (int)viewMode);
		s.write(cMemoryWindow, cHex, Hex);
		s.write(cMemoryWindow, cWide, WideView);
		s.write(cMemoryWindow, cDataWidth, dataWidth);
		s.write(cMemoryWindow, cDataPosX, dataPosX);
	}
}

//
// load memory window state
//
void LoadSettings()
{
	auto& s = Setting();
	s.Read(cMemoryWindow, cAddress, memoryOffset);
	s.Read(cMemoryWindow, cAddressMode, (int&)AddrMode_);
	s.Read(cMemoryWindow, cViewMode, (int&)viewMode);
	s.Read(cMemoryWindow, cHex, (int&)Hex);
	s.Read(cMemoryWindow, cWide, (int&)WideView);
	s.Read(cMemoryWindow, cDataWidth, dataWidth);
	s.Read(cMemoryWindow, cDataPosX, dataPosX);

	if (GetAsyncKeyState(VK_SHIFT))
	{
		dataPosX = 0;
		dataWidth = 16;
		WideView = false;
		Hex = true;
		viewMode = VM_SG4;
		AddrMode_ = Cpu;
	}
}

//
// setup for new data width
//
void SetupDataWidth(int delta)
{
	Measurements m(&BackBuf.Rect);

	if (delta < 0)
	{
		if (GetAsyncKeyState(VK_SHIFT))
		{
			dataWidth = (dataWidth & 7) ? dataWidth & (~7) : dataWidth - 8;
			if (dataWidth < m.viewStep) dataWidth = m.viewStep;
		}
		else
			dataWidth = dataWidth > m.viewStep ? dataWidth - m.viewStep : dataWidth;
	}
	else if (delta > 0)
	{
		if (GetAsyncKeyState(VK_SHIFT))
		{
			dataWidth = (dataWidth & 7) ? (dataWidth + 8) & (~7) : dataWidth + 8;
			if (dataWidth > 256) dataWidth = 256;
		}
		else
			dataWidth = dataWidth < (257 - m.viewStep) ? dataWidth + m.viewStep : dataWidth;
	}

	if (dataPosX + m.hexColumns > dataWidth)
		dataPosX = std::max(0, dataWidth - m.hexColumns);

	UpdateWidthDisplay();

	ResetMemoryCache();
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);

	// clear backbuffer
	HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(BackBuf.DeviceContext, &BackBuf.Rect, brush);

	// adjust vertical scrolling size
	UpdateVertScrollBar();
	UpdateHorzScrollBar();

	// redraw headers
	DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);
}


//
// shutdown the memory window
//
void Shutdown()
{
	if (hDlgMem)
	{
		SaveSettings();
		KillTimer(hDlgMem, IDT_MEM_TIMER);
		BackBuf.Cleanup(hDlgMem);
		DestroyWindow(hDlgMem);
		hDlgMem = nullptr;
		ResetMemoryCache();
	}
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

	case WM_HSCROLL:
		DoHorzScroll(wParam);
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

	case WM_CLOSE:
	case WM_DESTROY:
		Shutdown();
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
				Shutdown();
				break;
		}
		break;
	}
	return FALSE;
}

//
// resize the window
//
void ResizeWindow(int width, int height)
{
	RECT Rect;
	GetClientRect(hDlgMem, &Rect);

	// recreate the back buffer
	ResetMemoryCache();
	SetBackBuffer(Rect);
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
	DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);

	// reposition scroll bar
	MoveWindow(hScrollBar, Rect.right - cVertScrollBarWidth, cTopBarHeight, cVertScrollBarWidth, Rect.bottom - cTopBarHeight, TRUE);
	MoveWindow(hHorzScrollBar, Rect.left, Rect.bottom - cHorzScrollBarHeight, Rect.right - cVertScrollBarWidth, cHorzScrollBarHeight, TRUE);
	MoveWindow(hStatic, Rect.left, 0, Rect.right, cTopBarHeight, TRUE);
}


//------------------------------------------------------------------
//  Subclassed Edit Value Proc
//------------------------------------------------------------------
LRESULT CALLBACK subEditValProc(
		HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CHAR:
	case WM_KEYUP:
	{
		switch (wParam)
		{
			case '[':
			case ']':
			case '{':
			case '}':
			case VK_OEM_4:
			case VK_OEM_6:
				return 0;
		}
	}
	break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			CommitValue();
			return 0;
		case VK_TAB:
		case VK_ESCAPE:
			SetEditing(false);
			SetFocus(hEditAdrBeg);
			ResetMemoryCache();
			InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
			return 0;
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			FlashDialogWindow();
			return 0;
		case VK_OEM_4:
		case VK_OEM_6:
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
	case WM_KEYUP:
	case WM_CHAR:
	{
		switch (wParam)
		{
			case '[':
			case ']':
			case '{':
			case '}':
			case VK_OEM_4:
			case VK_OEM_6:
				return 0;
		}
	}
	break;

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
		case VK_OEM_4:
			SetupDataWidth(-1);
			return 0;
		case VK_OEM_6:
			SetupDataWidth(1);
			return 0;

		}
		break;
	}
	return CallWindowProc(EditValProc, wnd, msg, wParam, lParam);
}

LRESULT CALLBACK subEditAdrEndProc(
	HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
	case WM_CHAR:
	case WM_KEYUP:
	{
		switch (wParam)
		{
			case '[':
			case ']':
			case VK_OEM_4:
			case VK_OEM_6:
				return 0;
		}
	}
	break;

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
		case VK_OEM_4:
			SetupDataWidth(-1);
			return 0;
		case VK_OEM_6:
			SetupDataWidth(1);
			return 0;
		}
		break;
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
	RECT topBar = {0};
	topBar.left = rc.left + cTopBarStaticWidth;
	topBar.right = rc.right;
	topBar.top = rc.top;
	topBar.bottom = cTopBarHeight;

	// Adjust backing buffer location on client
	BackBuf.Rect.left   = rc.left;
	BackBuf.Rect.right  = rc.right  - cVertScrollBarWidth;
	BackBuf.Rect.top    = rc.top    + cTopBarHeight;
	BackBuf.Rect.bottom = rc.bottom + cTopBarHeight - cHorzScrollBarHeight;

	BackBuf.Width  = BackBuf.Rect.right  - BackBuf.Rect.left;
	BackBuf.Height = BackBuf.Rect.bottom - BackBuf.Rect.top;

	HDC hdc = GetDC(hDlgMem);

	BackBuf.CleanupBitmap();
	BackBuf.CleanupDC(hDlgMem);

	BackBuf.DeviceContext = CreateCompatibleDC(hdc);
	BackBuf.Bitmap = CreateCompatibleBitmap(hdc, BackBuf.Width, BackBuf.Height);
	
	HBITMAP old = (HBITMAP) SelectObject(BackBuf.DeviceContext, BackBuf.Bitmap);
	DeleteObject(old);
	ReleaseDC(hDlgMem, hdc);

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
			Rect.right - cVertScrollBarWidth,   //top x
			cTopBarHeight,      //top y
			cVertScrollBarWidth,     //width
			Rect.bottom - cTopBarHeight,  //height
			hDlgMem,
			(HMENU)IDC_MEM_VSCROLLBAR,
			(HINSTANCE)GetWindowLong(hDlgMem, GWL_HINSTANCE),
			nullptr);

		if (!hScrollBar) {
			MessageBox(nullptr, "Vertical Scroll Bar Failed.", "Error",
				MB_OK | MB_ICONERROR);
		}

		hHorzScrollBar = CreateWindowEx(
			0,
			"SCROLLBAR",
			nullptr,
			WS_VISIBLE | WS_CHILD | SBS_HORZ,
			Rect.left,   //top x
			Rect.bottom - cHorzScrollBarHeight,      //top y
			Rect.right - cVertScrollBarWidth,     //width
			cHorzScrollBarHeight,  //height
			hDlgMem,
			(HMENU)IDC_MEM_HSCROLLBAR,
			(HINSTANCE)GetWindowLong(hDlgMem, GWL_HINSTANCE),
			nullptr);

		if (!hHorzScrollBar) {
			MessageBox(nullptr, "Horizontal Scroll Bar Failed.", "Error",
				MB_OK | MB_ICONERROR);
		}
}

//------------------------------------------------------------------
// Draw display form with header and vert guide lines
//------------------------------------------------------------------
void DrawForm(HDC hdc,LPCRECT clientRect)
{
	RECT rc;
	Measurements m(clientRect);

	// Clear background.
	HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hdc, clientRect, brush);

	// Format for text
	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	// Draw separator lines for border, address, and ascii
	MoveToEx(hdc, m.left, m.top + cHeaderHeight, nullptr); LineTo(hdc, m.right, m.top + cHeaderHeight);
	MoveToEx(hdc, m.left + cAddressWidth, m.top, nullptr); LineTo(hdc, m.left + cAddressWidth, m.bottom - 1);
	MoveToEx(hdc, m.right - m.viewWidth - 2, m.top, nullptr); LineTo(hdc, m.right - m.viewWidth - 2, m.bottom);

	// Horizontal separators every four rows
	for (int lnum = 0; lnum <= 32; lnum += 4)
	{
		MoveToEx(hdc, m.left, m.currLineTop, nullptr); LineTo(hdc, m.right, m.currLineTop);
		m.currLineTop += m.lineHeight * 4;
	}

	// Draw header
	SetTextColor(hdc, RGB(138, 27, 255));
	SetRect(&rc, m.left, m.top, m.left + cAddressWidth, m.top + cHeaderHeight);
	DrawText(hdc, "Address", 7, &rc, fmt);
	if (!m.showHex)
	{
		rc.left = m.hexColumnPos(0) + 10;
		DrawText(hdc, ">", 1, &rc, fmt);
	}
	for (int n = 0; n < m.hexColumns; n++)
	{
		SetRect(&rc, m.hexColumnPos(n), m.top, m.hexColumnPos(n) + m.hexDigitsWidth - 4, m.top + cHeaderHeight);
		const std::string s(ToHexString(dataPosX + n, 2, false));
		DrawText(hdc, s.c_str(), 2, &rc, fmt);
	}
	SetRect(&rc, m.right - m.viewWidth - 2, m.top, m.right - 5, m.top + cHeaderHeight);
	const char* viewModes[] = 
	{
		"ASCII", 
		"Semi Graphics 4", 
		"PMODE 0 SCREEN 0 (2 colors)",
		"PMODE 0 SCREEN 1 (2 colors)",
		"PMODE 1 SCREEN 0 (4 colors)",
		"PMODE 1 SCREEN 1 (4 colors)",
		"PMODE 2 SCREEN 0 (2 colors)",
		"PMODE 2 SCREEN 1 (2 colors)",
		"PMODE 3 SCREEN 0 (4 colors)",
		"PMODE 3 SCREEN 1 (4 colors)",
		"PMODE 4 SCREEN 0 RGB (2 colors)", 
		"PMODE 4 SCREEN 1 RGB (2 colors)",
		"PMODE 4 NTSC (artifact colors)",
		"TEXT 40",
		"TEXT 80",
		"HSCREEN 1 (4 Colors)",
		"HSCREEN 2 (16 Colors)",
		"HSCREEN 3 (2 Colors)", 
		"HSCREEN 4 (4 Colors)" 
	};
	char temp[80];
	strcpy(temp, viewModes[viewMode]);
	if (WideView) strcat(temp, " - Wide");
	DrawText(hdc, temp, strlen(temp), &rc, fmt);

	// draw hex region
	int left = std::max(m.viewLeft(), m.viewHexLeft());
	int right = m.viewHexRight();
	SetRect(&rc, left, m.top + cHeaderHeight - 4, right, m.top + cHeaderHeight - 2);

	// draw horz bar
	HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	FillRect(BackBuf.DeviceContext, &rc, blackBrush);

	auto drawBarEnd = [blackBrush](RECT rc, int x)
	{
		rc.top -= 2;
		rc.bottom += 2;
		rc.left = x;
		rc.right = x + 1;
		FillRect(BackBuf.DeviceContext, &rc, blackBrush);
	};

	drawBarEnd(rc,left);
	drawBarEnd(rc,right - 1);
}


//------------------------------------------------------------------
// Fill memory data on form
//------------------------------------------------------------------
bool DrawMemory(HDC hdc, LPCRECT clientRect)
{
	bool dirty = false;
	bool forcedUpdate = false;

	memGpu.GimeReset();
	memGpu.SetCompatMode(1);
	memGpu.SetMonitorType(1);

	// if palette changes pixels will need rewriting
	if (memGpu.CopyPalette(gGimeGpu))
		ResetMemoryCache();

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
	else if (viewMode == VM_PMODE0_S0)
	{
		memGpu.SetGimeVdgMode(3);
		memGpu.SetGimeVdgMode2(22);
	}
	else if (viewMode == VM_PMODE0_S1)
	{
		memGpu.SetGimeVdgMode(3);
		memGpu.SetGimeVdgMode2(23);
	}
	else if (viewMode == VM_PMODE1_S0)
	{
		memGpu.SetGimeVdgMode(4);
		memGpu.SetGimeVdgMode2(24);
	}
	else if (viewMode == VM_PMODE1_S1)
	{
		memGpu.SetGimeVdgMode(4);
		memGpu.SetGimeVdgMode2(25);
	}
	else if (viewMode == VM_PMODE2_S0)
	{
		memGpu.SetGimeVdgMode(5);
		memGpu.SetGimeVdgMode2(26);
	}
	else if (viewMode == VM_PMODE2_S1)
	{
		memGpu.SetGimeVdgMode(5);
		memGpu.SetGimeVdgMode2(27);
	}
	else if (viewMode == VM_PMODE3_S0)
	{
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(28);
	}
	else if (viewMode == VM_PMODE3_S1)
	{
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(29);
	}
	else if (viewMode == VM_PMODE4_RGB_S0)
	{
		memGpu.SetMonitorType(1);
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(30);
	}
	else if (viewMode == VM_PMODE4_RGB_S1)
	{
		memGpu.SetGimeVdgMode(6);
		memGpu.SetGimeVdgMode2(31);
	}
	else if (viewMode == VM_TEXT40)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(3);
		memGpu.SetGimeVres(5);
	}
	else if (viewMode == VM_TEXT80)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(3);
		memGpu.SetGimeVres(21);
	}
	else if (viewMode == VM_HSCREEN1)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(128);
		memGpu.SetGimeVres(21);
	}
	else if (viewMode == VM_HSCREEN2)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(128);
		memGpu.SetGimeVres(122);
	}
	else if (viewMode == VM_HSCREEN3)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(128);
		memGpu.SetGimeVres(20);
	}
	else if (viewMode == VM_HSCREEN4)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(128);
		memGpu.SetGimeVres(29);
	}

	memGpu.SetupDisplay();
	memGpu.VertCenter = 0;
	memGpu.HorzCenter = 0;

	Measurements m(clientRect);

	int pixelHeight = memGpu.LinesperRow;
	bool hlfound = false;
	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	if (!ramCache)
	{
		forcedUpdate = true;
		ramCache = new unsigned char[dataWidth * 32];
	}

	// if not the same address then it will need repainting
	if (memoryOffset != ramPos)
	{
		dirty = true;
		ramPos = memoryOffset;
	}

	for (int lnum = 0; lnum < 32; lnum++, m.currLineTop += m.lineHeight)
	{
		unsigned int offset = lnum * dataWidth;
		unsigned int address = memoryOffset + offset;

		int x = m.rowAddressPos();
		int y = m.rowTextY()+1;

		// Draw address of start of line
		BitBlt(hdc, x, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 20) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 7, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 16) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 14, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 12) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 21, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 8) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 28, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 4) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 35, y, 7, 12, BackBuf.FontDC, 7 * ((address >> 0) & 15), 1, SRCCOPY);

		// update ram cache
		bool update = Editing || forcedUpdate;
		for (int a = 0; a < dataWidth; ++a)
		{
			auto b = ReadMemory(address + a);
			if (b != ramCache[offset + a])
				ramCache[offset + a] = b, update = true;
		}

		// skip nothing to update
		if (!update)
		{
			continue;
		}
		dirty = true;

		// draw hex columns & view
		for (int n = 0; n < m.hexColumns; n++)
		{
			// Get data
			unsigned char val = ramCache[dataPosX + offset + n];

			// Highlight data if cell is being edited
			bool isRed = Editing && editAddress == address + dataPosX + n;
			if (isRed) hlfound = true;

			x = m.hexColumnPos(n);
			BitBlt(hdc, x, y, 7, 11, isRed ? BackBuf.FontRedDC : BackBuf.FontDC, 7 * (val >> 4), 1, SRCCOPY);
			BitBlt(hdc, x + 7, y, 7, 11, isRed ? BackBuf.FontRedDC : BackBuf.FontDC, 7 * (val & 15), 1, SRCCOPY);
		}

		// render ascii
		if (m.showAscii)
		{
			for (int n = 0; n < m.viewColumns; n++)
			{
				unsigned char val = ramCache[offset + n + m.viewPosX];
				auto ch = val >= 32 && val < 127 ? val : '.';
				x = m.right - m.viewWidth + n * 7 + 1;
				BitBlt(hdc, x, y, 7, 11, BackBuf.FontAsciiDC, cAsciiCharWidth * (ch - 34), 1, SRCCOPY);
			}
		}
		else
		{
			float columns = (float)m.viewMaxWidth / m.viewColumns;
			int columnCount = (m.viewColumns + m.viewMaxColumns - 1) / m.viewMaxColumns;
			int left = m.right - m.viewWidth;
			for (int s = 0; s < columnCount; ++s)
			{
				// reset address start to zero
				memGpu.TagY = 0;
				memGpu.Start = 0;
				memGpu.StartofVidram = 0;
				memGpu.NewStartofVidram = 0;

				int columnPos = s * m.viewMaxColumns;
				int columnRem = std::min(m.viewMaxColumns, m.viewColumns - columnPos);

				// bytes to copy
				memGpu.BytesperRow = columnRem;

				// render 12 lines
				for (int j = 0; j < pixelHeight; ++j)
					memGpu.UpdateScreen32To(ramCache + lnum * dataWidth + m.viewPosX + columnPos, (unsigned int*)BackBuf.Data, j, BackBuf.DataWidth, false);

				// blit to back buffer
				HBITMAP bm = CreateBitmap(BackBuf.DataWidth, BackBuf.DataHeight, 1, 32, BackBuf.Data);
				HDC src = CreateCompatibleDC(hdc);
				auto obj = SelectObject(src, bm);
				int width = (int)(columnRem * columns);
				int renderedWidth = std::min(columnRem * 2 * m.pixelViewWidth[viewMode], m.viewWinWidth);
				StretchBlt(hdc, left, m.currLineTop, width, m.lineHeight, src, m.viewOffset, 0, renderedWidth, pixelHeight * 2, SRCCOPY);
				left += width;
				SelectObject(src, obj);
				DeleteObject(bm);
				DeleteDC(src);
			}
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
	Measurements m(&BackBuf.Rect);

	auto edit = [](bool b)
	{
		SetEditing(b);
		InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
	};

	// ignore tool bar area
	if (yPos < cHeaderHeight)
		return edit(false);

	// check title area
	if (yPos < m.currLineTop)
	{
		// in title then collapse hex
		if (xPos > m.hexColumnPos(0) - 10 && xPos < m.hexColumnPos(m.showHex ? m.hexColumns : 1))
		{
			Hex = !Hex;
			UpdateHorzScrollBar();
			RepaintAll();
		}
		else if (xPos > m.hexColumnPos(m.showHex ? m.hexColumns : 1))
		{
			WideView = !WideView;
			UpdateHorzScrollBar();
			RepaintAll();
		}
		return edit(false);
	}

	// work out which row
	int row = (yPos - m.currLineTop) / m.lineHeight;

	// if out of bounds abort
	if (row < 0 || row >= 32) return edit(false);

	// work out which column
	int col = m.hexPosColumn(xPos);

	// if out of bounds abort
	if (col < 0 || col >= m.hexColumns) return edit(false);

	// hit
	editAddress = memoryOffset + col + row * dataWidth + dataPosX;
	return edit(true);
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

	SCROLLINFO si = {0};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hScrollBar, SB_CTL, &si);

	si.nPos = roundDn(selectionRangeBeg, dataWidth);
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
	int PhySiz[4] = { 0x20000,0x80000,0x200000,0x800000 };

	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_TYPE);
	AddrMode mode = (AddrMode)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
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

	UpdateVertScrollBar();
	UpdateHorzScrollBar();
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
		SendMessage(hCtl,CB_SETCURSEL,(WPARAM)AddrMode_, (LPARAM) 0);
		SetMemType();

		hCtl = GetDlgItem(hDlg, IDC_VIEW_TYPE);
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"ASCII");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"SG4");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE0 S0");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE0 S1");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE1 S0");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE1 S1");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE2 S0");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE2 S1");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE3 S0");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE3 S1");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE4 S0 RGB");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE4 S1 RGB");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"PMODE4 NTSC");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"TEXT40");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"TEXT80");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"HSCREEN1");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"HSCREEN2");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"HSCREEN3");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"HSCREEN4");
		SendMessage(hCtl, CB_SETCURSEL, (WPARAM)viewMode, (LPARAM)0);
		SetViewType();

		SetBackBuffer(Rect);

		// Draw the form for memory data
		DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);

		// Not edit mode
		SetEditing(false);
}


void DoHorzScroll(WPARAM wParam)
{
	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hHorzScrollBar, SB_CTL, &si);
	int pos = si.nPos;
	switch ((int)LOWORD(wParam)) {
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
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
			si.nPos -= cHorzScrollFactor;
			break;
		case SB_LINEDOWN:
			si.nPos += cHorzScrollFactor;
			break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hHorzScrollBar, SB_CTL, &si, TRUE);
	GetScrollInfo(hHorzScrollBar, SB_CTL, &si);
	if (pos != si.nPos)
	{
		Measurements m(&BackBuf.Rect);
		dataPosX = (si.nPos / cHorzScrollFactor) * m.viewStep;
		SetEditing(false);
		RepaintAll();
	}
}


//------------------------------------------------------------------
//  Scroll handler
//------------------------------------------------------------------
void DoScroll(WPARAM wParam)
{
	SCROLLINFO si = {0};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hScrollBar, SB_CTL, &si);
	switch ((int)LOWORD(wParam)) {
	case SB_PAGEUP:
		si.nPos -= dataWidth * 32;
		break;
	case SB_PAGEDOWN:
		si.nPos += dataWidth * 32;
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
		si.nPos -= dataWidth;
		break;
	case SB_LINEDOWN:
		si.nPos += dataWidth;
		break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
	GetScrollInfo(hScrollBar, SB_CTL, &si);
	memoryOffset = si.nPos;//roundDn(si.nPos,dataWidth); nice but unusable with widths>16

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
void SetEditing(bool tf) 
{
	Editing = tf;
	if (Editing) 
	{
		std::string s = "Editing " + ToHexString(editAddress,6,true);
		SetDlgItemText(hDlgMem, IDC_ADRTXT, s.c_str());
		SetFocus(hEditVal);
	} 
	else 
	{
		ResetMemoryCache();
		SetFocus(hEditAdrBeg);
		UpdateWidthDisplay();
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

void SetWindowRect()
{
	if (hDlgMem != nullptr)
	{
		Rect rect;
		auto& s = Setting();
		rect.w = s.read(cMemoryWindow, cWindowSizeX, 640);
		rect.h = s.read(cMemoryWindow, cWindowSizeY, 480);
		rect.x = s.read(cMemoryWindow, cWindowPosX, CW_USEDEFAULT);
		rect.y = s.read(cMemoryWindow, cWindowPosY, CW_USEDEFAULT);

		if (GetAsyncKeyState(VK_SHIFT))
		{
			rect.x = CW_USEDEFAULT;
			rect.y = CW_USEDEFAULT;
			rect.w = 640;
			rect.h = 480;
		}

		RECT ra = { 0,0,0,0 };  // left,top,right,bottom
		::AdjustWindowRect(&ra, WS_OVERLAPPEDWINDOW, TRUE);
		int windowBorderWidth = ra.right - ra.left;
		int windowBorderHeight = ra.bottom - ra.top;
		::GetWindowRect(hDlgMem, &ra);

		int width = rect.w + windowBorderWidth;
		int height = rect.h + windowBorderHeight + 0; /*GetRenderWindowStatusBarHeight();*/
		int flags = SWP_NOOWNERZORDER | SWP_NOZORDER;
		int x = rect.IsDefaultX() ? ra.left : rect.x;
		int y = rect.IsDefaultY() ? ra.top : rect.y;
		SetWindowPos(hDlgMem, nullptr, x, y, width, height, flags);
	}
}

} }  // end namespace

//------------------------------------------------------------------
// Launch Memory Dialog
//------------------------------------------------------------------
void VCC::Debugger::UI::OpenMemoryMapWindow(HINSTANCE hInst,HWND parent)
{
	LoadSettings();

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
	SetWindowRect();
	UpdateVertScrollBar();
	UpdateHorzScrollBar();
}

