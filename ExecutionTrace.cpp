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

	Execution Trace Display - Part of the Debugger package for VCC
	Author: Mike Rojas
*/
#include "ExecutionTrace.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "resource.h"
#include <stdexcept>
#include <Richedit.h>
#include <sstream>
#include <array>
#include <windowsx.h>

namespace VCC { namespace Debugger { namespace UI { namespace 
{

	HWND ExecutionTraceWindow = NULL;
	HWND hWndExecutionTrace;
	HWND hWndVScrollBar;
	HWND hWndHScrollBar;
	HWND hWndTrace;

	HHOOK hHookKeyboard = NULL;
	void CaptureKeyboardEvents();
	void ReleaseKeyboard();

	int ScrollBarTop = 140;
	int ScrollBarWidth = 19;

	Debugger::tracebuffer_type currentTrace;

	enum class TraceStatus
	{
		Empty,
		Collecting,
		LoadPage,
		Loaded
	};

	TraceStatus status = TraceStatus::Empty;
	long traceOffset = 0;
	long tracePage = 50;
	long traceCursor = 0;
	long horizontalOffset = 0;
	long horizontalWidth = 965;
	bool showHScrollBar = true;

	BackBufferInfo	BackBuffer_;

	std::string ToCCString(const unsigned char CC)
	{
		std::ostringstream fmt;

		fmt << ((CC & 0b10000000) ? "E" : ".")
			<< ((CC & 0b01000000) ? "F" : ".")
			<< ((CC & 0b00100000) ? "H" : ".")
			<< ((CC & 0b00010000) ? "I" : ".")
			<< ((CC & 0b00001000) ? "N" : ".")
			<< ((CC & 0b00000100) ? "Z" : ".")
			<< ((CC & 0b00000010) ? "V" : ".")
			<< ((CC & 0b00000001) ? "C" : ".");

		return fmt.str();
	}

	std::string ToStateChangeString(const CPUTrace trace, bool showPC = true)
	{
		std::ostringstream fmt;

		if ((trace.startState.A != trace.endState.A) && (trace.startState.B != trace.endState.B))
		{
			fmt << " D=" << ToHexString(trace.endState.A, 2) << ToHexString(trace.endState.B, 2);
		}
		else if (trace.startState.A != trace.endState.A)
		{
			fmt << " A=" << ToHexString(trace.endState.A, 2);
		}
		else if (trace.startState.B != trace.endState.B)
		{
			fmt << " B=" << ToHexString(trace.endState.B, 2);
		}
		if (trace.endState.IsNative6309)
		{
			if ((trace.startState.E != trace.endState.E) && (trace.startState.F != trace.endState.F))
			{
				fmt << " W=" << ToHexString(trace.endState.E, 2) << ToHexString(trace.endState.F, 2);
			}
			else if (trace.startState.E != trace.endState.E)
			{
				fmt << " E=" << ToHexString(trace.endState.E, 2);
			}
			else if (trace.startState.F != trace.endState.F)
			{
				fmt << " F=" << ToHexString(trace.endState.F, 2);
			}
		}
		if (trace.startState.X != trace.endState.X)
		{
			fmt << " X=" << ToHexString(trace.endState.X, 4);
		}
		if (trace.startState.Y != trace.endState.Y)
		{
			fmt << " Y=" << ToHexString(trace.endState.Y, 4);
		}
		if (trace.startState.U != trace.endState.U)
		{
			fmt << " U=" << ToHexString(trace.endState.U, 4);
		}
		if (trace.startState.S != trace.endState.S)
		{
			fmt << " S=" << ToHexString(trace.endState.S, 4);
		}
		if (trace.endState.IsNative6309)
		{
			if (trace.startState.V != trace.endState.V)
			{
				fmt << " V=" << ToHexString(trace.endState.V, 4);
			}
		}
		if (trace.startState.DP != trace.endState.DP)
		{
			fmt << " DP=" << ToHexString(trace.endState.DP, 2);
		}
		if (trace.endState.IsNative6309)
		{
			if (trace.startState.MD != trace.endState.MD)
			{
				fmt << " MD=" << ToHexString(trace.endState.MD, 2);
			}
		}
		if (trace.startState.CC != trace.endState.CC)
		{
			fmt << " CC=" << ToHexString(trace.endState.CC, 2);
		}
		if (showPC)
		{
			if (trace.startState.PC != trace.endState.PC)
			{
				fmt << " PC=" << ToHexString(trace.endState.PC, 4);
			}
		}

		// Skip over first space.
		return (fmt.str().size() > 0) ? fmt.str().substr(1) : fmt.str();
	}

