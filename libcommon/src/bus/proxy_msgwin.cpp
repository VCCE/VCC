////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

#include <vcc/bus/proxy_msgwin.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/util/logger.h>

namespace VCC::Core
{
	HWND ProxyMsgWin::h_proxy_ = nullptr;
	HWND ProxyMsgWin::h_target_ = nullptr;
	const char PROXY_CLASS[] = "ProxyMessageWindow";

//------------------------------------------------------------------------------
// Private message handler forwards messages defined in cartridge_messages.h 
//------------------------------------------------------------------------------

	LRESULT CALLBACK ProxyMsgWin::ProxyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
    	switch (uMsg)
    	{
			// allow create
        	case WM_NCCREATE:
            	return TRUE;
			// forwared defined messages
			case WM_VCC_CPU_RESET:
			case WM_VCC_UPD_MENU:
			case WM_VCC_SOFT_RESET:
        		SendMessageA(h_target_, uMsg, wParam, lParam);
				DLOG_C("DLL msg %d\n", uMsg);
    	}
		return 0;
	}

//------------------------------------------------------------------------------
// Set up message only proxy for DLL messages.  This gets called each time
// DirectDrawInterface creates or recreates the VCC WndProc. This happens
// at VCC startup and when full screen mode is toggled.
//------------------------------------------------------------------------------

	HWND ProxyMsgWin::SetProxy(HWND h_VccWndProc)
	{
		HINSTANCE h_inst = GetModuleHandle(NULL);

		// One time create proxy msg window
    	if (h_proxy_ == nullptr) {

    		WNDCLASSA wc = { };
    		wc.lpfnWndProc   = ProxyProc;
    		wc.hInstance     = h_inst;
    		wc.lpszClassName = PROXY_CLASS;

    		if (!RegisterClassA(&wc)) {
				DLOG_C("Failed to register window class\n");
        		return h_VccWndProc;
    		}

    		h_proxy_ = CreateWindowExA(
        		0, PROXY_CLASS, "ProxyWindow",
        		0, 0, 0, 0, 0, HWND_MESSAGE,
        		nullptr, h_inst, nullptr
    		);

			if (!h_proxy_) {
    			DWORD err = GetLastError();
				DLOG_C("CreateWindowExA failed, error = %lu\n", err);
				return h_VccWndProc;
			}
		}

		// Save target handle (VCC WndProc)
    	h_target_ = h_VccWndProc;

		// Return proxy handle
		return h_proxy_;
	}
}

