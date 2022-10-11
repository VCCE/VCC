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

	Breakpoint Display - Part of the Debugger package for VCC
	Author: Mike Rojas
*/

#include "defines.h"
#include "DebuggerUtils.h"
#include "resource.h"
#include <commdlg.h>
#include <map>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <Richedit.h>
#include <codecvt>
#include <locale>


extern SystemState EmuState;

namespace Breakpoints
{
	HWND hWndBreakpoints;
	WNDPROC oldEditProc;

	bool BrowseDialogOpen = false;
	unsigned short PriorCPUHaltAddress = 0;

	struct Breakpoint
	{
		int id;
		int addr;
		int line;
		bool enabled;
	};

	std::map<int, Breakpoint> mapBreakpoints;
	std::map<int, int> mapLineAddr;
	std::map<int, int> mapAddrLine;

	std::vector<std::string> split(const std::string& input, const std::string& rgx) 
	{
		// passing -1 as the submatch index parameter performs splitting
		std::regex re(rgx);
		std::sregex_token_iterator
			first{ input.begin(), input.end(), re, -1 },
			last;
		return { first, last };
	}

	void ResizeWindow(int width, int height)
	{
		HWND hCtl = GetDlgItem(hWndBreakpoints, IDC_SOURCE_LISTING);
		MoveWindow(hCtl, 14, 255, width - 30, height - 298, TRUE);

		hCtl = GetDlgItem(hWndBreakpoints, IDCLOSE);
		SetWindowPos(hCtl, NULL, 10, height - 34, 0, 0, SWP_NOSIZE);
		hCtl = GetDlgItem(hWndBreakpoints, IDC_BTN_FIND_SOURCE);
		SetWindowPos(hCtl, NULL, width - 65, height - 34, 0, 0, SWP_NOSIZE);
		hCtl = GetDlgItem(hWndBreakpoints, IDC_EDIT_FIND_SOURCE);
		SetWindowPos(hCtl, NULL, width - 172, height - 34, 0, 0, SWP_NOSIZE);
	}

