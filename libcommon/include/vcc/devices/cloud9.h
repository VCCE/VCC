#pragma once
#include <vcc/core/exports.h>  // defines LIBCOMMON_EXPORT if libcommon is a DLL
#include <Windows.h>

namespace vcc::devices::rtc
{

	class LIBCOMMON_EXPORT cloud9
	{
	public:

		void set_read_only(bool value);

		unsigned char read_port(unsigned short port_id);


	private:

		void set_time();


	private:

		SYSTEMTIME now;
		unsigned __int64 InBuffer = 0;
		unsigned __int64 OutBuffer = 0;
		unsigned char BitCounter = 0;
		unsigned char TempHour = 0;
		unsigned char AmPmBit = 0;
		unsigned __int64 CurrentBit = 0;
		unsigned char FormatBit = 0; //1 = 12Hour Mode
		unsigned char CookieRecived = 0;
		unsigned char WriteEnabled = 0;
	};

}
