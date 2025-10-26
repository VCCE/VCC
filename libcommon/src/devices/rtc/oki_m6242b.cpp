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
#include <vcc/devices/rtc/oki_m6242b.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>


/* Table description:							   Bit3  Bit2  Bit1  Bit0
Write to $FF51 read from $FF50
	0x00		1-second digit register				 S8    S4    S2    S1
	0x01		10-second digit register			  x   S40   S20   S10
	0x02		1-minute digit register				Mi8   Mi4   Mi2   Mi1
	0x03		10-minute digit register			  x  Mi40  Mi20  Mi10
	0x04		1-hour digit register				 H8    H4    H2    H1
	0x05		PM/AM, 10-hour digit register		  x   P/A   H20   H10
	0x06		1-day digit register				 D8    D4    D2    D1
	0x07		10-day digit register				  x     x   D20   D10
	0x08		1-month digit register				Mo8   Mo4   Mo2   Mo1
	0x09		10-month digit register			   Mo80  Mo40  Mo20  Mo10
	0x0A		1-year digit register				 Y8    Y4    Y2    Y1
	0x0B		10-yead digit register				Y80   Y40   Y20   Y10
	0x0C		Week register						  x    W4    W2    W1

													 30
	0x0D		Control register D					Sec   IRQ  Busy  Hold
													adj  Flag
													           ITRP
	0x0E		Control register E					 T1    T0 /STND  Mask
													  
													           
	0x0F		Control register F				   Test 24/12  Stop  Rest
													  
	
	Note: Digits are BDC. Registers only four bits wide.
	X denotes 'not used'
*/

namespace vcc::devices::rtc
{

	unsigned char oki_m6242b::read_data() const
	{
		unsigned char ret_val = 0;

		SYSTEMTIME now;

		GetLocalTime(&now);
		switch (time_register_)
		{

		case 0:
			ret_val = now.wSecond % 10;
			break;

		case 1:
			ret_val = now.wSecond / 10;
			break;

		case 2:
			ret_val = now.wMinute % 10;
			break;

		case 3:
			ret_val = now.wMinute / 10;
			break;

		case 4:
			ret_val = now.wHour % 10;
			break;

		case 5:
			ret_val = now.wHour / 10;
			break;

		case 6:
			ret_val = now.wDay % 10;
			break;

		case 7:
			ret_val = now.wDay / 10;
			break;

		case 8:
			ret_val = now.wMonth % 10;
			break;

		case 9:
			ret_val = now.wMonth / 10;
			break;

		case 0xA:
			ret_val = now.wYear % 10;
			break;

		case 0xB:
			ret_val = (now.wYear % 100) / 10;
			break;

		case 0xC:
			ret_val = static_cast<unsigned char>(now.wDayOfWeek); // May not be right
			break;

		case 0xD:
			break;

		case 0xE:
			break;

		case 0xF:
			break;
		}

		return static_cast<unsigned char>(ret_val);
	}


	void oki_m6242b::set_read_write_address(size_t address)
	{
		time_register_ = (address & 0xF);
	}

	void oki_m6242b::write_data(unsigned char value)
	{
		if (time_register_ == 0x0F)
		{
			hour12_ = !((value & 4) >> 2);
		}
	}

}