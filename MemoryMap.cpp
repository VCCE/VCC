/*
This file is part of VCC (Virtual Color Computer).

	VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.

	Memory Map Display - Part of the Debugger package for VCC
	Author: Mike Rojas
*/

#include <afx.h>
#include "defines.h"
#include "resource.h"
#include <sstream>

using namespace std;

extern SystemState EmuState;

namespace MemoryMap
{
	HWND hWndMemory;
	HWND hWndVScrollBar;
	WNDPROC oldEditProc;

	int memoryOffset = 0;

	HDC		backDC;
	HBITMAP backBufferBMP;
	RECT    backRect;
	int		backBufferCX;
	int		backBufferCY;

	bool InitBackBuffer(HWND hWnd)
	{
		GetClientRect(hWnd, &backRect);

		backRect.right -= 20;
		backRect.bottom -= 39;

		backBufferCX = backRect.right - backRect.left;
		backBufferCY = backRect.bottom - backRect.top;

		HDC hdc = GetDC(hWnd);

		backBufferBMP = CreateCompatibleBitmap(hdc, backBufferCX, backBufferCY);
		if (backBufferBMP == NULL)
		{
			printf("failed to create backBufferBMP");
			return false;
		}

		backDC = CreateCompatibleDC(hdc);
		if (backDC == NULL)
		{
			printf("failed to create the backDC");
			return false;
		}

		HBITMAP oldbmp = (HBITMAP)SelectObject(backDC, backBufferBMP);
		DeleteObject(oldbmp);
		ReleaseDC(hWnd, hdc);
	}

