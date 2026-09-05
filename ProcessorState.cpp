//  This file is part of VCC (Virtual Color Computer).
//
//      VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation, either version 3 of the License, or
//      (at your option) any later version.
//
//      VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
//
//      Processor State Display - Part of the Debugger package for VCC
//      Authors: Mike Rojas, Chet Simpson
#include "ProcessorState.h"
#include "Debugger.h"
#include "DebuggerUtils.h"
#include "defines.h"
#include "resource.h"
#include <stdexcept>
#include "config.h"

// Generate hex char string from int value
// FIXME: This obfuscates that the callsite is using an rvalue.
#define HEXCHAR(v,l) (char *) ToHexString(v,l).c_str()

extern SystemState EmuState;

namespace VCC::Debugger::UI {
namespace {
    const std::string cWindowSizeX("WindowSizeX");
    const std::string cWindowSizeY("WindowSizeY");
    const std::string cWindowPosX("WindowPosX");
    const std::string cWindowPosY("WindowPosY");

    HWND    ProcessorStateWindow = nullptr;
    BackBufferInfo  BackBuf;
    std::unique_ptr<OpDecoder> Decoder;

    void DrawCntrTxt(HDC hdc, const RECT& r, const char * str, int x, int y, int l, int h)
    {
        RECT rc;
        SetRect(&rc, r.left + x, r.top + y, r.left + x + l, r.top + y + h);
        DrawText(hdc, str, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void DrawTextBox(HDC hdc, const RECT& r, int x, int y, int l, int h)
    {
        int rx = r.left + x;
        int ry = r.top + y;
        MoveToEx(hdc, rx, ry, nullptr);
        LineTo(hdc, rx + l, ry);
        LineTo(hdc, rx + l, ry + h);
        LineTo(hdc, rx, ry + h);
        LineTo(hdc, rx, ry);
    }

    void DrawVrtLine(HDC hdc, const RECT& r, int x, int y, int h)
    {
        int rx = r.left + x;
        int ry = r.top + y;
        MoveToEx(hdc, rx, ry, nullptr);
        LineTo(hdc, rx, ry + h);
    }

    void DrawProcessorState(HDC hdc, LPCRECT clientRect)
    {
        RECT rect = *clientRect;
        Decoder = std::make_unique<OpDecoder>();

        // Clear our background.
        HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        FillRect(hdc, &rect, brush);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
        SelectObject(hdc, pen);

        // Draw the border.
        MoveToEx(hdc, rect.left, rect.top, nullptr);
        LineTo(hdc, rect.right - 1, rect.top);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.top);

        HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, FIXED_PITCH, TEXT("Consolas"));

        // Draw register boxes and lables
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(138, 27, 255));
        // args     HDC RECT   X   Y   L   H
        DrawTextBox(hdc,rect,  8, 18, 72, 18);   //D
        DrawTextBox(hdc,rect, 88, 18, 72, 18);   //W
        DrawTextBox(hdc,rect,168, 18, 72, 18);   //X
        DrawTextBox(hdc,rect,248, 18, 72, 18);   //Y
        DrawTextBox(hdc,rect,328, 18, 72, 18);   //S
        DrawTextBox(hdc,rect,408, 18, 72, 18);   //U
        DrawTextBox(hdc,rect,295, 60, 40, 18);   //DP
        DrawTextBox(hdc,rect,350, 60, 40, 18);   //MD
        DrawTextBox(hdc,rect,408, 60, 72, 18);   //V
        DrawTextBox(hdc,rect, 35,102, 72, 18);   //PC
        DrawVrtLine(hdc,rect, 45, 18, 18);       //A|B
        DrawVrtLine(hdc,rect,125, 18, 18);       //E|F

        DrawCntrTxt(hdc,rect,"D",   8,  0, 72, 18); //D
        DrawCntrTxt(hdc,rect,"W",  90,  0, 72, 18); //W
        DrawCntrTxt(hdc,rect,"X", 168,  0, 72, 18); //X
        DrawCntrTxt(hdc,rect,"Y", 248,  0, 72, 18); //Y
        DrawCntrTxt(hdc,rect,"S", 328,  0, 72, 18); //S
        DrawCntrTxt(hdc,rect,"U", 408,  0, 72, 18); //U
        DrawCntrTxt(hdc,rect,"A",   8, 36, 36, 18); //A
        DrawCntrTxt(hdc,rect,"B",  44, 36, 36, 18); //B
        DrawCntrTxt(hdc,rect,"E",  90, 36, 36, 18); //E
        DrawCntrTxt(hdc,rect,"F", 126, 36, 36, 18); //F
        DrawCntrTxt(hdc,rect,"DP",295, 43, 45, 18); //DP
        DrawCntrTxt(hdc,rect,"MD",350, 43, 45, 18); //MD
        DrawCntrTxt(hdc,rect,"V", 408, 43, 72, 18); //V
        DrawCntrTxt(hdc,rect,"PC",  8,102, 25, 18); //PC

