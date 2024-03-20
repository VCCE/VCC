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
#include "MMUMonitor.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "tcc1014mmu.h"
#include "resource.h"
#include <string>
#include <stdexcept>

namespace VCC { namespace Debugger { namespace UI { namespace
{

	// Color constants
	const COLORREF rgbLightGray = RGB(192, 192, 192);
	const COLORREF rgbBlack     = RGB(  0,   0,   0);
	const COLORREF rgbViolet    = RGB(100,   0, 200);
	const COLORREF rgbDarkGray  = RGB(150, 150, 150);

	CriticalSection Section_;
	MMUState MMUState_;

	HWND MMUMonitorWindow = NULL;
	HWND hWndMMUMonitor;
	BackBufferInfo	BackBuffer_;

	class MMUMonitorDebugClient : public Client
	{
	public:
		void OnReset() override {
		}
		void OnUpdate() override {
			SectionLocker lock(Section_);
			MMUState_ = GetMMUState();
		}
	};

	// Make uppercase hex string
	std::string HexUpc(int val,int width)
	{
		return ToHexString(val,width,true);
	}

	// Draw some text
	void PutText(HDC hdc,int x,int y,int w,std::string s)
	{
		RECT rc;
		SetRect( &rc, x, y, x+w, y+20);
		DrawText( hdc, s.c_str(), s.size(), &rc,
				  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// Draw a box
	void MakeBox(HDC hdc,HPEN pen,int x,int y,int w,int h) {
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + w, y);
		LineTo(hdc, x + w, y + h);
		LineTo(hdc, x, y + h);
		LineTo(hdc, x, y);
	}

	void DrawMMUMonitor(HDC hdc, LPRECT clientRect)
	{
		RECT rect = *clientRect;

		// Clear background.
		HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		FillRect(hdc, &rect, brush);

		HPEN pen = (HPEN)CreatePen(PS_SOLID, 1, rgbLightGray);
		HPEN thickPen = (HPEN)CreatePen(PS_SOLID, 2, rgbLightGray);
		SelectObject(hdc, pen);

		// Draw the border.
		//MoveToEx(hdc, rect.left, rect.top, NULL);
		//LineTo(hdc, rect.right + 5, rect.top);
		//LineTo(hdc, rect.right + 5, rect.bottom - 2);
		//LineTo(hdc, rect.left, rect.bottom - 2);
		//LineTo(hdc, rect.left, rect.top);

		HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
								 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
								 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
								 FIXED_PITCH, TEXT("Consolas"));

		SelectObject(hdc, hFont);

		std::string s;

		int x = rect.left + 15;
		int y = rect.top + 5;

		// Registers header line
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x,y,80,"Real Memory");
		x += 100;
		PutText(hdc,x-5,y,40,"MAP 0");
		x += 80;
		PutText(hdc,x,y,80,"CPU Memory");
		x += 130;
		PutText(hdc,x-5,y,40,"Map 1");
		x += 50;
		PutText(hdc,x,y,80,"Real Memory");

		// Pull out MMU state. Do it quickly to keep the emulator frame rate high.
		MMUState regs;
		{
			SectionLocker lock(Section_);
			regs = MMUState_;
		}

