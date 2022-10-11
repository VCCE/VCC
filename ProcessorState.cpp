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

#include "defines.h"
#include "DebuggerUtils.h"
#include "resource.h"

extern SystemState EmuState;

namespace ProcessorState
{

	HDC		backDC;
	HBITMAP backBufferBMP;
	RECT    backRect;
	int		backBufferCX;
	int		backBufferCY;

	bool InitBackBuffer(HWND hWnd)
	{
		GetClientRect(hWnd, &backRect);

		backRect.bottom -= 39;

		backBufferCX = backRect.right - backRect.left;
		backBufferCY = backRect.bottom - backRect.top;

		HDC hdc = GetDC(hWnd);

		backBufferBMP = CreateCompatibleBitmap(hdc, backBufferCX, backBufferCY);
		if (backBufferBMP == NULL)
		{
			OutputDebugString("failed to create backBufferBMP");
			return false;
		}

		backDC = CreateCompatibleDC(hdc);
		if (backDC == NULL)
		{
			OutputDebugString("failed to create the backDC");
			return false;
		}

		HBITMAP oldbmp = (HBITMAP)SelectObject(backDC, backBufferBMP);
		DeleteObject(oldbmp);
		ReleaseDC(hWnd, hdc);

		return true;
	}

	void DrawProcessorState(HDC hdc, LPRECT clientRect)
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

		HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));

		SetTextColor(hdc, RGB(138, 27, 255));
		RECT rc;

		// Draw our register boxes.
		int x = rect.left;
		int y = rect.top;

		// Registers
		for (int n = 0; n < 8; n++)
		{
			MoveToEx(hdc, x + 10, y + 20, NULL);
			LineTo(hdc, x + 80, y + 20);
			LineTo(hdc, x + 80, y + 40);
			LineTo(hdc, x + 10, y + 40);
			LineTo(hdc, x + 10, y + 20);
			if (n == 6)
			{
				y += 50;
			}
			else
			{
				x += 80;
			}
		}
		x = rect.left;
		y = rect.top;
		MoveToEx(hdc, x + 45, y + 20, NULL);
		LineTo(hdc, x + 45, y + 40);
		x += 80;
		MoveToEx(hdc, x + 45, y + 20, NULL);
		LineTo(hdc, x + 45, y + 40);

		x = rect.left;
		y = rect.top;
		x += 90;
		y += 50;
		MoveToEx(hdc, x, y + 20, NULL);
		LineTo(hdc, x + 200, y + 20);
		LineTo(hdc, x + 200, y + 40);
		LineTo(hdc, x, y + 40);
		LineTo(hdc, x, y + 20);

		x += 25;
		for (int n = 0; n < 7; n++)
		{
			MoveToEx(hdc, x, y + 20, NULL);
			LineTo(hdc, x, y + 40);
			x += 25;
		}

		x = rect.left + 390;
		MoveToEx(hdc, x, y + 20, NULL);
		LineTo(hdc, x + 45, y + 20);
		LineTo(hdc, x + 45, y + 40);
		LineTo(hdc, x, y + 40);
		LineTo(hdc, x, y + 20);

		x += 50;
		MoveToEx(hdc, x, y + 20, NULL);
		LineTo(hdc, x + 45, y + 20);
		LineTo(hdc, x + 45, y + 40);
		LineTo(hdc, x, y + 40);
		LineTo(hdc, x, y + 20);

		SelectObject(hdc, hFont);

		x = rect.left;
		y = rect.top;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "D", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SetRect(&rc, x + 10, y + 40, x + 45, y + 60);
		DrawText(hdc, "A", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SetRect(&rc, x + 45, y + 40, x + 80, y + 60);
		DrawText(hdc, "B", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "W", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SetRect(&rc, x + 10, y + 40, x + 45, y + 60);
		DrawText(hdc, "E", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SetRect(&rc, x + 45, y + 40, x + 80, y + 60);
		DrawText(hdc, "F", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "X", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "Y", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "S", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "U", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 80;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "PC", 2, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		y += 50;
		SetRect(&rc, x + 10, y, x + 80, y + 20);
		DrawText(hdc, "V", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x = rect.left + 90;
		y = rect.top + 70;
		SetRect(&rc, x - 40, y, x - 5, y + 20);
		DrawText(hdc, "CC", 2, &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		y += 20;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "E", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "F", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "H", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "I", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "N", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "Z", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "V", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 25;
		SetRect(&rc, x, y, x + 25, y + 20);
		DrawText(hdc, "C", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x = rect.left + 390;
		y = rect.top + 50;
		SetRect(&rc, x, y, x + 45, y + 20);
		DrawText(hdc, "DP", 2, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		x += 50;
		SetRect(&rc, x, y, x + 45, y + 20);
		DrawText(hdc, "MD", 2, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetTextColor(hdc, RGB(0, 0, 0));

		// Pull out processor state.  Do it quickly to keep the emulator frame rate high.
		unsigned char regs[19];
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		memcpy(regs, EmuState.WatchProcState, sizeof(regs));
		LeaveCriticalSection(&EmuState.WatchCriticalSection);

		// Dump the registers.
		x = rect.left;
		y = rect.top;
		int gap = 0;
		std::string s;
		for (int n = 0; n < 14; n += 2)
		{
			if (n > 2)
			{
				gap = 15;
			}
			
			s = ToHexString(regs[n], 2);
			SetRect(&rc, x + 10 + gap, y + 20, x + 45, y + 40);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			
			s = ToHexString(regs[n + 1], 2);
			SetRect(&rc, x + 45, y + 20, x + 80 - gap, y + 40);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 80;
		}

		x = rect.left + 90;
		y = rect.top + 70;
		for (int n = 0; n < 8; n++)
		{
			const std::string s((regs[14] & 1 << n) ? "1" : "-");
			SetRect(&rc, x, y, x + 25, y + 20);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 25;
		}

		x = rect.left + 390;
		y = rect.top + 70;
		SetRect(&rc, x, y, x + 45, y + 20);
		s = ToHexString(regs[15], 2);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x += 50;
		s = ToHexString(regs[16], 2);
		SetRect(&rc, x, y, x + 45, y + 20);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x += 40;
		gap = 15;
		s = ToHexString(regs[17], 2);
		SetRect(&rc, x + 10 + gap, y, x + 45, y + 20);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		s = ToHexString(regs[18], 2);
		SetRect(&rc, x + 45, y, x + 80 - gap, y + 20);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetTextColor(hdc, RGB(138, 27, 255));
		y = rect.top + 100;
		s = "W, MD, V are 6309 CPU only";
		SetRect(&rc, rect.right - 200, y, rect.right - 10, y + 20);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

		// Cleanup.
		DeleteObject(pen);
		DeleteObject(hFont);
	}

	void HaltCPU()
	{
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		EmuState.CPUControl = 'H';
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
	}

	void RunCPU()
	{
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		EmuState.CPUControl = 'R';
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
	}

	void StepCPU()
	{
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		EmuState.CPUControl = 'S';
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
	}

	LRESULT CALLBACK ProcessorState(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			RECT Rect;
			GetClientRect(hDlg, &Rect);

			InitBackBuffer(hDlg);

			SetTimer(hDlg, IDT_PROC_TIMER, 64, (TIMERPROC)NULL);

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			DrawProcessorState(backDC, &backRect);
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
			case IDC_BTN_CPU_HALT:
				HaltCPU();
				break;
			case IDC_BTN_CPU_RUN:
				RunCPU();
				break;
			case IDC_BTN_CPU_STEP:
				StepCPU();
				break;
			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_PROC_TIMER);
				DeleteDC(backDC);
				DestroyWindow(hDlg);
				EmuState.ProcessorWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}
}