        // Draw CC Box with bit seperators and lables
        DrawTextBox(hdc,rect,35,60,200,18);
        for (int x = 60; x < 235 ; x+=25) DrawVrtLine(hdc,rect,x,60,18);
        DrawCntrTxt(hdc,rect,"CC",  8,60,25,18);
        DrawCntrTxt(hdc,rect,"E",  35,78,25,18);
        DrawCntrTxt(hdc,rect,"F",  60,78,25,18);
        DrawCntrTxt(hdc,rect,"H",  85,78,25,18);
        DrawCntrTxt(hdc,rect,"I", 110,78,25,18);
        DrawCntrTxt(hdc,rect,"N", 135,78,25,18);
        DrawCntrTxt(hdc,rect,"Z", 160,78,25,18);
        DrawCntrTxt(hdc,rect,"V", 185,78,25,18);
        DrawCntrTxt(hdc,rect,"C", 210,78,25,18);

        // Instruction box
        DrawTextBox(hdc,rect,120,102,120,18);

        // A message
        DrawCntrTxt(hdc,rect,"W, MD, V are 6309 CPU only",260,108,250,18);

        // Pull out processor state.  Do it quickly to keep the emulator frame rate high.
        const auto& regs(EmuState.Debugger.GetProcessorStateCopy());

        SetTextColor(hdc, RGB(0, 0, 0));