	void DrawMemoryState(HDC hdc, LPRECT clientRect)
	{
		RECT rect = *clientRect;

		// Clear our background.
		HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		FillRect(hdc, &rect, brush);

		HPEN pen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
		SelectObject(hdc, pen);

		// Draw the border.
		MoveToEx(hdc, rect.left, rect.top, NULL);
		LineTo(hdc, rect.right - 1, rect.top);
		LineTo(hdc, rect.right - 1, rect.bottom - 1);
		LineTo(hdc, rect.left, rect.bottom - 1);
		LineTo(hdc, rect.left, rect.top);

		int nCol1 = 60;
		int nCol2 = 135;

		// Draw our lines.
		MoveToEx(hdc, rect.left, rect.top + 20, NULL);
		LineTo(hdc, rect.right, rect.top + 20);

		MoveToEx(hdc, rect.left + nCol1, rect.top, NULL);
		LineTo(hdc, rect.left + nCol1, rect.bottom);

		MoveToEx(hdc, rect.right - nCol2, rect.top, NULL);
		LineTo(hdc, rect.right - nCol2, rect.bottom);

		HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));

		// Draw our header line.
		SelectObject(hdc, hFont);
		SetTextColor(hdc, RGB(138, 27, 255));
		{
			RECT rc;
			SetRect(&rc, rect.left, rect.top, rect.left + nCol1, rect.top + 20);
			DrawText(hdc, "Address", 7, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			int x = nCol1 + 5;
			for (int n = 0; n < 16; n++)
			{
				if (n == 8)
				{
					x += 15;
				}
				CString s;
				s.Format("%2X", n);
				RECT rc;
				SetRect(&rc, rect.left + x + (n * 18), rect.top, rect.left + x + (n * 18) + 20, rect.top + 20);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_VCENTER | DT_SINGLELINE);
			}

			SetRect(&rc, rect.right - nCol2, rect.top, rect.right - 5, rect.top + 20);
			DrawText(hdc, "ASCII", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		// Draw the Address Lines.
		int y = rect.top + 20;
		int h = 18;
		int nRows = 32;
		for (int addrLine = 0; addrLine < nRows; addrLine++)
		{
			SetTextColor(hdc, RGB(138, 27, 255));
			RECT rc;
			{
				CString s;
				s.Format("%06X", addrLine * 16 + memoryOffset);
				SetRect(&rc, rect.left, y, rect.left + nCol1, y + h);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// Pull out 16 bytes from memory.  Do it quickly to keep the emulator frame rate high.
			unsigned char line[16];
			EnterCriticalSection(&EmuState.WatchCriticalSection);
			for (int n = 0; n < 16; n++)
			{
				unsigned short addr = memoryOffset + (addrLine * 16) + n;
				line[n] = EmuState.WatchRamBuffer[addr];
			}
			LeaveCriticalSection(&EmuState.WatchCriticalSection);

			// Now use our copy of memory.

			// Hex Line in Hex Bytes
			SetTextColor(hdc, RGB(0, 0, 0));
			int x = nCol1 + 5;
			CString ascii;
			for (int n = 0; n < 16; n++)
			{
				if (isprint(line[n]))
				{
					ascii += line[n];
				}
				else
				{
					ascii += ".";
				}
				if (n == 8)
				{
					x += 15;
				}
				CString s;
				s.Format("%02X", line[n]);
				SetRect(&rc, rect.left + x + (n * 18), y, rect.left + x + (n * 18) + 20, y + h);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// ASCII Line
			x = rect.right - nCol2;
			{
				SetRect(&rc, x + 5, y, rect.right - 5, y + h);
				DrawText(hdc, (LPCSTR)ascii, ascii.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// Draw a separator.
			if (addrLine % 4 == 3)
			{
				MoveToEx(hdc, rect.left, y + h, NULL);
				LineTo(hdc, rect.right, y + h);
			}

			y += h;
		}

		// Cleanup.
		DeleteObject(pen);
		DeleteObject(hFont);
	}

	int roundUp(int numToRound, int multiple)
	{
		if (multiple == 0)
			return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0)
			return numToRound;

		return numToRound + multiple - remainder;
	}

	int roundDn(int numToRound, int multiple)
	{
		if (multiple == 0)
			return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0)
			return numToRound;

		return numToRound - remainder;
	}

	void LocateMemory()
	{
		TCHAR buffer[100];
		memset(buffer, 0, size(buffer));
		SendDlgItemMessage(hWndMemory, IDC_EDIT_FIND_MEM, WM_GETTEXT, sizeof(buffer), (LPARAM)(LPCSTR)buffer);

		unsigned int addrInt;
		std::stringstream ss;
		ss << buffer;
		ss >> hex >> addrInt;

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWndVScrollBar, SB_CTL, &si);

		si.nPos = roundDn(addrInt, 16);
		si.fMask = SIF_POS;
		SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
		GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
		memoryOffset = si.nPos;
		InvalidateRect(hWndMemory, &backRect, FALSE);
	}

	LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
				LocateMemory();
				return 0;
			}
		default:
			return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
		}
		return 0;
	}

	LRESULT CALLBACK MemoryMap(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			hWndMemory = hDlg;

			int ScrollBarWidth = 19;

			RECT Rect;
			GetClientRect(hDlg, &Rect);

			hWndVScrollBar = CreateWindowEx(
				0,
				"SCROLLBAR",
				NULL,
				WS_VISIBLE | WS_CHILD | SBS_VERT,
				Rect.right - ScrollBarWidth,
				0,
				ScrollBarWidth,
				Rect.bottom - 40,
				hDlg,
				(HMENU)IDC_MEM_VSCROLLBAR,
				(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
				NULL);

			if (!hWndVScrollBar)
			{
				MessageBox(NULL, "Vertical Scroll Bar Failed.", "Error", MB_OK | MB_ICONERROR);
				return 0;
			}

			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0x0000;
			si.nMax = 0xFFFF;
			si.nPage = 32 * 16;
			si.nPos = 0;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);

			HWND hCtl = GetDlgItem(hDlg, IDC_EDIT_FIND_MEM);
			oldEditProc = (WNDPROC)SetWindowLongPtr(hCtl, GWLP_WNDPROC, (LONG_PTR)subEditProc);

			InitBackBuffer(hDlg);

			SetTimer(hDlg, IDT_MEM_TIMER, 64, (TIMERPROC)NULL);

			break;
		}

		case WM_MOUSEWHEEL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);

			int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta < 0)
			{
				si.nPos += si.nPage / 2;
			}
			else
			{
				si.nPos -= si.nPage / 2;
			}
			si.nPos = roundUp(si.nPos, 32);
			si.fMask = SIF_POS;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			memoryOffset = si.nPos;
			InvalidateRect(hDlg, &backRect, FALSE);
			return 0;
		}

		case WM_VSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);

			CString s;
			switch ((int)LOWORD(wParam))
			{
			case SB_PAGEUP:
				s = "SB_PAGEUP";
				si.nPos -= si.nPage;
				break;
			case SB_PAGEDOWN:
				s = "SB_PAGEDOWN";
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
				s = "SB_THUMBPOSITION";
				si.nPos = si.nTrackPos;
				break;
			case SB_THUMBTRACK:
				s = "SB_THUMBTRACK";
				si.nPos = si.nTrackPos;
				break;
			case SB_TOP:
				s = "SB_TOP";
				si.nPos = 0;
				break;
			case SB_BOTTOM:
				s = "SB_BOTTOM";
				si.nPos = 0xFFFF;
				break;
			case SB_ENDSCROLL:
				s = "SB_ENDSCROLL";
				break;
			case SB_LINEUP:
				s = "SB_LINEUP";
				si.nPos -= 32 * 8;
				break;
			case SB_LINEDOWN:
				s = "SB_LINEDOWN";
				si.nPos += 32 * 8;
				break;
			}
			si.nPos = roundUp(si.nPos, 32);
			si.fMask = SIF_POS;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			memoryOffset = si.nPos;
			InvalidateRect(hDlg, &backRect, FALSE);
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			DrawMemoryState(backDC, &backRect);
			BitBlt(hdc, 0, 0, backBufferCX, backBufferCY, backDC, 0, 0, SRCCOPY);

			EndPaint(hDlg, &ps);
			break;
		}

		case WM_TIMER:

			switch (wParam)
			{
			case IDT_MEM_TIMER:
				InvalidateRect(hDlg, &backRect, FALSE);
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_BTN_FIND_MEM:
				LocateMemory();
				break;
			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_MEM_TIMER);
				DeleteDC(backDC);
				DestroyWindow(hDlg);
				HWND hCtl = GetDlgItem(hDlg, IDC_EDIT_FIND_MEM);
				(WNDPROC)SetWindowLongPtr(hCtl, GWLP_WNDPROC, (LONG_PTR)oldEditProc);
				EmuState.MemoryWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}
}