		// Register Rows
		y += 20;
		for (int n = 0; n < 8; n++)
		{
			x = rect.left + 15;
			if (regs.ActiveTask == 0)
				SetTextColor(hdc, rgbViolet);
			else
				SetTextColor(hdc, rgbDarkGray);
			int base = regs.Task0[n] * 8192;
			s = HexUpc(base, 5) + "-" + HexUpc(base + 8191, 5);
			PutText(hdc,x,y,80,s);

            // Map 0 Box
			x += 100;
			MakeBox(hdc,pen,x,y+1,30,16);
			s = HexUpc(regs.Task0[n], 2);
			if (regs.ActiveTask == 0)
				SetTextColor(hdc, rgbBlack);
			else
				SetTextColor(hdc, rgbDarkGray);
				PutText(hdc,x,y,30,s);

            // Active task left indicator
			x += 30;
			if (regs.ActiveTask == 0) {
				SelectObject(hdc, thickPen);
				MoveToEx(hdc, x, y + 10, NULL);
				LineTo(hdc, x + 50, y + 10);
			}

			// CPU memory
			x += 50;
			SetTextColor(hdc, rgbViolet);
			s = HexUpc(n * 8192, 4) + "-" + HexUpc(((n + 1) * 8192) - 1, 4);
			PutText(hdc,x,y,80,s);

			// Active task right indicator
			x += 80;
			SelectObject(hdc, pen);
			if (regs.ActiveTask == 1) {
				SelectObject(hdc, thickPen);
				MoveToEx(hdc, x, y + 10, NULL);
				LineTo(hdc, x + 50, y + 10);
			}

			// Map 1 Box
			x += 50;
			MakeBox(hdc,pen,x,y+1,30,16);
			if (regs.ActiveTask == 1)
				SetTextColor(hdc, rgbBlack);
			else
				SetTextColor(hdc, rgbDarkGray);
			s = HexUpc(regs.Task1[n], 2);
	        PutText(hdc,x,y,30,s);

			// Map 1 Real Memory Address
			x += 50;
			if (regs.ActiveTask == 1)
				SetTextColor(hdc, rgbViolet);
			else
				SetTextColor(hdc, rgbDarkGray);
			base = regs.Task1[n] * 8192;
			s = HexUpc(base, 5) + "-" + HexUpc(base + 8191, 5);
			PutText(hdc,x,y,80,s);

			// Next row
			y += 20;
		}
		// end registers

		y += 10;

		// MMU Enable bit
		x = rect.left + 10;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"MMU Enable");

		x += 80;
		MakeBox(hdc,pen,x,y+2,30,16);
		s = std::to_string(regs.Enabled);
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x,y,30,s);

		x += 40;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"$FF90 bit 6");

		// Ram Vectors
		x += 100;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"Ram Vectors");

		x += 85;
		MakeBox(hdc,pen,x,y+2,30,16);
		s = std::to_string(regs.RamVectors);
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x,y,30,s);

		x += 40;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"$FF90 bit 3");

		// MMU task bit
		x = rect.left + 10;
		y += 20;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,65,"MMU Task");

		x += 80;
		MakeBox(hdc,pen,x,y+2,30,16);
		s = std::to_string(regs.ActiveTask);
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x,y,30,s);

		x += 40;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"$FF91 bit 0");

		// Rom Mapping
		x += 100;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,80,"Rom Mapping");

		x += 85;
		MakeBox(hdc,pen,x,y+2,30,16);
		s = std::to_string(regs.RomMap);
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x,y,30,s);

		x += 40;
		SetTextColor(hdc, rgbViolet);
		PutText(hdc,x,y,90,"$FF90 bit 1-0");

		// Cleanup.
		DeleteObject(pen);
		DeleteObject(thickPen);
		DeleteObject(hFont);
	}

	INT_PTR CALLBACK MMUMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			hWndMMUMonitor = hDlg;
			RECT Rect;
			GetClientRect(hDlg, &Rect);
			BackBuffer_ = AttachBackBuffer(hDlg, 0, -40);
			SetTimer(hDlg, IDT_PROC_TIMER, 64, (TIMERPROC)NULL);
			EmuState.Debugger.RegisterClient(hDlg, std::make_unique<MMUMonitorDebugClient>());
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			DrawMMUMonitor(BackBuffer_.DeviceContext, &BackBuffer_.Rect);
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

			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_PROC_TIMER);
				DeleteDC(BackBuffer_.DeviceContext);
				DestroyWindow(hDlg);
				MMUMonitorWindow = NULL;
				EmuState.Debugger.RemoveClient(hDlg);
				break;
			}

			break;
		}
		return FALSE;
	}

} } } }


void VCC::Debugger::UI::OpenMMUMonitorWindow(HINSTANCE instance, HWND parent)
{
	if (MMUMonitorWindow == NULL)
	{
		MMUMonitorWindow = CreateDialog(
			instance,
			MAKEINTRESOURCE(IDD_MMU_MONITOR),
			parent,
			MMUMonitorDlgProc);

		ShowWindow(MMUMonitorWindow, SW_SHOWNORMAL);
	}

	SetFocus(MMUMonitorWindow);
}
