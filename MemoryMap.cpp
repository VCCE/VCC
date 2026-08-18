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
#include <windowsx.h>

namespace VCC::Debugger::UI {
namespace {

MemoryWindow* gMemoryWindows[2] = { 0, 0 };

// windows will allow the track bar to be positioned
// pixel by pixel, but it will snap to the
// nearest whole value. as the horizontal scrolling 
// will use small numbers, this causes a visible jumping
// of the track bar which is distracting, so instead
// allow higher resolution for the track bar and scale
// the result.
constexpr int cHorzScrollFactor = 10;
constexpr int cTopBarStaticWidth = 510;		// the tool bar
constexpr int cTopBarHeight = 34;
constexpr int cHeaderHeight = 20;			// header with labels
constexpr int cVertScrollBarWidth = 20;
constexpr int cHorzScrollBarHeight = 20;	// bottom scroll bar
constexpr int cAddressWidth = 60;			// address column width
constexpr int cAsciiCharWidth = 7;

// Help text
const char *cDbgHelp =
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
	"Click hex title to collapse/expand hex.\n"
	"Click view title to switch between normal/gime width\n"
	"and the full width.\n\n"
	"Other Keys:\n"
	" [  ]  Dec/inc data row width by 1.\n"
	" {  }  Dec/inc data row width by 8.\n"
	"";

}

const std::string MemoryWindow::cMemoryWindow("MemoryWindow");
const std::string MemoryWindow::cMemoryWindow2("MemoryWindow2");
const std::string MemoryWindow::cWindowSizeX("WindowSizeX");
const std::string MemoryWindow::cWindowSizeY("WindowSizeY");
const std::string MemoryWindow::cWindowPosX("WindowPosX");
const std::string MemoryWindow::cWindowPosY("WindowPosY");
const std::string MemoryWindow::cAddress("Address");
const std::string MemoryWindow::cAddressMode("AddressMode");
const std::string MemoryWindow::cViewMode("ViewMode");
const std::string MemoryWindow::cHex("Hex");
const std::string MemoryWindow::cWide("Wide");
const std::string MemoryWindow::cDataWidth("DataWidth");
const std::string MemoryWindow::cDataPosX("DataPosX");

struct MemoryWindow::Measurements
{
	const MemoryWindow* mem;
	int showHex;
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

	const int maxDataWidth[MemoryWindow::VM_MAX] =
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

	const int pixelViewWidth[MemoryWindow::VM_MAX] =
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
	Measurements(const MemoryWindow *mem, LPCRECT clientRect) : mem(mem)
	{
		showHex = mem->showHex;
		showAscii = mem->viewMode == MemoryWindow::VM_ASCII;

		// rect
		top = clientRect->top;
		bottom = clientRect->bottom - cHeaderHeight;
		left = clientRect->left;
		right = clientRect->right;
		height = bottom - top;
		width = right - left;
		
		// hex column info
		hexDigitsWidth = 18;
		hexColumns = mem->dataWidth > 32 ? 32 : mem->dataWidth;
		hexMinColums = hexColumns < 16 ? 16 : hexColumns;
		hexWidth = HexColumnPos(hexMinColums);

		if (!showHex)
		{
			hexWidth = HexColumnPos(0) + 10;
			hexColumns = 0;
		}

		// include a gutter after width of 16
		int gutter = (hexColumns > 16 && hexColumns & 7) ? 10 : 0;

		// view column info
		viewMaxColumns = (mem->dataWidth > maxDataWidth[mem->viewMode] ? maxDataWidth[mem->viewMode] : mem->dataWidth);
		viewColumns = mem->showWideView ? mem->dataWidth : viewMaxColumns;
		viewWidth = width - hexWidth - gutter;				// view width is remaining width
		viewWinWidth = viewColumns * 2 * pixelViewWidth[mem->viewMode];
		viewMaxWidth = (viewWidth - 2) & (~0xF);			// round view width
		viewOffset = 0;

		int charWidth = 16;
		viewStep = 1;
		if (mem->viewMode == MemoryWindow::VM_TEXT40 || mem->viewMode == MemoryWindow::VM_TEXT80)
		{
			charWidth = 4;
			viewStep = 2;
		}

		if (viewMaxWidth > viewColumns * charWidth * pixelViewWidth[mem->viewMode])
			viewMaxWidth = viewColumns * charWidth * pixelViewWidth[mem->viewMode];

		// line rows
		lineHeight = (height / 32) - 1;
		currLineTop = top + cHeaderHeight;

		if (mem->viewMode == MemoryWindow::VM_SG4)
		{
			if (viewMaxWidth > viewColumns * 16)
				viewMaxWidth = viewColumns * 16;
		}

		viewCellWidth = ((float)viewMaxWidth / viewWinWidth) * pixelViewWidth[mem->viewMode] * 2;

		if (mem->viewMode == MemoryWindow::VM_ASCII)
		{
			viewMaxWidth = (viewWidth - 2) & (~0xF);
			viewColumns = std::min(viewMaxWidth / cAsciiCharWidth, mem->dataWidth);
			viewCellWidth = 7;
		}

		if (mem->viewMode == MemoryWindow::VM_PMODE4_NTSC)
			viewOffset = 4;

		// reposition view so hex is always visible
		const float rounding = 0.000001f; // adjust for rounding
		float pos = mem->dataWidth <= hexColumns ? 0 : (float)mem->dataPosX / (mem->dataWidth - hexColumns);
		viewPosX = roundDn(std::max(0, (int)((mem->dataWidth - viewColumns) * (pos + rounding))), viewStep);

	}