	void DrawBorder(HDC hdc, LPRECT clientRect)
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

		DeleteObject(pen);
	}

	void DrawSamplingInProgress(HDC hdc, LPRECT clientRect)
	{
		status = TraceStatus::Collecting;

		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
		SetWindowText(hCtl, "Trace is running");

		RECT rect = *clientRect;

		HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));
		SelectObject(hdc, hFont);

		long samples = EmuState.Debugger.GetTraceSamples();

		std::stringstream ss;
		ss << samples << " Samples Collected";
		DrawText(hdc, (LPCSTR)ss.str().c_str(), ss.str().size(), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		DeleteObject(hFont);
	}

	void DrawNoSamplesCollected(HDC hdc, LPRECT clientRect)
	{
		status = TraceStatus::Empty;

		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
		SetWindowText(hCtl, "Trace is stopped");

		RECT rect;
		GetClientRect(hWndTrace, &rect);

		HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));
		SelectObject(hdc, hFont);

		std::string s = "Press Enable Trace to start collection";
		DrawText(hdc, (LPCSTR)s.c_str(), s.size(), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		DeleteObject(hFont);
	}

	void DrawSamples(HDC hdc, LPRECT clientRect, long samples)
	{
		// Nothing collected yet?
		if (samples == 0)
		{
			DrawNoSamplesCollected(hdc, clientRect);
			return;
		}

		// Collection finished?
		if (status == TraceStatus::Collecting || status == TraceStatus::Loaded)
		{
			status = TraceStatus::LoadPage;

			std::stringstream ss;
			ss << samples << " collected";

			HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
			SetWindowText(hCtl, ss.str().c_str());

			// Adjust Vertical Scrollbar based on samples collected.
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			si.nMax = samples;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
		}

		// Draw the collection.
		RECT rect = *clientRect;

		HFONT hFont = CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));
		SelectObject(hdc, hFont);

		HPEN pen = (HPEN)CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
		SelectObject(hdc, pen);

		// Load page?
		if (status == TraceStatus::LoadPage)
		{
			EmuState.Debugger.GetTraceResult(currentTrace, traceOffset, tracePage);
			status = TraceStatus::Loaded;
		}

		// Draw our header line.
		SetTextColor(hdc, RGB(138, 27, 255));
		std::vector<std::string> headers = { "Sample", "Cycles", "Address", "Memory", "Instruction", "CPU Cycles", "State Changes", "CC", "D", "X", "Y", "U", "S", "DP" };
		std::vector<int> columns = { 60, 60, 60, 90, 120, 80, 160, 60, 40, 30, 30, 30, 30, 30 };

		bool Is6309 = EmuState.CpuType == 1;

		// Running a 6309 CPU?
		if (Is6309)
		{
			std::vector<std::string>::iterator itHeader;
			std::vector<int>::iterator itColumn;

			itHeader = headers.begin();
			itHeader = headers.insert(itHeader + 9, "W");
			itColumn = columns.begin();
			itColumn = columns.insert(itColumn + 9, 30);

			itHeader = headers.end();
			itHeader = headers.insert(itHeader, "MD");
			itColumn = columns.end();
			itColumn = columns.insert(itColumn, 30);
		}

		int x = 10;
		int col = 0;
		for (auto& h : headers)
		{
			RECT rc;
			int w = columns[col++];
			SetRect(&rc, rect.left + x, rect.top, rect.left + x + w, rect.top + 20);

			// Draw the border.
			MoveToEx(hdc, rc.left, rc.top, NULL);
			LineTo(hdc, rc.right - 1, rc.top);
			LineTo(hdc, rc.right - 1, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.top);

			DrawText(hdc, h.c_str(), h.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			x += w + 5;
		}

		// Trace Mark Highlight Colors
		HBRUSH hBrushMark1 = CreateSolidBrush(RGB(20, 200, 20));
		HBRUSH hBrushMark2 = CreateSolidBrush(RGB(200, 20, 20));
		HBRUSH hBrushCursor = CreateSolidBrush(RGB(230, 230, 230));

		Debugger::tracebuffer_type marks;
		EmuState.Debugger.GetTraceMarkSamples(marks);

		long traceMark1 = -1;
		long traceMark2 = -1;

		// Calculate trace timing.
		if (marks.size() == 2)
		{
			traceMark1 = marks[0].cycleTime;
			traceMark2 = marks[1].cycleTime;
			long cycles = abs(traceMark2 - traceMark1);
			double hz = EmuState.CPUCurrentSpeed * 1000000.0;
			double elapsed = cycles / hz * 1000000.0;
			std::stringstream ss;
			ss << "T1 = " << traceMark1 << ", ";
			ss << "T2 = " << traceMark2 << ", ";
			ss << "Elapsed = " << cycles << " cycles, ";
			ss << elapsed << " us at " << hz / 1000 << "kHz";
			HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_TIMING);
			SetWindowText(hCtl, ss.str().c_str());
		}
		else if (marks.size() == 1)
		{
			traceMark1 = marks[0].cycleTime;
			std::stringstream ss;
			ss << "T1 = " << traceMark1 << " set, right click to set T2";
			HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_TIMING);
			SetWindowText(hCtl, ss.str().c_str());
		}
		else
		{
			std::string s = "Press 1 or 2 to set time marks on trace.";
			HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_TIMING);
			SetWindowText(hCtl, s.c_str());
		}

		// Draw our current page of samples
		SetTextColor(hdc, RGB(0, 0, 0));
		int y = rect.top + 20;
		int lines = min((long)currentTrace.size(), tracePage);
		for (int n = 0; n < lines; n++)
		{
			std::string s;

			RECT rc;
			int x = 10;

			long sampleNum = n + traceOffset;

			if (sampleNum == traceCursor)
			{
				SetRect(&rc, rect.left, y, rect.right, y + 14);
				FillRect(hdc, &rc, hBrushCursor);
			}

			if (currentTrace[n].cycleTime == traceMark1)
			{
				SetRect(&rc, rect.left, y, rect.right, y + 14);
				FillRect(hdc, &rc, hBrushMark1);
			}

			if (currentTrace[n].cycleTime == traceMark2)
			{
				SetRect(&rc, rect.left, y, rect.right, y + 14);
				FillRect(hdc, &rc, hBrushMark2);
			}

			int w = columns[0];
			SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
			s = ToDecimalString(sampleNum + 1, 10, false);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

			if (currentTrace[n].event == TraceEvent::Instruction)
			{
				col = 1;
				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToDecimalString(currentTrace[n].cycleTime, 10, false);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].pc, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToByteString(currentTrace[n].bytes);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = currentTrace[n].instruction + " " + currentTrace[n].operand;
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToDecimalString(currentTrace[n].execCycles, 2, false);
				s += " / ";
				s += ToDecimalString(currentTrace[n].decodeCycles, 2, false);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToStateChangeString(currentTrace[n]);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToCCString(currentTrace[n].startState.CC);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString((currentTrace[n].startState.A << 8) + currentTrace[n].startState.B, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				if (Is6309)
				{
					x += w + 5;
					w = columns[col++];
					SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
					s = ToHexString((currentTrace[n].startState.E << 8) + currentTrace[n].startState.F, 4);
					DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].startState.X, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].startState.Y, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].startState.U, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].startState.S, 4);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[col++];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToHexString(currentTrace[n].startState.DP, 2);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				if (Is6309)
				{
					x += w + 5;
					w = columns[col++];
					SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
					s = ToHexString(currentTrace[n].startState.MD, 2);
					DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
				y += 15;
			}
			else if (currentTrace[n].event == TraceEvent::EmulatorCycle)
			{
				x += w + 10;
				w = 1200;
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				std::stringstream ss;
				ss << "Emulation :: ";
				switch (currentTrace[n].emulationState)
				{
				case 0:
					ss << "  CPU Only         ";
					break;
				case 1:
					ss << "  Timer IRQ        ";
					break;
				case 2:
					ss << "  Audio Sample     ";
					break;
				case 3:
					ss << "  Both, 1st Audio  ";
					break;
				case 4:
					ss << "  Both, 2nd Timer  ";
					break;
				case 5:
					ss << "  Both, 1st Timer  ";
					break;
				case 6:
					ss << "  Both, 2nd Audio  ";
					break;
				case 7:
					ss << "  Both Timer/Audio ";
					break;
				case 10:
					ss << "Cycle Start =======";
					break;
				case 20:
					ss << "Cycle End ---------";
					break;
				}
				ss << " : ";
				ss << "  exec " << currentTrace[n].emulator[0];
				ss << "  timer " << currentTrace[n].emulator[1];
				ss << "  sound " << currentTrace[n].emulator[2];
				ss << "  cycles " << currentTrace[n].emulator[3];
				ss << "  drift " << currentTrace[n].emulator[4];
				ss << "  total " << currentTrace[n].emulator[5];
				DrawText(hdc, ss.str().c_str(), ss.str().size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				y += 15;
			}
			else if (currentTrace[n].event > TraceEvent::ScreenStart && currentTrace[n].event < TraceEvent::ScreenEnd)
			{
				x += w + 5;
				w = columns[1];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToDecimalString(currentTrace[n].cycleTime, 10, false);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[2];
				x += w + 5;
				w = columns[3];
				x += w + 5;

				w = columns[4];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = currentTrace[n].instruction;
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				x += w + 5;
				w = columns[5];
				SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
				s = ToDecimalString(currentTrace[n].execCycles, 2, false);
				s += " / ";
				s += ToDecimalString(currentTrace[n].decodeCycles, 2, false);
				DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				y += 15;
			}
			else if (currentTrace[n].event > TraceEvent::IRQStart && currentTrace[n].event < TraceEvent::IRQEnd)
			{
			x += w + 5;
			w = columns[1];
			SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
			s = ToDecimalString(currentTrace[n].cycleTime, 10, false);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

			x += w + 5;
			w = columns[2];
			x += w + 5;
			w = columns[3];
			x += w + 5;

			w = columns[4];
			SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
			s = currentTrace[n].instruction;
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			x += w + 5;
			w = columns[5];
			SetRect(&rc, rect.left + x, y, rect.left + x + w, y + 15);
			s = ToDecimalString(currentTrace[n].execCycles, 2, false);
			s += " / ";
			s += ToDecimalString(currentTrace[n].decodeCycles, 2, false);
			DrawText(hdc, s.c_str(), s.size(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			y += 15;
			}
		}

		DeleteObject(hBrushCursor);
		DeleteObject(hBrushMark1);
		DeleteObject(hBrushMark2);
		DeleteObject(pen);
		DeleteObject(hFont);
	}

	void DrawExecutionTrace(HDC hdc, LPRECT clientRect)
	{
		// Draw the border.
		DrawBorder(hdc, clientRect);

		// Get tracing status.
		bool tracing = EmuState.Debugger.IsTracing();
		long samples = EmuState.Debugger.GetTraceSamples();

		// Show tracing is in progress.
		if (tracing)
		{
			DrawSamplingInProgress(hdc, clientRect);
			return;
		}

		DrawSamples(hdc, clientRect, samples);
	}

	void ResizeWindow(int width, int height)
	{
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDCLOSE);
		SetWindowPos(hCtl, NULL, 9, height - 33, 0, 0, SWP_NOSIZE);

		hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_TIMING);
		SetWindowPos(hCtl, NULL, 100, height - 28, 0, 0, SWP_NOSIZE);

		SCROLLINFO si;
		RECT Rect;
		GetClientRect(hWndExecutionTrace, &Rect);

		// Hide horizontal scrollbar if not needed.
		showHScrollBar = Rect.right - ScrollBarWidth < horizontalWidth;
		ShowWindow(hWndHScrollBar, showHScrollBar ? SW_SHOW : SW_HIDE);

		int bottomOffset = (showHScrollBar) ? 0 : -ScrollBarWidth;

		if (showHScrollBar)
		{
			SetWindowPos(hWndHScrollBar, NULL, 0, Rect.bottom - 39 - ScrollBarWidth, Rect.right - ScrollBarWidth, ScrollBarWidth, 0);

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndHScrollBar, SB_CTL, &si);
			si.nPage = Rect.right - ScrollBarWidth;
			SetScrollInfo(hWndHScrollBar, SB_CTL, &si, TRUE);
		}

		SetWindowPos(hWndVScrollBar, NULL, Rect.right - ScrollBarWidth, ScrollBarTop, ScrollBarWidth, Rect.bottom - 60 - ScrollBarTop - bottomOffset, 0);
		SetWindowPos(hWndTrace, NULL, 0, 140, width - ScrollBarWidth, Rect.bottom - 197 - bottomOffset, NULL);
		tracePage = (Rect.bottom - 218 - bottomOffset) / 15;

		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
		si.nPage = tracePage;
		SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);

		if (status == TraceStatus::Loaded)
		{
			status = TraceStatus::LoadPage;
		}

		InvalidateRect(hWndTrace, &BackBuffer_.Rect, FALSE);
	}

	LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{

		case WM_CREATE:
		{
			RECT Rect;
			GetClientRect(hwnd, &Rect);
			BackBuffer_ = AttachBackBuffer(hwnd, horizontalWidth - Rect.right, 0);
			break;
		}

		case WM_SIZE:
		{
			RECT Rect;
			GetClientRect(hwnd, &Rect);
			DeleteDC(BackBuffer_.DeviceContext);
			BackBuffer_ = AttachBackBuffer(hwnd, horizontalWidth - Rect.right, 0);
			break;
		}

		case WM_MOUSEMOVE:
		{
			CaptureKeyboardEvents();

			// Make sure we capture when the mouse leaves the control.
			TRACKMOUSEEVENT mouse = { 0 };
			mouse.cbSize = sizeof(mouse);
			mouse.hwndTrack = hwnd;
			mouse.dwFlags = TME_LEAVE;
			TrackMouseEvent(&mouse);
			break;
		}

		case WM_MOUSELEAVE:
			ReleaseCapture();
			break;

		case WM_LBUTTONDOWN:
		{
			int y = GET_Y_LPARAM(lParam);
			if (y > 20)
			{
				int mark = ((y - 20) / 15) + traceOffset;
				traceCursor = mark;
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			DrawExecutionTrace(BackBuffer_.DeviceContext, &BackBuffer_.Rect);
			BitBlt(hdc, 0, 0, BackBuffer_.Width, BackBuffer_.Height, BackBuffer_.DeviceContext, horizontalOffset, 0, SRCCOPY);

			EndPaint(hwnd, &ps);
			break;
		}

		case WM_DESTROY:
			ReleaseCapture();
			DeleteDC(BackBuffer_.DeviceContext);
			break;
		}

		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	void SetTraceSamples()
	{
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_MAX_TRACE_SAMPLES);
		int index = SendMessage(hCtl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		TCHAR buf[256] = { 0 };
		SendMessage(hCtl, (UINT)CB_GETLBTEXT, (WPARAM)index, (LPARAM)buf);
		std::string samples = buf;
		int maxSamples = std::stoi(samples);
		EmuState.Debugger.SetTraceMaxSamples(maxSamples);

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = maxSamples;
		si.nPage = tracePage;
		si.nPos = 0;
		SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
	}

	void SetupControls(HWND hDlg)
	{
		RECT Rect;
		GetClientRect(hDlg, &Rect);
		
		WNDCLASSW rwc = {0};

		rwc.lpszClassName = L"TraceWindow";

		rwc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		rwc.style = CS_HREDRAW;
		rwc.lpfnWndProc = PanelProc;
		rwc.hCursor = LoadCursor(0, IDC_ARROW);
		RegisterClassW(&rwc);

		hWndTrace = CreateWindowExW(WS_EX_STATICEDGE, L"TraceWindow", NULL,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 140, Rect.right - ScrollBarWidth, Rect.bottom - 197, hDlg, (HMENU)1, NULL, NULL);
		tracePage = (Rect.bottom - 218) / 15;

		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_MAX_TRACE_SAMPLES);
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"10000");
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"100000");
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"250000");
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"500000");
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"1000000");
		SendMessage(hCtl, (UINT)CB_ADDSTRING, 0, (LPARAM)"2000000");
		SendMessage(hCtl, (UINT)CB_SETCURSEL, 0, 0);
		SetTraceSamples();

		hWndVScrollBar = CreateWindowEx(
			0,
			"SCROLLBAR",
			NULL,
			WS_VISIBLE | WS_CHILD | SBS_VERT,
			Rect.right - ScrollBarWidth,
			ScrollBarTop,
			ScrollBarWidth,
			Rect.bottom - 58 - ScrollBarTop,
			hDlg,
			(HMENU)IDT_TRC_VSCROLLBAR,
			(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
			NULL);

		if (!hWndVScrollBar)
		{
			MessageBox(NULL, "Vertical Scroll Bar Failed.", "Error", MB_OK | MB_ICONERROR);
			return;
		}

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = 10000;
		si.nPage = tracePage;
		si.nPos = 0;
		SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);

		hWndHScrollBar = CreateWindowEx(
			0,
			"SCROLLBAR",
			NULL,
			WS_VISIBLE | WS_CHILD | SBS_HORZ,
			0,
			Rect.bottom - 39 - ScrollBarWidth,
			Rect.right - ScrollBarWidth,
			ScrollBarWidth,
			hDlg,
			(HMENU)IDT_TRC_HSCROLLBAR,
			(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
			NULL);

		if (!hWndVScrollBar)
		{
			MessageBox(NULL, "Vertical Scroll Bar Failed.", "Error", MB_OK | MB_ICONERROR);
			return;
		}

		showHScrollBar = true;

		horizontalWidth = EmuState.CpuType == 0 ? 965 : 1035;

		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = horizontalWidth;
		si.nPage = Rect.right - ScrollBarWidth;
		si.nPos = 0;
		SetScrollInfo(hWndHScrollBar, SB_CTL, &si, TRUE);

		SetFocus(hWndTrace);
	}

	void GetAddressList(int hListCtl, Debugger::triggerbuffer_type& list)
	{
		HWND hCtl = GetDlgItem(hWndExecutionTrace, hListCtl);
		int count = SendMessage(hCtl, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
		for (int n = 0; n < count; n++)
		{
			TCHAR buf[256] = { 0 };
			SendMessage(hCtl, (UINT)LB_GETTEXT, (WPARAM)n, (LPARAM)buf);
			int addr;
			std::istringstream is(buf);
			is >> std::hex >> addr;
			if (addr > 0xffff)
			{
				throw std::out_of_range("Trigger address is too large");
			}
			list.push_back(static_cast<unsigned short>(addr));
		}
	}

	void UpdateTriggers(int hListCtl)
	{
		bool startTrigger = hListCtl == IDC_LIST_START_TRACE;

		Debugger::triggerbuffer_type triggers;
		GetAddressList(hListCtl, triggers);

		if (startTrigger)
		{
			EmuState.Debugger.SetTraceStartTriggers(triggers);
		}
		else
		{
			EmuState.Debugger.SetTraceStopTriggers(triggers);
		}
	}

	void EnableTrace()
	{
		currentTrace.clear();
		traceOffset = 0;
		EmuState.Debugger.ResetTrace();
		SetTraceSamples();
		UpdateTriggers(IDC_LIST_START_TRACE);
		UpdateTriggers(IDC_LIST_STOP_TRACE);
		EmuState.Debugger.SetTraceEnable();
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
		SetWindowText(hCtl, "Trace is running");
	}

	void StopTrace()
	{
		EmuState.Debugger.SetTraceDisable();
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
		SetWindowText(hCtl, "Trace is stopped");
	}

	void ResetTrace()
	{
		currentTrace.clear();
		traceOffset = 0;
		EmuState.Debugger.ResetTrace();
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_TRACE_STATUS);
		SetWindowText(hCtl, "Trace is disabled");
	}

	void AddAddressTrace(int hListCtl)
	{
		HWND hCtl = GetDlgItem(hWndExecutionTrace, IDC_EDIT_TRACE_ADDR);

		TCHAR buf[256];
		GetWindowText(hCtl, buf, 256);
		std::string addr = buf;
		for (auto& c : addr) c = toupper(c);

		if ((addr.size() != 4) || (addr.find_first_not_of("0123456789ABCDEF") != std::string::npos))
		{
			MessageBox(hWndExecutionTrace, "Enter a validate 4 byte hex address", "Invalid Address", IDOK);
			return;
		}

		hCtl = GetDlgItem(hWndExecutionTrace, hListCtl);
		int pos = SendMessage(hCtl, LB_ADDSTRING, 0, (LPARAM)addr.c_str());
		SendMessage(hCtl, LB_SETITEMDATA, pos, (LPARAM)0);
		SendMessage(hCtl, LB_SETCURSEL, pos, 0);

		UpdateTriggers(hListCtl);
	}

	void RemoveAddressTrace(int hListCtl)
	{
		HWND hCtl = GetDlgItem(hWndExecutionTrace, hListCtl);
		int selected = SendMessage(hCtl, LB_GETCURSEL, 0, 0);
		if (selected != LB_ERR)
		{
			SendMessage(hCtl, LB_DELETESTRING, selected, 0);

			UpdateTriggers(hListCtl);
		}
	}

	void SetTraceConfiguration()
	{
		HWND hCtl;
		int checked;

		hCtl = GetDlgItem(hWndExecutionTrace, IDC_SHOW_SCN_TRACE);
		checked = SendMessage(hCtl, BM_GETCHECK, 0, 0);
		bool captureScreenTraces = checked == BST_CHECKED;

		hCtl = GetDlgItem(hWndExecutionTrace, IDC_SHOW_EMU_TRACE);
		checked = SendMessage(hCtl, BM_GETCHECK, 0, 0);
		bool captureEmulationTraces = checked == BST_CHECKED;

		EmuState.Debugger.SetTraceOptions(captureScreenTraces, captureEmulationTraces);
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
			if (wParam == VK_UP)
			{
				traceCursor--;
				if (traceCursor < 0)
				{
					traceCursor = 0;
				}
				// Keep trace in range of cursor.
				if (traceCursor < traceOffset)
				{
					traceOffset--;
					if (traceOffset < 0)
					{
						traceOffset = 0;
					}
					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask = SIF_ALL;
					GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
					si.nPos = traceOffset;
					si.fMask = SIF_POS;
					SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
				}
				InvalidateRect(hWndTrace, NULL, FALSE);
			}
			if (wParam == VK_DOWN)
			{
				long samples = EmuState.Debugger.GetTraceSamples();
				traceCursor++;
				if (traceCursor >= samples)
				{
					traceCursor = samples - 1;
				}
				if (traceCursor >= traceOffset + tracePage)
				{
					traceOffset++;
					if ((traceOffset + tracePage) >= samples)
					{
						traceOffset = samples - tracePage;
					}
					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask = SIF_ALL;
					GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
					si.nPos = traceOffset;
					si.fMask = SIF_POS;
					SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
				}
				InvalidateRect(hWndTrace, NULL, FALSE);
			}
			if (wParam == '1')
			{
				EmuState.Debugger.SetTraceMark(1, traceCursor);
				InvalidateRect(hWndTrace, NULL, FALSE);
			}
			if (wParam == '2')
			{
				EmuState.Debugger.SetTraceMark(2, traceCursor);
				InvalidateRect(hWndTrace, NULL, FALSE);
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

	INT_PTR CALLBACK ExecutionTraceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			RECT Rect;
			GetClientRect(hDlg, &Rect);

			hWndExecutionTrace = hDlg;

			SetupControls(hDlg);

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
			si.nPos = roundUp(si.nPos, si.nPage / 2);
			si.fMask = SIF_POS;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			traceOffset = si.nPos;
			if (status == TraceStatus::Loaded)
			{
				status = TraceStatus::LoadPage;
			}
			InvalidateRect(hWndTrace, &BackBuffer_.Rect, FALSE);
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
				si.nPos = si.nMax;
				break;
			case SB_ENDSCROLL:
				break;
			case SB_LINEUP:
				si.nPos -= si.nPage / 2;
				break;
			case SB_LINEDOWN:
				si.nPos += si.nPage / 2;
				break;
			}
			si.nPos = roundUp(si.nPos, si.nPage / 2);
			si.fMask = SIF_POS;
			SetScrollInfo(hWndVScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndVScrollBar, SB_CTL, &si);
			traceOffset = si.nPos;
			if (status == TraceStatus::Loaded)
			{
				status = TraceStatus::LoadPage;
			}
			InvalidateRect(hWndTrace, &BackBuffer_.Rect, FALSE);
			return 0;
		}

		case WM_HSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWndHScrollBar, SB_CTL, &si);

			switch ((int)LOWORD(wParam))
			{
			case SB_PAGELEFT:
				si.nPos -= si.nPage;
				break;
			case SB_PAGERIGHT:
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
				si.nPos = si.nTrackPos;
				break;
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			case SB_LEFT:
				si.nPos = 0;
				break;
			case SB_RIGHT:
				si.nPos = si.nMax;
				break;
			case SB_ENDSCROLL:
				break;
			case SB_LINELEFT:
				si.nPos -= si.nPage / 2;
				break;
			case SB_LINERIGHT:
				si.nPos += si.nPage / 2;
				break;
			}
			si.fMask = SIF_POS;
			SetScrollInfo(hWndHScrollBar, SB_CTL, &si, TRUE);
			GetScrollInfo(hWndHScrollBar, SB_CTL, &si);
			horizontalOffset = si.nPos;
			InvalidateRect(hWndTrace, &BackBuffer_.Rect, FALSE);
			return 0;
		}

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
			case IDT_PROC_TIMER:
				InvalidateRect(hWndTrace, &BackBuffer_.Rect, FALSE);
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_MAX_TRACE_SAMPLES:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SetTraceSamples();
				}
				break;
			case IDC_BTN_ENABLE_TRACE:
				EnableTrace();
				break;
			case IDC_BTN_STOP_TRACE:
				StopTrace();
				break;
			case IDC_BTN_RESET_TRACE:
				ResetTrace();
				break;
			case IDC_BTN_ADD_START_TRACE:
				AddAddressTrace(IDC_LIST_START_TRACE);
				break;
			case IDC_BTN_DEL_START_TRACE:
				RemoveAddressTrace(IDC_LIST_START_TRACE);
				break;
			case IDC_BTN_ADD_STOP_TRACE:
				AddAddressTrace(IDC_LIST_STOP_TRACE);
				break;
			case IDC_BTN_DEL_STOP_TRACE:
				RemoveAddressTrace(IDC_LIST_STOP_TRACE);
				break;
			case IDC_SHOW_EMU_TRACE:
			case IDC_SHOW_SCN_TRACE:
				SetTraceConfiguration();
				break;
			case IDCLOSE:
			case WM_DESTROY:
				KillTimer(hDlg, IDT_PROC_TIMER);
				DestroyWindow(hDlg);
				ExecutionTraceWindow = NULL;
				break;
			}

			break;
		}
		return FALSE;
	}

} } } }


void VCC::Debugger::UI::OpenExecutionTraceWindow(HINSTANCE instance, HWND parent)
{
	if (ExecutionTraceWindow == NULL)
	{
		ExecutionTraceWindow = CreateDialog(
			instance,
			MAKEINTRESOURCE(IDD_EXEC_TRACE),
			parent,
			ExecutionTraceDlgProc);
		ShowWindow(ExecutionTraceWindow, SW_SHOWNORMAL);
	}

	SetFocus(ExecutionTraceWindow);
}

