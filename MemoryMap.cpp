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
//		Memory Map Display - Part of the Debugger package for VCC
//		Authors: Mike Rojas, Chet Simpson
#include "MemoryMap.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "tcc1014mmu.h"
#include "resource.h"
#include <sstream>


namespace VCC { namespace Debugger { namespace UI { namespace
{
	CriticalSection Section_;

	BackBufferInfo	BackBuffer_;

	std::array<unsigned char, 64 * 1024> RamBuffer_;

	HWND MemoryMapWindow = NULL;
	HWND hWndMemory;
	HWND hWndVScrollBar;
	WNDPROC oldEditProc;

	int memoryOffset = 0;

	enum EditState 
	{
		None,
		Editing,
		EditChar1,
		EditChar2
	};
	EditState EditState_ = EditState::None;

	unsigned char editKeys[2];
	unsigned short editAddress = 0;

	HHOOK hHookKeyboard = NULL;

	void HandleEdit(WPARAM key);

	unsigned char HexToInt(unsigned char c)
	{
		if (c <= 0x39)
			return c - 0x30;
		if (c <= 0x46)
			return c - 0x41 + 10;
		return 0;
	}

	class MemoryMapDebugClient : public Client
	{
	public:

		void OnReset() override
		{
			SectionLocker lock(Section_);

			std::fill(RamBuffer_.begin(), RamBuffer_.end(), unsigned char(0));
		}

		void OnUpdate() override
		{
			SectionLocker lock(Section_);

			for (auto addr = 0U; addr < RamBuffer_.size(); addr++)
			{
				RamBuffer_[addr] = SafeMemRead8(static_cast<unsigned short>(addr));
			}
		}

	};


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

				const std::string s(ToHexString(n, 2, false));
				SetRect(&rc, rect.left + x + (n * 18), rect.top, rect.left + x + (n * 18) + 20, rect.top + 20);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_VCENTER | DT_SINGLELINE);
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
				const std::string s(ToHexString(addrLine * 16 + memoryOffset, 6, true));

				SetRect(&rc, rect.left, y, rect.left + nCol1, y + h);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// Pull out 16 bytes from memory.  Do it quickly to keep the emulator frame rate high.
			unsigned char line[16];
			{
				SectionLocker lock(Section_);
				for (int n = 0; n < 16; n++)
				{
					auto addr = memoryOffset + (addrLine * 16) + n;
					line[n] = RamBuffer_[addr];
				}
			}

			// Now use our copy of memory.

			// Hex Line in Hex Bytes
			SetTextColor(hdc, RGB(0, 0, 0));
			int x = nCol1 + 5;
			std::string ascii;
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

				std::string s;

				int v = line[n];

				// Is this an Address we are editing?
				if (EditState_ != EditState::None && editAddress == (addrLine * 16 + n + memoryOffset))
				{
					SetTextColor(hdc, RGB(255, 0, 0));		// Red indicates editing
					switch (EditState_)
					{
					case EditState::Editing:
						break;
					case EditState::EditChar1:
						v = HexToInt(editKeys[0]) * 16;
						break;
					case EditState::EditChar2:
						v = HexToInt(editKeys[0]) * 16 + HexToInt(editKeys[1]);
						break;
					}
				}
				else
				{
					SetTextColor(hdc, RGB(0, 0, 0));
				}
				s = ToHexString(v, 2, true);
				SetRect(&rc, rect.left + x + (n * 18), y, rect.left + x + (n * 18) + 20, y + h);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			// ASCII Line
			x = rect.right - nCol2;
			{
				SetRect(&rc, x + 5, y, rect.right - 5, y + h);
				DrawText(hdc, ascii.c_str(), ascii.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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

	void LocateMemory()
	{
		TCHAR buffer[100];
		memset(buffer, 0, sizeof(buffer));
		SendDlgItemMessage(hWndMemory, IDC_EDIT_FIND_MEM, WM_GETTEXT, sizeof(buffer), (LPARAM)(LPCSTR)buffer);

		unsigned int addrInt;
		std::stringstream ss;
		ss << buffer;
		ss >> std::hex >> addrInt;

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWndVScrollBar, SB_CTL, &si);

		si.nPos = roundDn(addrInt, 16);
		si.fMask = SIF_POS;
		SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
		GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
		memoryOffset = si.nPos;
		InvalidateRect(hWndMemory, &BackBuffer_.Rect, FALSE);
		
	}

	void WriteMemory(unsigned short addr, unsigned char value)
	{
		EmuState.Debugger.QueueWrite(addr,value);
	}

	unsigned char ReadMemory(unsigned short addr)
	{
		VCC::SectionLocker lock(Section_);
		unsigned char value = RamBuffer_[addr];
		return value;
	}

	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		static int c = 0;

		if (nCode < 0)  // do not process message 
			return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);