	//
	// left edge of hex column
	//
	int HexLeft() const { return left + cAddressWidth + 5; }

	//
	// left edge of view 
	//
	int ViewLeft() const { return right - viewWidth; }

	//
	// left/right pos of hex window in view
	//
	int ViewHexLeft() const { return ViewLeft() + (int)(viewCellWidth * (mem->dataPosX - viewPosX)); }
	int ViewHexRight() const { return ViewHexLeft() + (int)(viewCellWidth * hexColumns); }

	//
	// return pos (pixel offset) of hex value column n
	//
	int HexColumnPos(int n) const
	{
		// hex cell spacing
		const int minorSpacing = hexDigitsWidth;

		// gutter spacing after each 8 values
		const int majorSpacing = 10;

		// calculate minor & major column position
		int minorColumn = n & 7;
		int majorColumn = n / 8;
		return HexLeft() + minorColumn * minorSpacing + majorColumn * (minorSpacing * 8 + majorSpacing);
	}

	//
	// reverse of hexColumnPos(x) given relative pixel offset
	//
	int HexPosColumn(int x) const
	{
		// move origin
		x -= HexLeft();
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
	int RowAddressPos() const
	{
		return left + 10;
	}

	//
	// return current row, centered for text 
	//
	int RowTextY() const
	{
		return currLineTop + (lineHeight - fontHeight) / 2;
	}
};


MemoryWindow::MemoryWindow(int index) :
	hDlgMem(nullptr),
	hScrollBar(nullptr),
	hHorzScrollBar(nullptr),
	hEditAdrBeg(nullptr),
	hEditAdrEnd(nullptr),
	hEditVal(nullptr),
	hStatic(nullptr),
	ramCache(nullptr),
	ramPos(-1),
	addrMode(NotSet),
	viewMode(VM_SG4),
	winIndex(index),
	memSize(0),
	memOffset(0),
	selectionRangeBeg(1),
	selectionRangeEnd(1),
	rom(nullptr),
	isEditing(false),
	showHex(1),
	showWideView(0),
	editAddress(0),
	dataWidth(16),
	dataPosX(0)
{
}


void MemoryWindow::ResetMemoryCache()
{
	ramPos = -1;
	delete[] ramCache;
	ramCache = nullptr;
}

void MemoryBackBufferInfo::Init(HWND hDlgMem)
{
	dataWidth = 640;
	dataHeight = 32;
	dataStride = dataWidth * 4;
	data = new char[dataStride * dataHeight];

	HDC hdc = GetDC(hDlgMem);

	RECT frc;
	SetRect(&frc, 0, 0, 7 * 16, 14);

	// Set display Font
	font = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE,
		FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
		TEXT("Consolas"));

	// Set display pen color
	pen = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));

	UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	// cache main hex font (black)
	fontDC = CreateCompatibleDC(hdc);
	fontBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(fontDC, fontBitmap);
	SelectObject(fontDC, font);
	DrawText(fontDC, "0123456789ABCDEF", 32, &frc, fmt);

	// cache secondary hex font (red)
	fontRedDC = CreateCompatibleDC(hdc);
	fontRedBitmap = CreateCompatibleBitmap(hdc, 7 * 16, 14);
	SelectObject(fontRedDC, fontRedBitmap);
	SelectObject(fontRedDC, font);
	SetTextColor(fontRedDC, RGB(255, 0, 0));  // Red
	DrawText(fontRedDC, "0123456789ABCDEF", 32, &frc, fmt);

	// cache ascii font
	frc.right = 7 * 96;
	fontAsciiDC = CreateCompatibleDC(hdc);
	fontAsciiBitmap = CreateCompatibleBitmap(hdc, 7 * 96, 14);
	SelectObject(fontAsciiDC, fontAsciiBitmap);
	SelectObject(fontAsciiDC, font);
	DrawText(fontAsciiDC, "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[.]^_`abcdefghijklmnopqrstuvwxyz{|}~.", 96, &frc, fmt);

	ReleaseDC(hDlgMem, hdc);
}

void MemoryBackBufferInfo::CleanupPen()
{
	DeleteObject(pen);
	pen = nullptr;
}

void MemoryBackBufferInfo::CleanupFont()
{
	DeleteObject(font);
	font = nullptr;
}

void MemoryBackBufferInfo::CleanupFonts(HWND hWnd)
{
	DeleteObject(fontBitmap);
	DeleteObject(fontRedBitmap);
	DeleteObject(fontAsciiBitmap);
	ReleaseDC(hWnd, fontDC);
	DeleteDC(fontDC);
	fontDC = nullptr;
	ReleaseDC(hWnd, fontRedDC);
	DeleteDC(fontRedDC);
	fontRedDC = nullptr;
	ReleaseDC(hWnd, fontAsciiDC);
	DeleteDC(fontAsciiDC);
	fontAsciiDC = nullptr;
}

