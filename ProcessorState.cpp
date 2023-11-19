//	This file is part of VCC (Virtual Color Computer).
//	
//		VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//		it under the terms of the GNU General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//	
//		VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU General Public License for more details.
//	
//		You should have received a copy of the GNU General Public License
//		along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
//	
//		Processor State Display - Part of the Debugger package for VCC
//		Authors: Mike Rojas, Chet Simpson
#include "ProcessorState.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "DebuggerUtils.h"
#include "resource.h"
#include <stdexcept>

extern SystemState EmuState;

namespace VCC { namespace Debugger { namespace UI { namespace
{
	HWND	ProcessorStateWindow = NULL;
	BackBufferInfo	BackBuffer_;


	void DrawCenteredText(HDC hdc, int left, int top, int right, int bottom, const std::string& text)
	{
		RECT rc;

		SetRect(&rc, left, top, right, bottom);
		DrawText(hdc, text.c_str(), text.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
		const auto& regs(EmuState.Debugger.GetProcessorStateCopy());

		// Dump the registers.
		x = rect.left;
		y = rect.top;
		int gap = 0;
		std::string s;
		
		DrawCenteredText(hdc, x + 10, y + 20, x + 45, y + 40, ToHexString(regs.A, 2));	//	A
		DrawCenteredText(hdc, x + 45, y + 20, x + 80, y + 40, ToHexString(regs.B, 2));	//	B
		x += 80;
		if (EmuState.CpuType == 1)
		{
			DrawCenteredText(hdc, x + 10, y + 20, x + 45, y + 40, ToHexString(regs.E, 2));	//	E
			DrawCenteredText(hdc, x + 45, y + 20, x + 80, y + 40, ToHexString(regs.F, 2));	//	F
		}
		x += 80;

		DrawCenteredText(hdc, x + 10, y + 20, x + 80, y + 40, ToHexString(regs.X, 4));	//	X
		x += 80;
		DrawCenteredText(hdc, x + 10, y + 20, x + 80, y + 40, ToHexString(regs.Y, 4));	//	Y
		x += 80;
		DrawCenteredText(hdc, x + 10, y + 20, x + 80, y + 40, ToHexString(regs.S, 4));	//	S
		x += 80;
		DrawCenteredText(hdc, x + 10, y + 20, x + 80, y + 40, ToHexString(regs.U, 4));	//	U
		x += 80;
		DrawCenteredText(hdc, x + 10, y + 20, x + 80, y + 40, ToHexString(regs.PC, 4));	//	V
		x += 80;


		x = rect.left + 90;
		y = rect.top + 70;
		for (int n = 0; n < 8; n++)
		{
			s = (regs.CC & 1 << n) ? "1" : "-";
			SetRect(&rc, x, y, x + 25, y + 20);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += 25;
		}

		x = rect.left + 390;
		y = rect.top + 70;
		SetRect(&rc, x, y, x + 45, y + 20);
		s = ToHexString(regs.DP, 2);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		x += 50;
		if (EmuState.CpuType == 1)
		{
			s = ToHexString(regs.MD, 2);
			SetRect(&rc, x, y, x + 45, y + 20);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		x += 40;
		gap = 15;
		if (EmuState.CpuType == 1)
		{
			s = ToHexString(regs.V, 4);
			SetRect(&rc, x + 10 + gap, y, x + 80 - gap, y + 20);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		SetTextColor(hdc, RGB(138, 27, 255));
		y = rect.top + 100;
		s = "W, MD, V are 6309 CPU only";
		SetRect(&rc, rect.right - 200, y, rect.right - 10, y + 20);
		DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

		// Cleanup.
		DeleteObject(pen);
		DeleteObject(hFont);
	}


	INT_PTR CALLBACK ProcessorStateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			RECT Rect;
			GetClientRect(hDlg, &Rect);

			BackBuffer_ = AttachBackBuffer(hDlg, 0, -39);

			SetTimer(hDlg, IDT_PROC_TIMER, 64, (TIMERPROC)NULL);

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			DrawProcessorState(BackBuffer_.DeviceContext, &BackBuffer_.Rect);
			BitBlt(hdc, 0, 0, BackBuffer_.Width, BackBuffer_.Height, BackBuffer_.DeviceContext, 0, 0, SRCCOPY);

			EndPaint(hDlg, &ps);
			break;
		}

		case WM_TIMER:

			switch (wParam)
			{
			case IDT_PROC_TIMER:
				InvalidateRect(hDlg, &BackBuffer_.Rect, FALSE);
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_BTN_SET_PC:
				if (EmuState.Debugger.IsHalted())
				{
					char buf[16];
					GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PC_ADDR), buf, 8);
					char *eptr;
					long val = strtol(buf,&eptr,16);
					if (*buf == '\0' || *eptr != '\0' || val < 0 || val > 65535)
					{
						MessageBox(hDlg,"Invalid hex address","Error",IDOK);
					} else {
						CPUForcePC(val & 0xFFFF);
					}
				} else {
					MessageBox(hDlg,"CPU must be halted to change PC","Error",IDOK);
				}
				break;
			case IDC_BTN_CPU_HALT:
				EmuState.Debugger.QueueHalt();
				break;
			case IDC_BTN_CPU_RUN:
				EmuState.Debugger.QueueRun();
				break;
			case IDC_BTN_CPU_STEP:
				EmuState.Debugger.QueueStep();
				break;
			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_PROC_TIMER);
				DeleteDC(BackBuffer_.DeviceContext);
				DestroyWindow(hDlg);
				ProcessorStateWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}

} } } }


void VCC::Debugger::UI::OpenProcessorStateWindow(HINSTANCE instance, HWND parent)
{
	if (ProcessorStateWindow == NULL)
	{
		ProcessorStateWindow = CreateDialog(
			instance,
			MAKEINTRESOURCE(IDD_PROCESSOR_STATE),
			parent,
			ProcessorStateDlgProc);
		ShowWindow(ProcessorStateWindow, SW_SHOWNORMAL);
	}

	SetFocus(ProcessorStateWindow);
}