        // Dump the registers.
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.A,2),  10, 18,36,18);    //A
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.B,2),  44, 18,36,18);    //B
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.X,4), 168, 18,72,18);    //X
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.Y,4), 248, 18,72,18);    //Y
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.S,4), 328, 18,72,18);    //S
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.U,4), 408, 18,72,18);    //U
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.PC,4), 35,102,72,18);    //PC
        DrawCntrTxt(hdc,rect,HEXCHAR(regs.DP,2),295, 60,40,18);    //DP
        if (EmuState.CpuType == 1) {
            DrawCntrTxt(hdc,rect,HEXCHAR(regs.E,2),  90,18,36,18); //E
            DrawCntrTxt(hdc,rect,HEXCHAR(regs.F,2), 126,18,36,18); //F
            DrawCntrTxt(hdc,rect,HEXCHAR(regs.MD,2),350,60,40,18); //MD
            DrawCntrTxt(hdc,rect,HEXCHAR(regs.V,4), 408,60,72,18); //V
        }
        // Condition Code bits
        int x = 35;
        for (int n = 7; n >= 0; n--) {
          if (regs.CC & 1 << n) {
            DrawCntrTxt(hdc, rect, "1", x, 60, 25, 18);
          } else {
            DrawCntrTxt(hdc, rect, "-", x, 60, 25, 18);
          }
          x += 25;
        }
        // Instruction
        VCC::CPUTrace trace = {};
        Decoder->DecodeInstruction(regs,trace);
        trace.instruction.append(6 - trace.instruction.length(),' ');
        std::string ins = trace.instruction + trace.operand;
        RECT rc;
        SetRect(&rc,rect.left+130,rect.top+102,rect.left+250,rect.top+120);
        DrawText(hdc, ins.c_str(), -1, &rc, DT_VCENTER | DT_SINGLELINE);
        // Cleanup.
        DeleteObject(pen);
        DeleteObject(hFont);
    }

    void SaveSettings(HWND hDlg)
    {
        constexpr int cHeaderHeight = 20;			// header with labels

        RECT CurWindow;
        ::GetWindowRect(hDlg, &CurWindow);
        RECT CurScreen;
        ::GetClientRect(hDlg, &CurScreen);
        int clientWidth = (int)CurScreen.right;
        int clientHeight = (int)CurScreen.bottom;

        {
            VCC::Rect rect;
            // remember positioning:
            rect.x = CurWindow.left;
            rect.y = CurWindow.top;

            // remember size:
            rect.w = clientWidth; // Used for saving new window size to the ini file.
            rect.h = clientHeight - cHeaderHeight;

            auto& s = Setting();
            std::string section("ProcessorState");
            auto write = [&](const std::string& key, int value)
            {
                s.write(section, key, value);
            };
            write(cWindowSizeX, rect.w);
            write(cWindowSizeY, rect.h);
            write(cWindowPosX, rect.x);
            write(cWindowPosY, rect.y);
        }
    }

    void LoadSettings(HWND hDlg)
    {
        if (hDlg != nullptr)
        {
            Rect rect;

            rect.x = CW_USEDEFAULT;
            rect.y = CW_USEDEFAULT;
            rect.w = 488;
            rect.h = 143;

            if (!GetAsyncKeyState(VK_SHIFT))
            {
                auto& s = Setting();
                std::string section("ProcessorState");
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
            ::GetWindowRect(hDlg, &ra);

            int width = rect.w + windowBorderWidth;
            int height = rect.h + windowBorderHeight + 0; /*GetRenderWindowStatusBarHeight();*/
            int flags = SWP_NOOWNERZORDER | SWP_NOZORDER;
            int x = rect.IsDefaultX() ? ra.left : rect.x;
            int y = rect.IsDefaultY() ? ra.top : rect.y;
            SetWindowPos(hDlg, nullptr, x, y, width, height, flags);
        }
    }

    INT_PTR CALLBACK ProcessorStateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
    {

        switch (message)
        {
        case WM_INITDIALOG:
        {
            RECT Rect;
            GetClientRect(hDlg, &Rect);
            BackBuf = AttachBackBuffer(hDlg, 0, -35);
            SetTimer(hDlg, IDT_PROC_TIMER, 64, nullptr);
            LoadSettings(hDlg);
            break;
        }

        case WM_PAINT:
        {
            if (EmuState.Debugger.IsHalted()) {
                SetWindowTextA(hDlg,"Processor State (Halted)");
                SendMessageA(GetDlgItem(hDlg,IDC_BTN_CPU_HALT),WM_SETTEXT,0, (LPARAM) "Run");
                EnableWindow(GetDlgItem(hDlg,IDC_BTN_CPU_STEP),true);
                EnableWindow(GetDlgItem(hDlg,IDC_BTN_SET_PC),true);
            } else {
                SetWindowTextA(hDlg,"Processor State");
                SendMessageA(GetDlgItem(hDlg,IDC_BTN_CPU_HALT),WM_SETTEXT,0, (LPARAM) "Halt");
                EnableWindow(GetDlgItem(hDlg,IDC_BTN_CPU_STEP),false);
                EnableWindow(GetDlgItem(hDlg,IDC_BTN_SET_PC),false);
            }

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hDlg, &ps);

            DrawProcessorState(BackBuf.DeviceContext, &BackBuf.Rect);
            BitBlt(hdc, 0, 0, BackBuf.Width, BackBuf.Height, BackBuf.DeviceContext, 0, 0, SRCCOPY);

            EndPaint(hDlg, &ps);
            break;
        }

        case WM_TIMER:
            switch (wParam)
            {
            case IDT_PROC_TIMER:
                InvalidateRect(hDlg, &BackBuf.Rect, FALSE);
                return 0;
            }
            break;

        case WM_NCDESTROY:
            SaveSettings(hDlg);
            KillTimer(hDlg, IDT_PROC_TIMER);
            DeleteDC(BackBuf.DeviceContext);
            ProcessorStateWindow = nullptr;
            break;

        case WM_CLOSE:
            DestroyWindow(hDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_BTN_CPU_HALT:
                EmuState.Debugger.ToggleRun();
                break;
            case IDC_BTN_SET_PC:
            {
                char buf[16];
                char *eptr;
                long val;
                GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PC_ADDR), buf, 8);
                val = strtol(buf,&eptr,16);
                if (*buf == '\0' || *eptr != '\0' || val < 0 || val > 65535)
                {
                    MessageBox(hDlg,"Invalid hex address","Error",IDOK);
                } else {
                    CPUForcePC(val & 0xFFFF);
                }
                break;
            }
            case IDC_BTN_CPU_STEP:
                EmuState.Debugger.QueueStep();
                break;

            case IDCLOSE:
                DestroyWindow(hDlg);
                break;
            }
            break;
        }
        return FALSE;
    }

} }


void VCC::Debugger::UI::OpenProcessorStateWindow(HINSTANCE instance, HWND parent)
{
    if (ProcessorStateWindow == nullptr)
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
