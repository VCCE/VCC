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
#include <sstream>
#include <iomanip>


namespace VCC { namespace Debugger
{

	std::string ToHexString(long value, int length, bool leadingZeros)
	{
		std::ostringstream fmt;

		fmt << std::hex << std::setw(length) << std::setfill(leadingZeros ? '0' : ' ') << value;

		return fmt.str();
	}

	std::string ToDecimalString(long value, int length, bool leadingZeros)
	{
		std::ostringstream fmt;

		fmt << std::dec << std::setw(length) << std::setfill(leadingZeros ? '0' : ' ') << value;

		return fmt.str();
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