void MemoryBackBufferInfo::Cleanup(HWND hWnd)
{
	delete[] data;
	data = nullptr;
	CleanupFonts(hWnd);
	CleanupFont();
	CleanupPen();
	CleanupBitmap();
	CleanupDC(hWnd);
}

//
// refresh the whole window
//
void MemoryWindow::RepaintAll()
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
void MemoryWindow::UpdateVertScrollBar()
{
	SCROLLINFO si = { 0 };
	int visible = 32 * dataWidth;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin = 0;
	si.nPage = visible;
	si.nMax = (memSize > visible) ? memSize - 1 : 0;
	si.nPos = memOffset;
	SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
}


//
// adjust horizontal scroll bar based on data width
//
void MemoryWindow::UpdateHorzScrollBar()
{
	Measurements m(this,&BackBuf.Rect);
	int visible = (cHorzScrollFactor * (showHex ? m.hexColumns : m.viewColumns)) / m.viewStep;
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
void MemoryWindow::UpdateWidthDisplay()
{
	char info[64];
	sprintf(info, "%d", dataWidth);
	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_WIDTH);
	SendMessage(hCtl, WM_SETTEXT, 0, (LPARAM)info);
}

//
// on user entering a new width
//
void MemoryWindow::CommitWidth()
{
	char info[64];
	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_WIDTH);
	ComboBox_GetText(hCtl, info, 64);
	CommitWidth(info);
}

//
// commit new width value or revert
//
void MemoryWindow::CommitWidth(const char* value)
{
	Measurements m(this, &BackBuf.Rect);
	SetFocus(hEditAdrBeg);
	int width = atoi(value);
	if (width < m.viewStep || width > 256)
	{
		UpdateWidthDisplay();
		return;
	}
	dataWidth = width;
	// if goes over end step back one line
	if (dataPosX + m.hexColumns > dataWidth)
		dataPosX = std::max(0, dataWidth - m.hexColumns);
	if (memOffset + 32*dataWidth + dataPosX > memSize)
	{
		dataPosX = 0;
		memOffset = memSize - 32*dataWidth;
	}
	ResetMemoryCache();
	UpdateWidthDisplay();
	UpdateHorzScrollBar();
	UpdateVertScrollBar();
	RepaintAll();
}

//
// on user picking new width from combo box
//
void MemoryWindow::SelectWidth()
{
	char info[64];
	strcpy(info, "-1");
	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_WIDTH);
	int index = (int)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR)
		SendMessage(hCtl, CB_GETLBTEXT, index, (LPARAM)info);
	CommitWidth(info);
}

//
// setup new type of view
//
void MemoryWindow::SetViewType()
{
	HWND hCtl = GetDlgItem(hDlgMem, IDC_VIEW_TYPE);
	ViewMode mode = (ViewMode)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
	if (viewMode == mode) return;
	viewMode = mode;
	if (viewMode == VM_TEXT40 || viewMode == VM_TEXT80)
	{
		memOffset = roundDn(memOffset, 2);
		dataWidth = roundDn(dataWidth, 2);
	}
}

//
// save memory window state
//
void MemoryWindow::SaveSettings()
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
		auto& section = winIndex ? cMemoryWindow2 : cMemoryWindow;
		auto write = [&](const std::string& key, int value)
		{
			s.write(section, key, value);
		};
		write(cWindowSizeX, rect.w);
		write(cWindowSizeY, rect.h);
		write(cWindowPosX, rect.x);
		write(cWindowPosY, rect.y);

		// other state
		write(cAddress, memOffset);
		write(cAddressMode, (int)addrMode);
		write(cViewMode, (int)viewMode);
		write(cHex, (int)showHex);
		write(cWide, (int)showWideView);
		write(cDataWidth, dataWidth);
		write(cDataPosX, dataPosX);
	}
}

//
// load memory window state
//
void MemoryWindow::LoadSettings()
{
	auto& s = Setting();
	auto& section = winIndex ? cMemoryWindow2 : cMemoryWindow;
	auto read = [&](const std::string& key, int& value)
	{
		value = s.read(section, key, value);
	};

	read(cAddress, memOffset);
	read(cAddressMode, (int&)addrMode);
	read(cViewMode, (int&)viewMode);
	read(cHex, (int&)showHex);
	read(cWide, (int&)showWideView);
	read(cDataWidth, dataWidth);
	read(cDataPosX, dataPosX);

	if (GetAsyncKeyState(VK_SHIFT))
	{
		dataPosX = 0;
		dataWidth = 16;
		showWideView = 0;
		showHex = 1;
		viewMode = VM_SG4;
		addrMode = Cpu;
		memOffset = 0;
	}
}