		BOOL upFlag = (HIWORD(lParam) & KF_UP) == KF_UP;

		// Only process Key Down events
		if (!upFlag)
		{
			// Valid keypress - hex digits only: a-f, 0-9, and return
			if (wParam == VK_RETURN || wParam == VK_TAB)
			{
				HandleEdit(VK_RETURN);
				wParam = 0;
			}
			if ((wParam >= 0x30 && wParam <= 0x39) ||
				(wParam >= 0x41 && wParam <= 0x46))
			{
				HandleEdit(wParam);
				wParam = 0;
			}
			if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
			{
				HandleEdit(wParam - 0x30);
				wParam = 0;
			}
		}

		return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);
	}

	void CaptureKeyboardEvents()
	{
		if (hHookKeyboard == NULL)
		{
			hHookKeyboard = SetWindowsHookEx(
				WH_KEYBOARD,
				KeyboardProc,
				(HINSTANCE)NULL, GetCurrentThreadId());
		}
	}

	void ReleaseKeyboard()
	{
		if (hHookKeyboard != NULL)
		{
			UnhookWindowsHookEx(hHookKeyboard);
			hHookKeyboard = NULL;
		}
	}

	void StartEdit(int xPos, int yPos)
	{
		int hLine = 18;
		int xStart = 66;
		int yStart = 20;
		int leftDeadArea = xStart + (8 * 18);
		int rightDeadArea = leftDeadArea + 15;
		int maxXPos = rightDeadArea + (8 * 18);
		if (yPos < yStart || xPos < xStart || (xPos >= leftDeadArea && xPos <= rightDeadArea) || xPos > maxXPos)
		{
			EditState_ = EditState::None;
			ReleaseKeyboard();
			return;
		}
		int addrLine = (yPos - yStart) / hLine;
		int offset = (xPos < leftDeadArea) ? (xPos - xStart) / 18 : ((xPos - rightDeadArea) / 18) + 8;
		auto addr = addrLine * 16 + memoryOffset + offset;
		editAddress = static_cast<unsigned short>(addr);
		EditState_ = EditState::Editing;
		CaptureKeyboardEvents();
	}

	void HandleEdit(WPARAM key)
	{
		if (EditState_ == EditState::None)
		{
			ReleaseKeyboard();
			return;
		}
		if (key == VK_RETURN)
		{
			// Commit current edit to memory.
			unsigned char editValue = HexToInt(editKeys[0]) * 16 + HexToInt(editKeys[1]);
			WriteMemory(editAddress, editValue);

			// Remain in edit mode
			EditState_ = EditState::Editing;

			// Address to next memory location.
			editAddress++;

			// Wrap around if we reach the top of memory 
			if (editAddress > 0xFFFF)
			{
				editAddress = 0;
				EditState_ = EditState::None;
				ReleaseKeyboard();
				return;
			}

			// Load up next byte for editing.
			unsigned char currentValue = ReadMemory(editAddress);
			const std::string s(ToHexString(currentValue,1,true));
			editKeys[0] = s[0];
			editKeys[1] = s[1];

			return;
		}
		// Not hex keys - ending editing 
		if (key < 0x30 || key > 0x46)
		{
			ReleaseKeyboard();
			EditState_ = EditState::None;
			return;
		}
		// Keep capturing two keys until return is pressed
		if (EditState_ == EditState::Editing)
		{
			editKeys[0] = (unsigned char)key;
			editKeys[1] = '0';
			EditState_ = EditState::EditChar1;
			return;
		}
		if (EditState_ == EditState::EditChar1)
		{
			editKeys[1] = (unsigned char)key;
			EditState_ = EditState::EditChar2;
			return;
		}
		if (EditState_ == EditState::EditChar2)
		{
			editKeys[0] = (unsigned char)key;
			editKeys[1] = '0';
			EditState_ = EditState::EditChar1;
			return;
		}
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
		}

