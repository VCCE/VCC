#pragma once
#include <vcc/core/detail/exports.h>


namespace vcc::devices::rtc
{

	class LIBCOMMON_EXPORT oki_m6242b
	{
	public:

		unsigned char read_port(unsigned short port_id);
		void write_port(unsigned char data, unsigned char port_id);


	private:

		unsigned char time_register_ = 0;
		unsigned char hour12_ = 0;
	};

}
