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
#pragma once
#include <Windows.h>
#include <string>
#include <vector>




namespace VCC::Debugger
{

    template<class Type_>
    inline LPCSTR ToLPCSTR(std::basic_string<Type_>& str)
    {
        return str.c_str();
    }

    template<class Type_>
    inline LPCSTR ToLPCSTR(const std::basic_string<Type_>& str)
    {
        return str.c_str();
    }

    template<class Type_>
    LPCSTR ToLPCSTR(const std::basic_filebuf<Type_>&& str)
    {
        static_assert("RValue not supported for ToLPCSTR");
    }

    std::string ToHexString(long value, int length, bool leadingZeros = true);
    std::string ToDecimalString(long value, int length, bool leadingZeros = true);
	std::string ToByteString(const std::vector<unsigned char>& bytes);
	bool replace(std::string& str, const std::string& from, const std::string& to);
	int roundUp(int numToRound, int multiple);
	int roundDn(int numToRound, int multiple);
	unsigned char DbgRead8(bool phyAddr, unsigned short block, unsigned short PC);
}

namespace VCC::Debugger::UI
{

	struct BackBufferInfo
	{
		HDC		DeviceContext;
		HBITMAP Bitmap;
		RECT    Rect;
		int		Width;
		int		Height;
	};

	BackBufferInfo AttachBackBuffer(HWND hWnd, int widthAdjust, int heightAdjust);

}
