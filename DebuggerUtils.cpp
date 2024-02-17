//	Copyright 2015 by Joseph Forgione
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
//		Debugger Utilities - Part of the Debugger package for VCC
//		Author: Chet Simpson
#include "Debugger.h"
#include "tcc1014mmu.h"
#include <sstream>
#include <iomanip>
#include <stdarg.h>
#include <memory>
#include "defines.h"

namespace VCC { namespace Debugger
{

	std::string ToHexString(long value, int length, bool leadingZeros)
	{
		std::ostringstream fmt;

		fmt << std::hex << std::setw(length) << std::uppercase << std::setfill(leadingZeros ? '0' : ' ') << value;

		return fmt.str();
	}

	std::string ToDecimalString(long value, int length, bool leadingZeros)
	{
		std::ostringstream fmt;

		fmt << std::dec << std::setw(length) << std::setfill(leadingZeros ? '0' : ' ') << value;

		return fmt.str();
	}

	std::string ToByteString(std::vector<unsigned char> bytes)
	{
		std::ostringstream fmt;

		for (auto& b : bytes)
		{
			if (fmt.str().size() > 0)
			{
				fmt << " ";
			}
			fmt << ToHexString(b, 2, true);
		}

		return fmt.str();
	}

	bool replace(std::string& str, const std::string& from, const std::string& to) 
	{
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	int roundUp(int numToRound, int multiple)
	{
		if (multiple == 0)
			return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0)
			return numToRound;

		return numToRound + multiple - remainder;
	}

	int roundDn(int numToRound, int multiple)
	{
		if (multiple == 0)
			return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0)
			return numToRound;

		return numToRound - remainder;
	}
    // Get memory for Decode, CPU address or Physical Block + Address	
	unsigned char DbgRead8(bool phyAddr, unsigned char block,unsigned short PC) {

//unsigned int NumBlocks[4] = {0x2,0x8,0x20,0x80};
//maxblock = EmuState.RamSize / 0x2000;
//unsigned short GetMem(long address) {

		if (phyAddr) {
			long addr = PC + block * 0x2000;
			return (unsigned char) GetMem(addr);
		} else {
			return MemRead8(PC);
		}
	}
	
} }

namespace VCC { namespace Debugger { namespace UI
{

	BackBufferInfo AttachBackBuffer(HWND hWnd, int widthAdjust, int heightAdjust)
	{
		BackBufferInfo	backBuffer;

		GetClientRect(hWnd, &backBuffer.Rect);

		backBuffer.Rect.right += widthAdjust;
		backBuffer.Rect.bottom += heightAdjust;

		backBuffer.Width = backBuffer.Rect.right - backBuffer.Rect.left;
		backBuffer.Height = backBuffer.Rect.bottom - backBuffer.Rect.top;

		HDC hdc = GetDC(hWnd);

		backBuffer.Bitmap = CreateCompatibleBitmap(hdc, backBuffer.Width, backBuffer.Height);
		if (backBuffer.Bitmap == NULL)
		{
			throw std::runtime_error("failed to create backbuffer bitmap");
		}

		backBuffer.DeviceContext = CreateCompatibleDC(hdc);
		if (backBuffer.DeviceContext == NULL)
		{
			throw std::runtime_error("failed to create the backbuffer device context");
		}

		HBITMAP oldbmp = (HBITMAP)SelectObject(backBuffer.DeviceContext, backBuffer.Bitmap);
		DeleteObject(oldbmp);
		ReleaseDC(hWnd, hdc);

		return backBuffer;
	}

} } }