//
// setup for new data width
//
void MemoryWindow::SetupDataWidth(int delta)
{
	Measurements m(this,&BackBuf.Rect);

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
void MemoryWindow::Shutdown()
{
	if (hDlgMem)
	{
		SaveSettings();
		KillTimer(hDlgMem, IDT_MEM_TIMER);
		BackBuf.Cleanup(hDlgMem);
		//DestroyWindow(hDlgMem);
		hDlgMem = nullptr;
		ResetMemoryCache();
	}
}

void MemoryWindow::OnTimer(HWND hDlg)
{
	if (addrMode == AddrMode::Cpu || addrMode == AddrMode::Real)
	{
		InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
	}
}

void MemoryWindow::Paint(HWND hDlg)
{
	if (DrawMemory(BackBuf.DeviceContext, &BackBuf.Rect))
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);
		BitBlt(hdc, 0, 0, BackBuf.Width, BackBuf.Height, BackBuf.DeviceContext, 0, 0, SRCCOPY);
		EndPaint(hDlg, &ps);
	}	
}

void MemoryWindow::Help(HWND hDlg)
{
	MessageBox(hDlg,cDbgHelp,"Usage",0);
	SetFocus(hEditAdrBeg);
}

void MemoryWindow::Export()
{
	LocateMemory();
	ExportMemory();
	SetFocus(hEditAdrBeg);
}

void MemoryWindow::ViewType()
{
	SetViewType();
	UpdateVertScrollBar();
	UpdateHorzScrollBar();
	RepaintAll();
	UpdateWidthDisplay();
}

void MemoryWindow::MemType(HWND hDlg)
{
	SetMemType();
	UpdateVertScrollBar();
	UpdateHorzScrollBar();
	ResetMemoryCache();
	InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
}

void MemoryWindow::LeftButton(int x, int y)
{
	if (addrMode == AddrMode::PAK)
		FlashDialogWindow();
	else
		SetEditPosition(x,y);
}

//------------------------------------------------------------------
//  Display Memory Dialog
//------------------------------------------------------------------
LRESULT CALLBACK MemoryWindowSubclassProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg) 
	{
		case WM_ERASEBKGND:
			return 1;

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
			memoryWindow()->ResizeWindow(width, height);
			return 0;
		}

		case WM_LBUTTONDOWN:
			memoryWindow()->LeftButton(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_VSCROLL:
			memoryWindow()->DoScroll(wParam);
			break;

		case WM_HSCROLL:
			memoryWindow()->DoHorzScroll(wParam);
			break;

		case WM_MOUSEWHEEL:
		{
			auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
			auto amount = (delta / 30) != 0 ? (delta / 30) : delta;
			auto param = amount > 0 ? (WPARAM)SB_LINEUP : (WPARAM)SB_LINEDOWN;
			auto mw = memoryWindow();
			for (int i = 0; i < std::abs(amount); ++i)
				mw->DoScroll((WPARAM)param);
			break;
		}

		case WM_PAINT:
		{
			memoryWindow()->Paint(hDlg);
			break;
		}

		case WM_TIMER:
			switch (wParam) {
				case IDT_MEM_TIMER:
					memoryWindow()->OnTimer(hDlg);
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hDlg);
			break;

		case WM_NCDESTROY:
		{
			auto mw = memoryWindow();
			if (mw)
			{
				mw->Shutdown();
				gMemoryWindows[mw->winIndex] = nullptr;
				delete mw;
			}
			RemoveWindowSubclass(hDlg, MemoryWindowSubclassProc, uIdSubclass);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_MEM_TYPE:
					memoryWindow()->MemType(hDlg);
					break;
				case IDC_VIEW_TYPE:
					memoryWindow()->ViewType();
					break;
				case IDC_BTN_EXPORT_MEM:
					memoryWindow()->Export();
					break;
				case IDC_BTN_HELP:
					memoryWindow()->Help(hDlg);
					break;
				case IDCLOSE:
					break;
			}
			break;
		}
	}

	return DefSubclassProc(hDlg, uMsg, wParam, lParam);
}

//
// resize the window
//
void MemoryWindow::ResizeWindow(int width, int height)
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


void MemoryWindow::Escape()
{
	SetEditing(false);
	UpdateWidthDisplay();
	SetFocus(hEditAdrBeg);
	ResetMemoryCache();
	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}


//
// Subclassed Edit Value Proc
//
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg)
	{
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
				case VK_RETURN:
				case VK_ESCAPE:
					return 0;
			}
			break;
		}

		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_RETURN:
					memoryWindow()->CommitValue();
					return 0;
				case VK_TAB:
				case VK_ESCAPE:
					memoryWindow()->Escape();
					return 0;
				case VK_UP:
				case VK_DOWN:
				case VK_PRIOR:
				case VK_NEXT:
				case VK_HOME:
				case VK_END:
					memoryWindow()->FlashDialogWindow();
					return 0;
				case VK_OEM_4:
				case VK_OEM_6:
					return 0;
			}
			break;
		}

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, &EditSubclassProc, uIdSubclass);
			break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//
// width combo box edit field handling
//
LRESULT CALLBACK MemWidthEditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg)
	{
		case WM_CHAR:
		case WM_KEYDOWN:
		{
			switch (wParam) 
			{
				case VK_RETURN:
					memoryWindow()->CommitWidth();
					return 0;
				case VK_TAB:
				case VK_ESCAPE:
					memoryWindow()->Escape();
					return 0;
			}
			break;
		}

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, &MemWidthEditSubclassProc, uIdSubclass);
			break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


