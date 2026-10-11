
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

//-----------------------------------------------------------------------------
// VCC’s main window handle is not stable. When entering or leaving fullscreen
// mode, the system may destroy and recreate the main window, causing its HWND
// to change. This breaks messaging from other program units or dynamically
// loaded DLLs that rely on a fixed window handle.
//
// To provide a stable messaging endpoint, VCC creates an invisible proxy 
// message window. This proxy window has a permanent HWND and receives
// DLL messages. It forwards these messages to VCC’s main WndProc.
//
// Whenever the main window is recreated, the proxy updates its forwarding 
// target to the new HWND.
//
// The proxy window ensures reliable message delivery to VCC’s WndProc even
// when the main window handle changes.
//-----------------------------------------------------------------------------

#pragma once
#include <windows.h>

namespace VCC::Core
{
	class ProxyMsgWin
	{
	public:
    	static HWND SetProxy(HWND hwnd);
	private:
		static LRESULT CALLBACK ProxyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    	static HWND h_proxy_;
    	static HWND h_target_;
	};
}

