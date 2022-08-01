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

	Processor State Display - Part of the Debugger package for VCC
	Author: Mike Rojas
*/

#include <afx.h>
#include "defines.h"
#include "resource.h"

using namespace std;

extern SystemState EmuState;

namespace MMUMonitor
{
	HWND hWndMMUMonitor;
	HWND hWndVScrollBar;
	int memoryOffset = 0;
	int maxPageNo = 0;

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

	void DrawMMUMonitor(HDC hdc, LPRECT clientRect)
	{
		RECT rect = *clientRect;

		// Clear our background.
		HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		FillRect(hdc, &rect, brush);

		HPEN pen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
		HPEN thickPen = (HPEN)CreatePen(PS_SOLID, 2, RGB(192, 192, 192));
		SelectObject(hdc, pen);

		// Draw the border.
		MoveToEx(hdc, rect.left, rect.top, NULL);
		LineTo(hdc, rect.right - 1, rect.top);
		LineTo(hdc, rect.right - 1, rect.bottom - 1);
		LineTo(hdc, rect.left, rect.bottom - 1);
		LineTo(hdc, rect.left, rect.top);

		HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));

		SelectObject(hdc, hFont);

		RECT rc;
		int x = rect.left + 45;
		int y = rect.top + 10;

		SetTextColor(hdc, RGB(0, 0, 0));
		SetRect(&rc, x, y, x + 80, y + 20);
		DrawText(hdc, "Real Memory", 11, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		x += 100;
		SetRect(&rc, x-5, y, x + 35, y + 20);
		DrawText(hdc, "MAP 0", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 80;
		SetRect(&rc, x, y, x + 80, y + 20);
		DrawText(hdc, "CPU Memory", 10, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 130;
		SetRect(&rc, x-5, y, x + 35, y + 20);
		DrawText(hdc, "MAP 1", 5, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 55;
		SetRect(&rc, x, y, x + 80, y + 20);
		DrawText(hdc, "Real Memory", 11, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		y += 25;

		// Pull out MMU state.  Do it quickly to keep the emulator frame rate high.
		unsigned char regs[20];
		unsigned char page[8192];
		unsigned char pageNo;

		EnterCriticalSection(&EmuState.WatchCriticalSection);
		memcpy(regs, EmuState.WatchMMUState, sizeof(regs));
		memcpy(page, EmuState.WatchMMUPage, sizeof(page));
		pageNo = EmuState.MMUPage;
		LeaveCriticalSection(&EmuState.WatchCriticalSection);

		CString s;

		// Registers
		for (int n = 0; n < 8; n++)
		{
			x = rect.left + 45;
			if (regs[0] == 0)
			{
				SetTextColor(hdc, RGB(138, 27, 255));
			}
			else
			{
				SetTextColor(hdc, RGB(150, 150, 150));
			}
			SetRect(&rc, x, y, x + 80, y + 20);
			int base = regs[2 + n] * 8192;
			s.Format("%05X-%05X", base, base + 8191);
			DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			x += 100;
			SelectObject(hdc, pen);
			MoveToEx(hdc, x, y, NULL);
			LineTo(hdc, x + 30, y);
			LineTo(hdc, x + 30, y + 20);
			LineTo(hdc, x, y + 20);
			LineTo(hdc, x, y);
			s.Format("%02X", regs[2+n]);
			if (regs[0] == 0)
			{
				SetTextColor(hdc, RGB(0, 0, 0));
			}
			else
			{
				SetTextColor(hdc, RGB(150, 150, 150));
			}
			SetRect(&rc, x, y, x + 30, y + 20);
			DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 30;
			if (regs[0] == 0)
			{
				SelectObject(hdc, thickPen);
				MoveToEx(hdc, x, y + 10, NULL);
				LineTo(hdc, x + 50, y + 10);
			}
			x += 50;
			SetTextColor(hdc, RGB(138, 27, 255));
			SetRect(&rc, x, y, x + 80, y + 20);
			s.Format("%04X-%04X", n * 8192, ((n+1) * 8192) - 1);
			DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 80;
			SelectObject(hdc, pen);
			if (regs[0] == 1)
			{
				SelectObject(hdc, thickPen);
				MoveToEx(hdc, x, y + 10, NULL);
				LineTo(hdc, x + 50, y + 10);
			}
			x += 50;
			SelectObject(hdc, pen);
			MoveToEx(hdc, x, y, NULL);
			LineTo(hdc, x + 30, y);
			LineTo(hdc, x + 30, y + 20);
			LineTo(hdc, x, y + 20);
			LineTo(hdc, x, y);
			if (regs[0] == 1)
			{
				SetTextColor(hdc, RGB(0, 0, 0));
			}
			else
			{
				SetTextColor(hdc, RGB(150, 150, 150));
			}
			s.Format("%02X", regs[10 + n]);
			SetRect(&rc, x, y, x + 30, y + 20);
			DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 50;
			if (regs[0] == 1)
			{
				SetTextColor(hdc, RGB(138, 27, 255));
			}
			else
			{
				SetTextColor(hdc, RGB(150, 150, 150));
			}
			SetRect(&rc, x, y, x + 80, y + 20);
			base = regs[10 + n] * 8192;
			s.Format("%05X-%05X", base, base + 8191);
			DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			y += 25;
		}

		x = rect.left + 35;
		y += 20;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "MMU Enable Bit", 14, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		x += 110;
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + 30, y);
		LineTo(hdc, x + 30, y + 20);
		LineTo(hdc, x, y + 20);
		LineTo(hdc, x, y);
		s.Format("%d", regs[1]);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetRect(&rc, x, y, x + 30, y + 20);
		DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 40;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "$FF90 bit 6", 11, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		x += 100;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "Ram Vectors", 11, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		x += 110;
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + 30, y);
		LineTo(hdc, x + 30, y + 20);
		LineTo(hdc, x, y + 20);
		LineTo(hdc, x, y);
		s.Format("%d", regs[18]);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetRect(&rc, x, y, x + 30, y + 20);
		DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 40;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "$FF90 bit 3", 11, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		x = rect.left + 35;
		y += 20;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "MMU Task Bit", 12, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		x += 110;
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + 30, y);
		LineTo(hdc, x + 30, y + 20);
		LineTo(hdc, x, y + 20);
		LineTo(hdc, x, y);
		s.Format("%d", regs[0]);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetRect(&rc, x, y, x + 30, y + 20);
		DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 40;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "$FF91 bit 0", 11, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		x += 100;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "Rom Mapping", 11, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		x += 110;
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + 30, y);
		LineTo(hdc, x + 30, y + 20);
		LineTo(hdc, x, y + 20);
		LineTo(hdc, x, y);
		s.Format("%d", regs[19]);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetRect(&rc, x, y, x + 30, y + 20);
		DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 40;
		SetTextColor(hdc, RGB(138, 27, 255));
		SetRect(&rc, x, y, x + 100, y + 20);
		DrawText(hdc, "$FF90 bit 1-0", 13, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		int nCol1 = 60;
		int nCol2 = 135;

		x = 0;
		y += 40;

		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, rect.right - 1, y);
		LineTo(hdc, rect.right - 1, rect.bottom - 1);
		LineTo(hdc, x, rect.bottom - 1);
		LineTo(hdc, x, y);

		// Draw our lines.
		MoveToEx(hdc, x, y + 20, NULL);
		LineTo(hdc, rect.right, y + 20);

		MoveToEx(hdc, x + nCol1, y, NULL);
		LineTo(hdc, x + nCol1, rect.bottom);

		MoveToEx(hdc, rect.right - nCol2, y, NULL);
		LineTo(hdc, rect.right - nCol2, rect.bottom);

		// Memory Page
		SetTextColor(hdc, RGB(138, 27, 255));
		{
			RECT rc;
			SetRect(&rc, x, y, x + nCol1, y + 20);
			DrawText(hdc, "Address", 7, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			int mx = nCol1 + 5;
			for (int n = 0; n < 16; n++)
			{
				if (n == 8)
				{
					mx += 15;
				}
				CString s;
				s.Format("%2X", n);
				RECT rc;
				SetRect(&rc, x + mx + (n * 18), y, x + mx + (n * 18) + 20, y + 20);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_VCENTER | DT_SINGLELINE);
			}

			SetRect(&rc, rect.right - nCol2, y, rect.right - 5, y + 20);
			DrawText(hdc, "ASCII", 6, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		// Draw the Address Lines.
		y += 20;
		int h = 18;
		int nRows = 16;
		for (int addrLine = 0; addrLine < nRows; addrLine++)
		{
			SetTextColor(hdc, RGB(138, 27, 255));
			RECT rc;
			{
				CString s;
				s.Format("%06X", addrLine * 16 + memoryOffset + pageNo * 8192);
				SetRect(&rc, x, y, x + nCol1, y + h);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// Pull out 16 bytes from page memory.
			unsigned char line[16];
			for (int n = 0; n < 16; n++)
			{
				unsigned short addr = memoryOffset + (addrLine * 16) + n;
				line[n] = page[addr];
			}

			// Hex Line in Hex Bytes
			SetTextColor(hdc, RGB(0, 0, 0));
			int mx = nCol1 + 5;
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
					mx += 15;
				}
				CString s;
				s.Format("%02X", line[n]);
				SetRect(&rc, x + mx + (n * 18), y, x + mx + (n * 18) + 20, y + h);
				DrawText(hdc, (LPCSTR)s, s.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// ASCII Line
			mx = rect.right - nCol2;
			{
				SetRect(&rc, mx + 5, y, rect.right - 5, y + h);
				DrawText(hdc, (LPCSTR)ascii, ascii.GetLength(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// Draw a separator.
			if (addrLine % 4 == 3)
			{
				MoveToEx(hdc, x, y + h, NULL);
				LineTo(hdc, rect.right, y + h);
			}

			y += h;
		}

		// Cleanup.
		DeleteObject(pen);
		DeleteObject(thickPen);
		DeleteObject(hFont);
	}

	int GetMaximumPage()
	{
		unsigned int MemConfig[4] = { 0x20000,0x80000,0x200000,0x800000 };

		EnterCriticalSection(&EmuState.WatchCriticalSection);
		maxPageNo = MemConfig[EmuState.RamSize] / 8192;
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
		return maxPageNo;
	}

	void ChangeMemoryPage()
	{
		int page = SendDlgItemMessage(hWndMMUMonitor, IDC_SELECT_MMU_PAGE, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		if (page < 0)
		{
			return;
		}
		int max = GetMaximumPage();
		if (page >= max)
		{
			page = 0;
		}
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		EmuState.MMUPage = (unsigned char)page;
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
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

	LRESULT CALLBACK MMUMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			hWndMMUMonitor = hDlg;

			RECT Rect;
			GetClientRect(hDlg, &Rect);

			int ScrollBarTop = 315;
			int ScrollBarWidth = 19;

			hWndVScrollBar = CreateWindowEx(
				0,
				"SCROLLBAR",
				NULL,
				WS_VISIBLE | WS_CHILD | SBS_VERT,
				Rect.right - ScrollBarWidth,
				ScrollBarTop,
				ScrollBarWidth,
				Rect.bottom - 40 - ScrollBarTop,
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
			si.nMax = 0x1FFF;
			si.nPage = 16 * 16;
			si.nPos = 0;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);

			InitBackBuffer(hDlg);

			int max = GetMaximumPage();
			for (int page = 0; page < max; page++)
			{
				CString s;
				int base = page * 8192;
				s.Format("%02X - %05X-%05X", page, base, base + 8191);
				SendDlgItemMessage(hDlg, IDC_SELECT_MMU_PAGE, CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPCSTR)s);
			}

			SetTimer(hDlg, IDT_PROC_TIMER, 64, (TIMERPROC)NULL);

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
			si.nPos = roundUp(si.nPos, 16);
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
				si.nPos -= 16 * 8;
				break;
			case SB_LINEDOWN:
				s = "SB_LINEDOWN";
				si.nPos += 16 * 8;
				break;
			}
			si.nPos = roundUp(si.nPos, 16);
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

			DrawMMUMonitor(backDC, &backRect);
			BitBlt(hdc, 0, 0, backBufferCX, backBufferCY, backDC, 0, 0, SRCCOPY);

			EndPaint(hDlg, &ps);
			break;
		}

		case WM_TIMER:

			switch (wParam)
			{
			case IDT_PROC_TIMER:
				InvalidateRect(hDlg, &backRect, FALSE);
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_SELECT_MMU_PAGE:
				ChangeMemoryPage();
				break;

			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_PROC_TIMER);
				DeleteDC(backDC);
				DestroyWindow(hDlg);
				EmuState.MMUMonitorWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}
}