//
// width combo box handling
//
LRESULT CALLBACK MemWidthComboSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case CBN_SELCHANGE:
				case CBN_EDITCHANGE:
					memoryWindow()->SelectWidth();
					return 0;
			}
			break;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, &MemWidthComboSubclassProc, uIdSubclass);
			break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}



void MemoryWindow::SelectAdrEnd()
{
	LocateMemory();
	SendMessage(hEditAdrEnd, EM_SETSEL, 0, -1);
	SetFocus(hEditAdrEnd);
}

void MemoryWindow::SelectAdrBeg()
{
	LocateMemory();
	SetFocus(hEditAdrBeg);
}

//
// Subclassed Edit Address Proc
//
LRESULT CALLBACK EditAdrBegSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg) 
	{
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
			switch (wParam) 
			{
				case VK_RETURN:
					memoryWindow()->LocateMemory();
					return 0;
				case VK_TAB:
					memoryWindow()->SelectAdrEnd();
					return 0;
				case VK_UP:
					memoryWindow()->DoScroll((WPARAM)SB_LINEUP);
					return 0;
				case VK_DOWN:
					memoryWindow()->DoScroll((WPARAM)SB_LINEDOWN);
					return 0;
				case VK_PRIOR:
					memoryWindow()->DoScroll((WPARAM)SB_PAGEUP);
					return 0;
				case VK_NEXT:
					memoryWindow()->DoScroll((WPARAM)SB_PAGEDOWN);
					return 0;
				case VK_HOME:
					memoryWindow()->DoScroll((WPARAM)SB_TOP);
					return 0;
				case VK_END:
					memoryWindow()->DoScroll((WPARAM)SB_BOTTOM);
					return 0;
				case VK_OEM_4:
					memoryWindow()->SetupDataWidth(-1);
					return 0;
				case VK_OEM_6:
					memoryWindow()->SetupDataWidth(1);
					return 0;

			}
			break;

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, &EditAdrBegSubclassProc, uIdSubclass);
			break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);

}

LRESULT CALLBACK EditAdrEndSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	auto memoryWindow = [dwRefData]() { return (MemoryWindow*)dwRefData; };

	switch (uMsg) 
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
					memoryWindow()->LocateMemory();
					return 0;
				case VK_TAB:
					memoryWindow()->SelectAdrBeg();
					return 0;
				case VK_UP:
					memoryWindow()->DoScroll((WPARAM)SB_LINEUP);
					return 0;
				case VK_DOWN:
					memoryWindow()->DoScroll((WPARAM)SB_LINEDOWN);
					return 0;
				case VK_PRIOR:
					memoryWindow()->DoScroll((WPARAM)SB_PAGEUP);
					return 0;
				case VK_NEXT:
					memoryWindow()->DoScroll((WPARAM)SB_PAGEDOWN);
					return 0;
				case VK_HOME:
					memoryWindow()->DoScroll((WPARAM)SB_TOP);
					return 0;
				case VK_END:
					memoryWindow()->DoScroll((WPARAM)SB_BOTTOM);
					return 0;
				case VK_OEM_4:
					memoryWindow()->SetupDataWidth(-1);
					return 0;
				case VK_OEM_6:
					memoryWindow()->SetupDataWidth(1);
					return 0;
			}
			break;

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, &EditAdrEndSubclassProc, uIdSubclass);
			break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//------------------------------------------------------------------
// Read a byte from Coco Memory
//------------------------------------------------------------------
unsigned char MemoryWindow::ReadMemory(int addr)
{
	switch (addrMode) {

	case AddrMode::Cpu:
		return SafeMemRead8(addr & 0xFFFF);

	case AddrMode::Real:
		return (unsigned char) GetMem(addr);

	case AddrMode::ROM:
		if (rom == nullptr) rom = Getint_rom_pointer();
		return rom[addr & 0x7FFF];

	case AddrMode::PAK:
		return PackMem8Read(addr & 0x7FFF);
	}
	return 0;
}

//------------------------------------------------------------------
// Write a byte to Coco Memory
//------------------------------------------------------------------
void MemoryWindow::WriteMemory(int addr, unsigned char value)
{
	switch (addrMode) {

	case AddrMode::Cpu:
		EmuState.Debugger.QueueWrite(addr & 0xFFFF, value);
		break;

	case AddrMode::Real:
		SetMem(addr,value);
		break;

	case AddrMode::ROM:
		if (rom == nullptr) rom = Getint_rom_pointer();
		rom[addr & 0x7FFF] = value;
		break;

	case AddrMode::PAK:
		FlashDialogWindow();
	}
}

//------------------------------------------------------------------
// Setup back buffer for data display
//------------------------------------------------------------------
void MemoryWindow::SetBackBuffer(const RECT& rc)
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

	SelectObject(BackBuf.DeviceContext, BackBuf.pen);
	SelectObject(BackBuf.DeviceContext, BackBuf.font);
}

//------------------------------------------------------------------
// Create vertical scroll bar
//------------------------------------------------------------------
void MemoryWindow::CreateScrollBar(const RECT& Rect)
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
			this);

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
			this);

		if (!hHorzScrollBar) {
			MessageBox(nullptr, "Horizontal Scroll Bar Failed.", "Error",
				MB_OK | MB_ICONERROR);
		}
}

