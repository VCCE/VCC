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

//----------------------------------------------------
// Defines messages sent to VCC WndProc from cart DLL's
//----------------------------------------------------

#pragma once
#include <windows.h>
#include <cstdint>

//----------------------------------------------------
// Structures for moving data across DLL boundaries
// These require a fixed size buffer to contain the data
// buffer must be an array of atomic C types
// size is the size of the data buffer.
//----------------------------------------------------

struct CartLoadRequest {
	const uint32_t size;
	char pluginPath[MAX_PATH];
	CartLoadRequest() noexcept
		: size(MAX_PATH), pluginPath{} {}
};

struct CartNameReply {
	static constexpr uint32_t BUFSIZE = 128;
    const uint32_t size;
    char name[BUFSIZE];
	CartNameReply() noexcept
		: size(BUFSIZE), name{} {}
};

struct CartDescReply {
	static constexpr uint32_t BUFSIZE = 512;
    const uint32_t size;
    char description[BUFSIZE];
	CartDescReply() noexcept
		: size(BUFSIZE), description{} {}
};

//----------------------------------------------------
// DLL <-> VCC Messages and helpers
//----------------------------------------------------

// Fatal (any plugin)
inline constexpr uint32_t WM_VCC_FATAL = WM_APP + 100;
inline LRESULT SendFatalError(HWND hwnd, uint32_t err) {
	return SendMessage(hwnd,WM_VCC_FATAL, err, 0);
}

// Hard Reset (any plugin)
inline constexpr uint32_t WM_VCC_CPU_RESET = WM_APP + 101;
inline LRESULT SendHardReset(HWND hwnd) {
	return SendMessage(hwnd,WM_VCC_CPU_RESET, 0, 0);
}

// Update menu request (any plugin)
inline constexpr uint32_t WM_VCC_UPD_MENU = WM_APP + 102;
inline LRESULT SendMenuUpdate(HWND hwnd) {
	return SendMessage(hwnd,WM_VCC_UPD_MENU, 0, 0);
}

// Soft RESET (any plugin)
inline constexpr uint32_t WM_VCC_SOFT_RESET = WM_APP + 103;
inline LRESULT SendSoftReset(HWND hwnd) {
	return SendMessage(hwnd,WM_VCC_SOFT_RESET, 0, 0);
}

// Set active slot (MPI)
inline constexpr uint32_t WM_VCC_SET_ACTIVE_SLOT = WM_APP + 104;
inline LRESULT SendActiveSlot(HWND hwnd, uint32_t slotnum) {
	return SendMessage(hwnd,WM_VCC_SET_ACTIVE_SLOT, slotnum, 0);
}

// Unload slot request (MPI)
inline constexpr uint32_t WM_VCC_UNLOAD_SLOT = WM_APP + 105;
inline LRESULT SendUnloadSlot(HWND hwnd, uint32_t slotnum) {
	return SendMessage(hwnd,WM_VCC_UNLOAD_SLOT, slotnum, 0);
}

// Load slot request (MPI)
inline constexpr uint32_t WM_VCC_LOAD_SLOT = WM_APP + 106;
inline LRESULT SendLoadSlot(HWND hwnd, uint32_t slot, CartLoadRequest& req) {
	return SendMessage(
		hwnd,
		WM_VCC_LOAD_SLOT,
		static_cast<WPARAM>(slot),
		reinterpret_cast<LPARAM>(&req)
	);
}

// Request name of cartridge in slot (MPI)
inline constexpr uint32_t WM_VCC_GET_CART_NAME = WM_APP + 107;
inline LRESULT GetCartName(HWND hwnd, uint32_t slot, CartNameReply& rpy) {
	return SendMessage(
		hwnd,
		WM_VCC_GET_CART_NAME,
		static_cast<WPARAM>(slot),
		reinterpret_cast<LPARAM>(&rpy)
	);
}

// Request desription of cartridge in slot (MPI)
inline constexpr uint32_t WM_VCC_GET_CART_DESCRIPT = WM_APP + 108;
inline LRESULT GetCartDesc(HWND hwnd, uint32_t slot, CartDescReply& rpy) {
	return SendMessage(
		hwnd,
		WM_VCC_GET_CART_DESCRIPT,
		static_cast<WPARAM>(slot),
		reinterpret_cast<LPARAM>(&rpy)
	);
}