	bool LoadSource(char* source)
	{
		SendDlgItemMessage(hWndBreakpoints, IDC_EDIT_SOURCE, WM_SETTEXT, strlen(source), (LPARAM)(LPCSTR)source);

		int nLines = 0;
		std::string loaded(std::to_string(nLines) + " lines loaded");
		SendDlgItemMessage(hWndBreakpoints, IDC_LINES_LOADED, WM_SETTEXT, loaded.size(), (LPARAM)ToLPCSTR(loaded));

		std::ifstream file(source);
		if (!file.is_open())
		{
			return false;
		}

		std::string lines;

		for (std::string line; getline(file, line); )
		{
			auto addr = line.length() >= 4 ? line.substr(0, 4): std::string(4, ' ');
			auto opcodes = line.length() >= 20 ? line.substr(5, 16) : std::string(16, ' ');
			auto sourceline = line.length() >= 45 ? line.substr(22, 25) : std::string(25, ' ');
			auto code = line.length() >= 60 ? line.substr(56) : "";

			if (addr != std::string(4, ' '))
			{
				unsigned int addrInt;
				std::stringstream ss;
				ss << addr;
				ss >> std::hex >> addrInt;
				mapLineAddr[nLines] = addrInt;		// Map address to listing line number.
				mapAddrLine[addrInt] = nLines;		// Map listing line number to address.
			}
			lines += "    | ";
			lines += addr.c_str();
			lines += " | ";
			lines += opcodes.c_str();
			lines += " | ";
			lines += code.c_str();
			lines += "\n";
			nLines++;
		}

		loaded = std::to_string(nLines) + " lines loaded";
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_SETSEL, WPARAM(0), LPARAM(-1));
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_REPLACESEL, WPARAM(TRUE), (LPARAM)ToLPCSTR(lines));
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_LINESCROLL, 0, (LPARAM)-nLines);
		SendDlgItemMessage(hWndBreakpoints, IDC_LINES_LOADED, WM_SETTEXT, loaded.size(), (LPARAM)ToLPCSTR(loaded));
		return true;
	}

	bool SelectSourceListing()
	{
		if (BrowseDialogOpen)
		{
			return false;
		}
		BrowseDialogOpen = false;

		OPENFILENAME ofn;
		char SourceFileName[MAX_PATH] = "";
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.Flags = OFN_HIDEREADONLY;
		ofn.hInstance = GetModuleHandle(0);
		ofn.lpstrDefExt = "";
		ofn.lpstrFilter = "LWASM Source Listings (*.lst)\0*.lst\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
		ofn.nFilterIndex = 0;							// current filter index
		ofn.lpstrFile = SourceFileName;					// contains full path and filename on return
		ofn.nMaxFile = MAX_PATH;						// sizeof lpstrFile
		ofn.lpstrFileTitle = NULL;						// filename and extension only
		ofn.nMaxFileTitle = MAX_PATH;					// sizeof lpstrFileTitle
		ofn.lpstrInitialDir = NULL;					// initial directory
		ofn.lpstrTitle = "Load LWASM Source Listing";	// title bar string

		int RetVal = GetOpenFileName(&ofn);
		if (RetVal)
		{
			if (LoadSource(SourceFileName) == 0)
			{
				MessageBox(NULL, "Can't open source listing", "Error", 0);
				return false;
			}
		}

		BrowseDialogOpen = false;
		return true;
	}

	void SetupListingControl()
	{
		CHARFORMAT fmt;
		fmt.cbSize = sizeof(CHARFORMAT);
		LRESULT lResult = SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&fmt);
		strcpy(fmt.szFaceName, "Courier New");
		fmt.yHeight = 10 * 20;
		fmt.dwEffects = CFE_BOLD;
		fmt.dwMask = CFM_BOLD | CFM_FACE | CFM_SIZE | CFM_COLOR;
		fmt.crTextColor = RGB(255, 255, 255);
		lResult = SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_SETCHARFORMAT, SCF_ASSOCIATEFONT, (LPARAM)&fmt);

		HWND hCtl = GetDlgItem(hWndBreakpoints, IDC_SOURCE_LISTING);

		DWORD msk = ENM_KEYEVENTS | ENM_MOUSEEVENTS; // | ENM_SCROLL | ENM_SCROLLEVENTS;
		SendMessage(hCtl, EM_SETEVENTMASK, WPARAM(0), (LPARAM)msk);
		SendMessage(hCtl, EM_SETBKGNDCOLOR, WPARAM(0), RGB(0, 0, 0));

		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_DEL_BREAKPOINT), false);
		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_ON), false);
		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_OFF), false);
	}

	int CurrentSourceLine()
	{
		HWND hCtl = GetDlgItem(hWndBreakpoints, IDC_SOURCE_LISTING);
		int currentCharStart = SendMessage(hCtl, EM_LINEINDEX, -1, 0L);
		return SendMessage(hCtl, EM_LINEFROMCHAR, currentCharStart, 0);
	}

	void SelectSourceText(int line, int count, int pos = 0)
	{
		HWND hCtl = GetDlgItem(hWndBreakpoints, IDC_SOURCE_LISTING);
		int currentCharStart = SendMessage(hCtl, EM_LINEINDEX, line, 0L);
		int currentCharEnd = currentCharStart + count;
		if (count == -1)
		{
			currentCharEnd = SendMessage(hCtl, EM_LINEINDEX, line + 1, 0L);
		}
		SendMessage(hCtl, EM_SETSEL, currentCharStart + pos, currentCharEnd);
	}

	std::string GetSelectedSourceText()
	{
		WCHAR buf[1000];
		memset(buf, 0, sizeof(buf));
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_GETSELTEXT, 0, (LPARAM)&buf);

		//setup converter
		using convert_type = std::codecvt_utf8<wchar_t>;
		return std::wstring_convert<convert_type, wchar_t>().to_bytes( buf );
	}

	void SetLineColor(int line, int rgb)
	{
		SelectSourceText(line, -1);
		CHARFORMAT fmt;
		fmt.cbSize = sizeof(CHARFORMAT);
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
		fmt.dwMask = CFM_COLOR;
		fmt.crTextColor = rgb;
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
	}

	void UpdateBreakpointUI(Breakpoint bp)
	{
		HWND hListCtl = GetDlgItem(hWndBreakpoints, IDC_LIST_BREAKPOINTS);
		const std::string enabled = bp.enabled ? "On" : "Off";
		std::string s = "Breakpoint #" + std::to_string(bp.id) + "\t" + enabled + "\taddr = " + ToHexString(bp.addr, 0, false);
		int pos = SendMessage(hListCtl, LB_ADDSTRING, 0, (LPARAM)ToLPCSTR(s));
		SendMessage(hListCtl, LB_SETITEMDATA, pos, (LPARAM)bp.id);
		SendMessage(hListCtl, LB_SETCURSEL, pos, 0);

		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_DEL_BREAKPOINT), true);
		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_ON), true);
		EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_OFF), true);
		
		SelectSourceText(bp.line, 4);
		s = (bp.enabled ? "BK" : "--") + ToDecimalString(bp.id, 2, false);
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_REPLACESEL, WPARAM(FALSE), (LPARAM)ToLPCSTR(s));
		SetLineColor(bp.line, RGB(255, 100, 100));
	}

	void UpdateCPUState()
	{
		EnterCriticalSection(&EmuState.WatchCriticalSection);

		EmuState.CPUNumBreakpoints = mapBreakpoints.size();
		if (EmuState.CPUBreakpoints != NULL)
		{
			free(EmuState.CPUBreakpoints);
		}
		EmuState.CPUBreakpoints = (unsigned short*)malloc(sizeof(unsigned short) * EmuState.CPUNumBreakpoints);

		int n = 0;
		std::map<int, Breakpoint>::iterator it = mapBreakpoints.begin();
		while (it != mapBreakpoints.end())
		{
			Breakpoint bp = it->second;
			if (bp.enabled)
			{
				EmuState.CPUBreakpoints[n++] = bp.addr;
			}
			it++;
		}
		EmuState.CPUControl = 'B';

		LeaveCriticalSection(&EmuState.WatchCriticalSection);
	}

	int AddBreakpoint(int line)
	{
		// Find first available id
		int n = 1;
		while (mapBreakpoints.find(n) != mapBreakpoints.end()) n++;
		Breakpoint bp;
		bp.id = n;
		bp.enabled = true;
		bp.line = line;
		bp.addr = mapLineAddr[line];
		mapBreakpoints[n] = bp;

		UpdateCPUState();

		UpdateBreakpointUI(bp);

		return n;
	}

	void EnableBreakpoint(int id, bool enable)
	{
		HWND hListCtl = GetDlgItem(hWndBreakpoints, IDC_LIST_BREAKPOINTS);

		int cnt = SendMessage(hListCtl, LB_GETCOUNT, 0, 0);
		for (int n = 0; n < cnt; n++)
		{
			int itemId = SendMessage(hListCtl, LB_GETITEMDATA, n, 0);
			if (itemId == id)
			{
				SendMessage(hListCtl, LB_DELETESTRING, n, 0);

				Breakpoint bp = mapBreakpoints[id];
				bp.enabled = enable;
				mapBreakpoints[id] = bp;

				UpdateCPUState();

				UpdateBreakpointUI(bp);
				break;
			}
		}
	}

	void RemoveBreakpoint(int id)
	{
		Breakpoint bp = mapBreakpoints[id];
		mapBreakpoints.erase(id);

		UpdateCPUState();

		HWND hListCtl = GetDlgItem(hWndBreakpoints, IDC_LIST_BREAKPOINTS);
		int cnt = SendMessage(hListCtl, LB_GETCOUNT, 0, 0);
		for (int n = 0; n < cnt; n++)
		{
			int itemId = SendMessage(hListCtl, LB_GETITEMDATA, n, 0);
			if (itemId == id)
			{
				SendMessage(hListCtl, LB_DELETESTRING, n, 0);
				SendMessage(hListCtl, LB_SETCURSEL, -1, 0);

				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_DEL_BREAKPOINT), false);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_ON), false);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_OFF), false);

				SelectSourceText(bp.line, 4);
				const std::string s("    ");
				SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_REPLACESEL, WPARAM(FALSE), (LPARAM)ToLPCSTR(s));
				SetLineColor(bp.line, RGB(255, 255, 255));
				break;
			}
		}
	}

	void NewBreakPoint()
	{
		int currentLine = CurrentSourceLine();

		// Can we set a break point on this line?
		if (mapLineAddr[currentLine] == 0)
		{
			return;
		}
		SelectSourceText(currentLine, 4);
		const std::string start = GetSelectedSourceText();
		if (start.substr(0, 4) == "    ")
		{
			AddBreakpoint(currentLine);
		}
	}

	void ListBoxSelection(int action)
	{
		HWND hListCtl = GetDlgItem(hWndBreakpoints, IDC_LIST_BREAKPOINTS);
		if (action == LBN_SELCHANGE)
		{
			int selected = SendMessage(hListCtl, LB_GETCURSEL, 0, 0);
			if (selected != LB_ERR)
			{
				int itemId = SendMessage(hListCtl, LB_GETITEMDATA, selected, 0);
				Breakpoint bp = mapBreakpoints[itemId];

				int topLine = SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_GETFIRSTVISIBLELINE, 0, 0);
				int scroll = bp.line - topLine;
				SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_LINESCROLL, 0, (LPARAM)scroll);

				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_DEL_BREAKPOINT), true);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_ON), true);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_OFF), true);
			}
			else
			{
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_DEL_BREAKPOINT), false);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_ON), false);
				EnableWindow(GetDlgItem(hWndBreakpoints, IDC_BTN_BREAKPOINT_OFF), false);
			}
		}
	}

	void ListBoxAction(int action)
	{
		HWND hListCtl = GetDlgItem(hWndBreakpoints, IDC_LIST_BREAKPOINTS);
		int selected = SendMessage(hListCtl, LB_GETCURSEL, 0, 0);
		if (selected != LB_ERR)
		{
			int itemId = SendMessage(hListCtl, LB_GETITEMDATA, selected, 0);
			Breakpoint bp = mapBreakpoints[itemId];
			switch (action)
			{
			case IDC_BTN_DEL_BREAKPOINT:
				RemoveBreakpoint(bp.id);
				break;
			case IDC_BTN_BREAKPOINT_ON:
				EnableBreakpoint(bp.id, true);
				break;
			case IDC_BTN_BREAKPOINT_OFF:
				EnableBreakpoint(bp.id, false);
				break;
			}
		}
	}

	void ToggleBreakpoint(int line)
	{
		// Can we set a break point on this line?
		if (mapLineAddr[line] == 0)
		{
			return;
		}

		SelectSourceText(line, 4);
		const std::string start(GetSelectedSourceText());
		if (start.substr(0, 2) == "BK" || start.substr(0, 2) == "--")
		{
			const std::string nn = start.substr(2, 2);
			int id = atoi(nn.c_str());
			RemoveBreakpoint(id);
		}
		else
		{
			AddBreakpoint(line);
		}
	}

	void FindSourceLine()
	{
		TCHAR buffer[100];
		memset(buffer, 0, sizeof(buffer));
		SendDlgItemMessage(hWndBreakpoints, IDC_EDIT_FIND_SOURCE, WM_GETTEXT, sizeof(buffer), (LPARAM)(LPCSTR)buffer);

		size_t newsize = strlen(buffer) + 1;
		wchar_t* wcstring = new wchar_t[newsize];

		// Convert char* string to a wchar_t* string.
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, buffer, _TRUNCATE);

		HWND hCtl = GetDlgItem(hWndBreakpoints, IDC_SOURCE_LISTING);

		DWORD cPos = 0;
		SendMessage(hCtl, EM_GETSEL, (WPARAM)&cPos, 0);

		cPos++;

		FINDTEXTEX ft;
		ft.lpstrText = (LPSTR)wcstring;
		ft.chrg.cpMin = cPos;
		ft.chrg.cpMax = -1;
		int pos = SendMessage(hCtl, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
		if (pos == -1)
		{
			SelectSourceText(0, -1);
			return;
		}

		int line = SendMessage(hCtl, EM_LINEFROMCHAR, pos, 0);
		int topLine = SendMessage(hCtl, EM_GETFIRSTVISIBLELINE, 0, 0);
		int scroll = line - topLine;
		SendMessage(hCtl, EM_LINESCROLL, 0, (LPARAM)scroll);
		SendMessage(hCtl, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
	}

	int HandleKeyDown(DWORD key)
	{
		int currentLine = CurrentSourceLine();
		if (key == 'B' || key == 'b')
		{
			ToggleBreakpoint(currentLine);
		}
		if (key == 'E' || key == 'e')
		{
			SelectSourceText(currentLine, 4);
			const std::string start = GetSelectedSourceText();
			if (start.substr(0, 2) == "BK")
			{
				const std::string nn = start.substr(2, 2);
				int id = atoi(nn.c_str());
				EnableBreakpoint(id, false);
			}
			if (start.substr(0, 2) == "--")
			{
				const std::string nn = start.substr(2, 2);
				int id = atoi(nn.c_str());
				EnableBreakpoint(id, true);
			}
		}
		return 0;
	}

	void LocateBreakpoint(unsigned short addr)
	{
		// Do we have a line number to show?
		if (mapAddrLine[addr] == 0)
		{
			return;
		}

		int line = mapAddrLine[addr];
		int topLine = SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_GETFIRSTVISIBLELINE, 0, 0);
		int scroll = line - topLine;
		SendDlgItemMessage(hWndBreakpoints, IDC_SOURCE_LISTING, EM_LINESCROLL, 0, (LPARAM)scroll);
		SelectSourceText(line, -1);
	}

	void CheckCPUState()
	{
		char CPUState;
		unsigned short PC;
		EnterCriticalSection(&EmuState.WatchCriticalSection);
		PC = (EmuState.WatchProcState[12] << 8) + EmuState.WatchProcState[13];
		CPUState = EmuState.WatchProcState[19];
		LeaveCriticalSection(&EmuState.WatchCriticalSection);
		if (CPUState == 'H' && PriorCPUHaltAddress != PC)
		{
			LocateBreakpoint(PC);
			PriorCPUHaltAddress = PC;
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
				FindSourceLine();
				return 0;
			}
		default:
			return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
		}
		return 0;
	}

	LRESULT CALLBACK Breakpoints(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			hWndBreakpoints = hDlg;
			SetupListingControl();

			HWND hCtl = GetDlgItem(hDlg, IDC_EDIT_FIND_SOURCE);
			oldEditProc = (WNDPROC)SetWindowLongPtr(hCtl, GWLP_WNDPROC, (LONG_PTR)subEditProc);

			SetTimer(hDlg, IDT_BRKP_TIMER, 250, (TIMERPROC)NULL);

			break;
		}

		case WM_NOTIFY:
			switch (((NMHDR*)lParam)->code)
			{
			case EN_MSGFILTER:
				switch (((MSGFILTER*)lParam)->msg) 
				{
				case WM_KEYDOWN:
					return HandleKeyDown(((MSGFILTER*)lParam)->wParam);
				case WM_KEYUP:
					return TRUE;
				}
				return 0;
			}
			break;

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
			lpMMI->ptMinTrackSize.x = 525;
			lpMMI->ptMinTrackSize.y = 400;
			break;
		}

		case WM_SIZE:
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			ResizeWindow(width, height);
			return 0;
		}

		case WM_TIMER:

			switch (wParam)
			{
			case IDT_BRKP_TIMER:
				CheckCPUState();
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_LIST_BREAKPOINTS:
				ListBoxSelection(HIWORD(wParam));
				break;
			case IDC_BTN_SOURCE_BROWSE:
				SelectSourceListing();
				break;
			case IDC_BTN_NEW_BREAKPOINT:
				NewBreakPoint();
				break;
			case IDC_BTN_DEL_BREAKPOINT:
			case IDC_BTN_BREAKPOINT_ON:
			case IDC_BTN_BREAKPOINT_OFF:
				ListBoxAction(LOWORD(wParam));
				break;
			case IDC_BTN_FIND_SOURCE:
				FindSourceLine();
				break;
			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_BRKP_TIMER);
				DestroyWindow(hDlg);
				EmuState.BreakpointWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}
}