//------------------------------------------------------------------
// Draw display form with header and vert guide lines
//------------------------------------------------------------------
void MemoryWindow::DrawForm(HDC hdc,LPCRECT clientRect)
{
	RECT rc;
	Measurements m(this,clientRect);

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
		rc.left = m.HexColumnPos(0) + 10;
		DrawText(hdc, ">", 1, &rc, fmt);
	}
	for (int n = 0; n < m.hexColumns; n++)
	{
		SetRect(&rc, m.HexColumnPos(n), m.top, m.HexColumnPos(n) + m.hexDigitsWidth - 4, m.top + cHeaderHeight);
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
	if (showWideView) strcat(temp, " - Wide");
	DrawText(hdc, temp, strlen(temp), &rc, fmt);

	// draw hex region
	int left = std::max(m.ViewLeft(), m.ViewHexLeft());
	int right = m.ViewHexRight();
	SetRect(&rc, left, m.top + cHeaderHeight - 4, right, m.top + cHeaderHeight - 2);

	// draw horz bar
	HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	FillRect(BackBuf.DeviceContext, &rc, blackBrush);

	auto drawBarEnd = [blackBrush,this](RECT rc, int x)
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
bool MemoryWindow::DrawMemory(HDC hdc, LPCRECT clientRect)
{
	bool dirty = false;
	bool forcedUpdate = false;

	memGpu.GimeReset();
	memGpu.SetCompatMode(1);
	memGpu.SetMonitorType(1);

	// if palette changes pixels will need rewriting
	if (memGpu.CopyPalette(gGimeGpu))
		ResetMemoryCache();

	auto pmode = [this](int mode, int mode2) 
	{ 
		memGpu.SetGimeVdgMode(mode);
		memGpu.SetGimeVdgMode2(mode2); 
	};

	auto mon = [this](int monitor)
	{
		memGpu.SetMonitorType(monitor);
	};

	auto gime = [this](int mode, int vres)
	{
		memGpu.SetCompatMode(0);
		memGpu.SetGimeVmode(mode);
		memGpu.SetGimeVres(vres);
	};

	if (viewMode == VM_SG4)	memGpu.SetVidMask(524287);
	else if (viewMode == VM_PMODE4_NTSC) pmode(6,31),mon(0);
	else if (viewMode == VM_PMODE0_S0) pmode(3,22);
	else if (viewMode == VM_PMODE0_S1) pmode(3,23);
	else if (viewMode == VM_PMODE1_S0) pmode(4,24);
	else if (viewMode == VM_PMODE1_S1) pmode(4,25);
	else if (viewMode == VM_PMODE2_S0) pmode(5,26);
	else if (viewMode == VM_PMODE2_S1) pmode(5,27);
	else if (viewMode == VM_PMODE3_S0) pmode(6,28);
	else if (viewMode == VM_PMODE3_S1) pmode(6,29);
	else if (viewMode == VM_PMODE4_RGB_S0) pmode(6,30),mon(1);
	else if (viewMode == VM_PMODE4_RGB_S1) pmode(6,31);
	else if (viewMode == VM_TEXT40) gime(3,5);
	else if (viewMode == VM_TEXT80) gime(3,21);
	else if (viewMode == VM_HSCREEN1) gime(128,21);
	else if (viewMode == VM_HSCREEN2) gime(128,122);
	else if (viewMode == VM_HSCREEN3) gime(128,20);
	else if (viewMode == VM_HSCREEN4) gime(128,29);

	memGpu.SetupDisplay();
	memGpu.VertCenter = 0;
	memGpu.HorzCenter = 0;

	Measurements m(this,clientRect);

	int pixelHeight = memGpu.LinesperRow;
	bool hlfound = false;
	UINT fmt = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	if (!ramCache)
	{
		forcedUpdate = true;
		ramCache = new unsigned char[dataWidth * 32];
	}

	// if not the same address then it will need repainting
	if (memOffset != ramPos)
	{
		dirty = true;
		ramPos = memOffset;
	}

	for (int lnum = 0; lnum < 32; lnum++, m.currLineTop += m.lineHeight)
	{
		unsigned int offset = lnum * dataWidth;
		unsigned int address = memOffset + offset;

		int x = m.RowAddressPos();
		int y = m.RowTextY()+1;

		// Draw address of start of line
		BitBlt(hdc, x, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 20) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 7, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 16) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 14, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 12) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 21, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 8) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 28, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 4) & 15), 1, SRCCOPY);
		BitBlt(hdc, x + 35, y, 7, 12, BackBuf.fontDC, 7 * ((address >> 0) & 15), 1, SRCCOPY);

		// update ram cache
		bool update = isEditing || forcedUpdate;
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
			bool isRed = isEditing && editAddress == address + dataPosX + n;
			if (isRed) hlfound = true;

			x = m.HexColumnPos(n);
			BitBlt(hdc, x, y, 7, 11, isRed ? BackBuf.fontRedDC : BackBuf.fontDC, 7 * (val >> 4), 1, SRCCOPY);
			BitBlt(hdc, x + 7, y, 7, 11, isRed ? BackBuf.fontRedDC : BackBuf.fontDC, 7 * (val & 15), 1, SRCCOPY);
		}

		// render ascii
		if (m.showAscii)
		{
			for (int n = 0; n < m.viewColumns; n++)
			{
				unsigned char val = ramCache[offset + n + m.viewPosX];
				auto ch = val >= 32 && val < 127 ? val : '.';
				x = m.right - m.viewWidth + n * 7 + 1;
				BitBlt(hdc, x, y, 7, 11, BackBuf.fontAsciiDC, cAsciiCharWidth * (ch - 34), 1, SRCCOPY);
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
					memGpu.UpdateScreen32To(ramCache + lnum * dataWidth + m.viewPosX + columnPos, (unsigned int*)BackBuf.data, j, BackBuf.dataWidth, false);

				// blit to back buffer
				HBITMAP bm = CreateBitmap(BackBuf.dataWidth, BackBuf.dataHeight, 1, 32, BackBuf.data);
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
	if (isEditing && !hlfound) {
		SetEditing(false);
	}

	return dirty;
}

//------------------------------------------------------------------
//  Determine byte to edit based on click location
//------------------------------------------------------------------
void MemoryWindow::SetEditPosition(int xPos, int yPos)
{
	Measurements m(this,&BackBuf.Rect);

	auto edit = [this](bool b)
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
		if (xPos > m.HexColumnPos(0) - 10 && xPos < m.HexColumnPos(m.showHex ? m.hexColumns : 1))
		{
			showHex ^= 1;
			UpdateHorzScrollBar();
			RepaintAll();
		}
		else if (xPos > m.HexColumnPos(m.showHex ? m.hexColumns : 1))
		{
			showWideView ^= 1;
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
	int col = m.HexPosColumn(xPos);

	// if out of bounds abort
	if (col < 0 || col >= m.hexColumns) return edit(false);

	// hit
	editAddress = memOffset + col + row * dataWidth + dataPosX;
	return edit(true);
}

//------------------------------------------------------------------
// Determine data to display based on address box
//------------------------------------------------------------------
void MemoryWindow::LocateMemory()
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

	std::string begStr = ToHexString(selectionRangeBeg, 6, true);
	std::string endStr = ToHexString(selectionRangeEnd, 6, true);

	SetDlgItemText(hDlgMem, IDC_EDIT_RANGE_BEG, begStr.c_str());
	SetDlgItemText(hDlgMem, IDC_EDIT_RANGE_END, endStr.c_str());

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

	memOffset = si.nPos;

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}

//------------------------------------------------------------------
// Export the selected range to disk
//------------------------------------------------------------------
void MemoryWindow::ExportMemory()
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
void MemoryWindow::CommitValue()
{
	if (!isEditing) {
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
	if (editAddress >= memSize) {
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
void MemoryWindow::SetMemType()
{
	int PhySiz[4] = { 0x20000,0x80000,0x200000,0x800000 };

	HWND hCtl = GetDlgItem(hDlgMem, IDC_MEM_TYPE);
	AddrMode mode = (AddrMode)SendMessage(hCtl, CB_GETCURSEL, 0, 0);
	if (addrMode == mode) return;  // Not changed

	addrMode = mode;
	switch (addrMode) {
		case AddrMode::Cpu:
			memSize = 0x10000;
			break;
		case AddrMode::Real:
			memSize = PhySiz[EmuState.RamSize];
			break;
		case AddrMode::ROM:
			memSize = 0x8000;
			break;
		case AddrMode::PAK:
			memSize = 0x8000;
			break;
	}

	if (memOffset > memSize)
		memOffset = 0;
}


//------------------------------------------------------------------
// Memory Dialog initialization
//------------------------------------------------------------------
void MemoryWindow::InitializeDialog(HWND hDlg)
{
		hDlgMem = hDlg;

		RECT Rect;
		GetClientRect(hDlg, &Rect);

		BackBuf.Init(hDlg);
		SetBackBuffer(Rect);
		CreateScrollBar(Rect);

		hStatic = GetDlgItem(hDlg,-1);

		//Subclass edit boxes
		hEditAdrBeg = GetDlgItem(hDlg, IDC_EDIT_RANGE_BEG);
		SetWindowSubclass(hEditAdrBeg, &EditAdrBegSubclassProc, (UINT_PTR)this, (DWORD_PTR)this);

		hEditAdrEnd = GetDlgItem(hDlg, IDC_EDIT_RANGE_END);
		SetWindowSubclass(hEditAdrEnd, &EditAdrEndSubclassProc, (UINT_PTR)this, (DWORD_PTR)this);

		hEditVal = GetDlgItem(hDlg, IDC_EDIT_VALUE);
		SetWindowSubclass(hEditVal, &EditSubclassProc, (UINT_PTR)this, (DWORD_PTR)this);

		SetTimer(hDlg, IDT_MEM_TIMER, 1000/60, nullptr);

		// Dropdown to select memory type displayed
		HWND hCtl = GetDlgItem(hDlg, IDC_MEM_TYPE);
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "CPU");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "REAL");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "ROM");
		SendMessage(hCtl,CB_ADDSTRING,(WPARAM) 0, (LPARAM) "PAK");
		SendMessage(hCtl,CB_SETCURSEL,(WPARAM)addrMode, (LPARAM) 0);

		// force this to update to set MemSize correctly
		addrMode = NotSet;
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

		hCtl = GetDlgItem(hDlg, IDC_MEM_WIDTH);
		COMBOBOXINFO cbi = { sizeof(cbi) };
		if (GetComboBoxInfo(hCtl, &cbi) && cbi.hwndItem)
			SetWindowSubclass(cbi.hwndItem, &MemWidthEditSubclassProc, (UINT_PTR)this, (DWORD_PTR)this);
		SetWindowSubclass(hCtl, &MemWidthComboSubclassProc, (UINT_PTR)this, (DWORD_PTR)this);
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"16");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"20");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"32");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"40");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"64");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"80");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"128");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"160");
		SendMessage(hCtl, CB_ADDSTRING, (WPARAM)0, (LPARAM)"256");


		SetBackBuffer(Rect);

		// Draw the form for memory data
		DrawForm(BackBuf.DeviceContext, &BackBuf.Rect);

		// Not edit mode
		SetEditing(false);

		UpdateVertScrollBar();
		UpdateHorzScrollBar();
}