		return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
	}

	INT_PTR CALLBACK MemoryMapDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

			BackBuffer_ = AttachBackBuffer(hDlg, -20, -39);

			SetTimer(hDlg, IDT_MEM_TIMER, 64, (TIMERPROC)NULL);

			EmuState.Debugger.RegisterClient(hDlg, std::make_unique<MemoryMapDebugClient>());

			break;
		}

		case WM_LBUTTONDBLCLK:
		{
			int xPos = ((int)(short)LOWORD(lParam));
			int yPos = ((int)(short)HIWORD(lParam));
			StartEdit(xPos, yPos);
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
			InvalidateRect(hDlg, &BackBuffer_.Rect, FALSE);
			return 0;
		}

		case WM_VSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);

			switch ((int)LOWORD(wParam))
			{
			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;
			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
				si.nPos = si.nTrackPos;
				break;
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			case SB_TOP:
				si.nPos = 0;
				break;
			case SB_BOTTOM:
				si.nPos = 0xFFFF;
				break;
			case SB_ENDSCROLL:
				break;
			case SB_LINEUP:
				si.nPos -= 32 * 8;
				break;
			case SB_LINEDOWN:
				si.nPos += 32 * 8;
				break;
			}
			si.nPos = roundUp(si.nPos, 32);
			si.fMask = SIF_POS;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			memoryOffset = si.nPos;
			InvalidateRect(hDlg, &BackBuffer_.Rect, FALSE);
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			DrawMemoryState(BackBuffer_.DeviceContext, &BackBuffer_.Rect);
			BitBlt(hdc, 0, 0, BackBuffer_.Width, BackBuffer_.Height, BackBuffer_.DeviceContext, 0, 0, SRCCOPY);

			EndPaint(hDlg, &ps);
			break;
		}

		case WM_TIMER:

			switch (wParam)
			{
			case IDT_MEM_TIMER:
				InvalidateRect(hDlg, &BackBuffer_.Rect, FALSE);
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
				ReleaseKeyboard();
				KillTimer(hDlg, IDT_MEM_TIMER);
				DeleteDC(BackBuffer_.DeviceContext);
				DestroyWindow(hDlg);
				HWND hCtl = GetDlgItem(hDlg, IDC_EDIT_FIND_MEM);
				(WNDPROC)SetWindowLongPtr(hCtl, GWLP_WNDPROC, (LONG_PTR)oldEditProc);
				MemoryMapWindow = NULL;
				EmuState.Debugger.RemoveClient(hDlg);
				break;
			}

			break;
		}
		return FALSE;
	}

} } } }


void VCC::Debugger::UI::OpenMemoryMapWindow(HINSTANCE instance, HWND parent)
{
	if (MemoryMapWindow == NULL)
	{
		MemoryMapWindow = CreateDialog(
			instance,
			MAKEINTRESOURCE(IDD_MEMORY_MAP),
			parent,
			MemoryMapDlgProc);
		ShowWindow(MemoryMapWindow, SW_SHOWNORMAL);
	}

	SetFocus(MemoryMapWindow);
}


