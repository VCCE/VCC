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
	const COLORREF rgbBlack  = RGB(  0,   0,   0);
	const COLORREF rgbViolet = RGB(140,   0, 160);
	const COLORREF rgbGray   = RGB(120, 120, 120);

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

	// Convert unsigned int to uppercase hex string
	std::string HexUpc(long value,unsigned int width)
	{
		return ToHexString(value, width, true);
	}

	// Draw string centered in rectangle width w, height 20
	// x,y set the top left corner of the centering box
	void PutText(HDC hdc,int x,int y,int w,std::string s)
	{
		RECT rc;
		SetRect( &rc, x, y, x + w, y + 20);
		DrawText( hdc, s.c_str(), s.size(), &rc,
				  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// Draw a visible box
	void MakeBox(HDC hdc,HPEN pen,int x,int y,int w,int h)
	{
		SelectObject(hdc, pen);
		MoveToEx(hdc, x, y, NULL);
		LineTo(hdc, x + w, y);
		LineTo(hdc, x + w, y + h);
		LineTo(hdc, x, y + h);
		LineTo(hdc, x, y);
	}

	// Set draw color per task number
	void SetTaskColor(HDC hdc,int Task,COLORREF Tsk0,COLORREF Tsk1)
	{
		if (Task==1) {
			SetTextColor(hdc, Tsk1);
		} else {
			SetTextColor(hdc, Tsk0);
		}
	}

	// Draw the Monitor Window
	void DrawMMUMonitor(HDC hdc, LPRECT clientRect)
	{
		RECT rect = *clientRect;

		// Clear background.
		HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		FillRect(hdc, &rect, brush);

		HPEN pen = (HPEN)CreatePen(PS_SOLID, 1, rgbGray);
		HPEN thickPen = (HPEN)CreatePen(PS_SOLID, 2, rgbGray);

		HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		                         DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		                         FIXED_PITCH, TEXT("Consolas"));
		SelectObject(hdc, hFont);

		// Quick pull out MMU state
		MMUState regs;
		{
			SectionLocker lock(Section_);
			regs = MMUState_;
		}

		// X,Y position for MMU registers
		int x = rect.left + 4;
		int y = rect.top + 4;

		// Header
		SetTextColor(hdc, rgbBlack);
		PutText(hdc,x+4,  y,80,"Real");
		PutText(hdc,x+71 ,y,40,"MAP 0");
		PutText(hdc,x+121,y,80,"CPU Memory");
		PutText(hdc,x+212,y,40,"MAP 1");
		PutText(hdc,x+239,y,80,"Real");

		// Rows
		for (int n = 0; n < 8; n++)
		{
			// Y position for row
			y += 20;

			// Map 0 Real Memory
			SetTaskColor(hdc,regs.ActiveTask,rgbViolet,rgbGray);
			PutText(hdc,x+14,y,60,HexUpc(regs.Task0[n]*8192,6));

			// Map 0 Box
			MakeBox(hdc,pen,x+76,y+1,30,16);
			SetTaskColor(hdc,regs.ActiveTask,rgbBlack,rgbGray);
			PutText(hdc,x+76,y,30,HexUpc(regs.Task0[n],2));

			// CPU memory
			SetTextColor(hdc, rgbViolet);
			PutText(hdc,x+131,y,60,HexUpc(n * 8192,4));

			// Map 1 Box
			MakeBox(hdc,pen,x+216,y+1,30,16);
			SetTaskColor(hdc,regs.ActiveTask,rgbGray,rgbBlack);
			PutText(hdc,x+216,y,30,HexUpc(regs.Task1[n],2));

			// Map 1 Real Memory
			SetTaskColor(hdc,regs.ActiveTask,rgbGray,rgbViolet);
			PutText(hdc,x+248,y,60,HexUpc(regs.Task1[n]*8192,6));

			// Active task indicator arrows
			SelectObject(hdc, thickPen);
			int dx = (regs.ActiveTask == 0) ? x : x+66;
			PutText(hdc,dx+115,y,5,"<");
			MoveToEx(hdc,dx+120,y+10, NULL);
			LineTo(hdc,dx+135,y+10);
			PutText(hdc,dx+137,y,5,">");
		}

		// Y position for the Control bits
		y = rect.top + 191;

		// MMU Enable bit
		SetTextColor(hdc,rgbViolet);
		PutText(hdc,x,y,130,"FF90:b6 Enable  ");
		MakeBox(hdc,pen,x+130,y+2,20,16);
		SetTextColor(hdc,rgbBlack);
		PutText(hdc,x+130,y,20,std::to_string(regs.Enabled));

		// MMU task bit
		SetTextColor(hdc,rgbViolet);
		PutText(hdc,x,y+20,130,"FF91:b0 MMU Task");
		MakeBox(hdc,pen,x+130,y+22,20,16);
		SetTextColor(hdc,rgbBlack);
		PutText(hdc,x+130,y+20,20,std::to_string(regs.ActiveTask));

		// X position for second column control bits
		x = rect.left + 166;

		// Ram Vectors
		SetTextColor(hdc,rgbViolet);
		PutText(hdc,x,y,140,   "FF90:b3 RAM Vector");
		MakeBox(hdc,pen,x+140,y+2,20,16);
		SetTextColor(hdc,rgbBlack);
		PutText(hdc,x+140,y,20,std::to_string(regs.RamVectors));

		// Rom Mapping
		SetTextColor(hdc,rgbViolet);
		PutText(hdc,x,y+20,140,"FF90:b1-0 ROM Map ");
		MakeBox(hdc,pen,x+140,y+22,20,16);
		SetTextColor(hdc,rgbBlack);
		PutText(hdc,x+140,y+20,20,std::to_string(regs.RomMap));

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