void MemoryWindow::DoHorzScroll(WPARAM wParam)
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
		Measurements m(this,&BackBuf.Rect);
		dataPosX = (si.nPos / cHorzScrollFactor) * m.viewStep;
		SetEditing(false);
		RepaintAll();
	}
}


//------------------------------------------------------------------
//  Scroll handler
//------------------------------------------------------------------
void MemoryWindow::DoScroll(WPARAM wParam)
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
	memOffset = roundDn(si.nPos,dataWidth);

	InvalidateRect(hDlgMem, &BackBuf.Rect, FALSE);
}

//------------------------------------------------------------------
// Convert hexadecimal string to a positive long. Return -1 on error
//------------------------------------------------------------------
int MemoryWindow::CStrToHex(const char * buf)
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
void MemoryWindow::SetEditing(bool tf) 
{
	isEditing = tf;
	if (isEditing) 
	{
		std::string s = "Editing " + ToHexString(editAddress,6,true);
		SetDlgItemText(hDlgMem, IDC_ADRTXT, s.c_str());
		ShowWindow(hEditVal, SW_SHOW);
		SetFocus(hEditVal);
	} 
	else 
	{
		ResetMemoryCache();
		UpdateWidthDisplay();
		SetFocus(hEditAdrBeg);
		SetDlgItemText(hDlgMem, IDC_ADRTXT, "");
		ShowWindow(hEditVal, SW_HIDE);
	}
	SetDlgItemText(hDlgMem, IDC_EDIT_VALUE, "");
}

//------------------------------------------------------------------
//  Input error flash
//------------------------------------------------------------------
void MemoryWindow::FlashDialogWindow()
{
	FlashWindow(hDlgMem,true);
	Sleep(350);
	FlashWindow(hDlgMem,false);
}

void MemoryWindow::SetWindowRect()
{
	if (hDlgMem != nullptr)
	{
		Rect rect;

		rect.x = CW_USEDEFAULT;
		rect.y = CW_USEDEFAULT;
		rect.w = 640;
		rect.h = 480;

		if (!GetAsyncKeyState(VK_SHIFT))
		{
			auto& s = Setting();
			auto& section = winIndex ? cMemoryWindow2 : cMemoryWindow;
			auto read = [&](const std::string& key, int& value)
			{
				value = s.read(section, key, value);
			};

			read(cWindowSizeX, rect.w);
			read(cWindowSizeY, rect.h);
			read(cWindowPosX, rect.x);
			read(cWindowPosY, rect.y);
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

bool MemoryWindow::Init()
{
	if (hDlgMem == nullptr)
	{
		return false;
	}

	ShowWindow(hDlgMem, SW_SHOWNORMAL);
	SetFocus(hEditAdrBeg);
	SetWindowRect();
	return true;

}

//
// main dialog procecure, only contains initdialog
//
INT_PTR CALLBACK MemoryWindowDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG) 
	{
		auto memoryWindow = [lParam]() { return (MemoryWindow*)lParam; };
		SetWindowSubclass(hDlg, MemoryWindowSubclassProc, lParam, lParam);
		memoryWindow()->InitializeDialog(hDlg);
		
		// set focus
		return TRUE;
	}
	return FALSE;
}


}  // end namespace

//------------------------------------------------------------------
// Launch Memory Dialog
//------------------------------------------------------------------
void VCC::Debugger::UI::OpenMemoryMapWindow(HINSTANCE hInst,HWND parent)
{
	int i = 0;
	while (i < 2 && gMemoryWindows[i]) ++i;
	if (i == 2) return;

	auto memoryWindow = gMemoryWindows[i] = new MemoryWindow(i);
	memoryWindow->LoadSettings();
	CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_MEMORY_MAP),
		            parent, MemoryWindowDialogProc, (LPARAM)memoryWindow);

	if (!memoryWindow->Init())
	{
		MessageBox(nullptr, "CreateDialog", "Error", MB_OK | MB_ICONERROR);
		delete memoryWindow;
	}
}

